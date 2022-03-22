#include "PhysicsSystem.h"
#include "../../generated/Physics.hpp"
#include "../../game/Game.h"
#include <utils/gltf_model_loader.h>
#include <btBulletCollisionCommon.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

struct BulletStuff
{
    // collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
    btDefaultCollisionConfiguration collisionConfig;

    btCollisionDispatcher dispatcher;

    // btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
    btDbvtBroadphase overlappingPairCache;

    btSequentialImpulseConstraintSolver solver;

    btDiscreteDynamicsWorld world;

    btEmptyShape emptyShape;

    BulletStuff()
        :
        dispatcher(&collisionConfig),
        world(&dispatcher, &overlappingPairCache, &solver, &collisionConfig)
    {

        world.setGravity(btVector3(0, 0, 0));
    }

    ~BulletStuff()
    {
    }
};

btVector3 vec3ToBt(const vec3 &v)
{
    return btVector3(v.x, v.y, v.z);
}

vec3 btToVec3(const btVector3 &v)
{
    return vec3(v.getX(), v.getY(), v.getZ());
}

btQuaternion quatToBt(const quat &q)
{
    return btQuaternion(q.x, q.y, q.z, q.w);
}

quat btToQuat(const btQuaternion &q)
{
    return quat(q.getW(), q.getX(), q.getY(), q.getZ());
}

void syncCollider(btCollisionObject *obj, Collider &collider)
{
    if (collider.dirty<&Collider::bounciness>())
        obj->setRestitution(collider.bounciness);

    if (collider.dirty<&Collider::frictionCoefficent>())
    {
        collider.frictionCoefficent = max(0.f, collider.frictionCoefficent);
        obj->setFriction(collider.frictionCoefficent);
    }

    // if (collider.dirty<&Collider::bodyOffsetTranslation>() || collider.dirty<&Collider::bodyOffsetRotation>())
    // {
        // TODO: use btCompoundShape for this
        // https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=9546
    // }
    collider.undirtAll();
}

void syncRigidBody(RigidBody &body, BulletStuff *bullet)
{
    assert(body.bt);
    if (body.collider.anyDirty())
        syncCollider(body.bt, body.collider);

    if (!body.anyDirty())
        return;

    // something is dirty
    body.bt->activate();

    if (body.dirty<&RigidBody::gravity>())
        body.bt->setGravity(vec3ToBt(body.gravity));

    body.linearDamping = max(body.linearDamping, 0.f);
    body.angularDamping = max(body.angularDamping, 0.f);
    body.bt->setDamping(body.linearDamping, body.angularDamping);

    if (body.dirty<&RigidBody::allowSleep>())
    {
        body.bt->setActivationState(!body.allowSleep ? DISABLE_DEACTIVATION : ACTIVE_TAG);
    }
    if (body.dirty<&RigidBody::mass>())
    {
        btVector3 inertia;
        if (!dynamic_cast<btEmptyShape *>(body.bt->getCollisionShape()))
            body.bt->getCollisionShape()->calculateLocalInertia(body.mass, inertia);

        body.bt->setMassProps(body.mass, inertia);
    }

    if (body.dirty<&RigidBody::angularAxisFactor>())
        body.bt->setAngularFactor(vec3ToBt(body.angularAxisFactor));
    if (body.dirty<&RigidBody::linearAxisFactor>())
        body.bt->setLinearFactor(vec3ToBt(body.linearAxisFactor));

    body.undirtAll();
}

void addRigidBody(BulletStuff *bullet, RigidBody *rb)
{
    bullet->world.addRigidBody(rb->bt, rb->collider.collisionCategoryBits, rb->collider.collideWithMaskBits);
    rb->bedirtAll();
    rb->collider.bedirtAll();
    syncRigidBody(*rb, bullet);
    rb->bt->activate();
}

void readdBodyForChange(entt::registry &reg, entt::entity e, BulletStuff *bullet, std::function<void()> change)
{
    if (auto rb = reg.try_get<RigidBody>(e))
    {
        assert(rb->bt);
        bullet->world.removeRigidBody(rb->bt);
        change();
        addRigidBody(bullet, rb);
    }
    else change();

}

void PhysicsSystem::init(EntityEngine* engine)
{
    room = dynamic_cast<Room3D *>(engine);
    if (!room) throw gu_err("engine is not a room");

    bullet = new BulletStuff;

    updateFrequency = 60;

    engine->entities.on_destroy<RigidBody>()				.connect<&PhysicsSystem::onRigidBodyRemoved>(this);
    engine->entities.on_replace<RigidBody>()				.connect<&PhysicsSystem::onRigidBodyRemoved>(this);
    engine->entities.on_construct<RigidBody>()				.connect<&PhysicsSystem::onRigidBodyAdded>(this);

    // only allow a single type of shape:
    engine->entities.on_construct<BoxColliderShape>()		.connect<&entt::registry::remove_if_exists<SphereColliderShape, CapsuleColliderShape, ConvexColliderShape, ConcaveColliderShape>>();
    engine->entities.on_construct<SphereColliderShape>()	.connect<&entt::registry::remove_if_exists<BoxColliderShape, CapsuleColliderShape, ConvexColliderShape, ConcaveColliderShape>>();
    engine->entities.on_construct<CapsuleColliderShape>()	.connect<&entt::registry::remove_if_exists<BoxColliderShape, SphereColliderShape, ConvexColliderShape, ConcaveColliderShape>>();
    engine->entities.on_construct<ConvexColliderShape>()	.connect<&entt::registry::remove_if_exists<BoxColliderShape, SphereColliderShape, CapsuleColliderShape, ConcaveColliderShape>>();
    engine->entities.on_construct<ConcaveColliderShape>()	.connect<&entt::registry::remove_if_exists<BoxColliderShape, SphereColliderShape, CapsuleColliderShape, ConvexColliderShape>>();

    engine->entities.on_construct<BoxColliderShape>()		.connect<&PhysicsSystem::onBoxAdded>(this);
    engine->entities.on_construct<SphereColliderShape>()	.connect<&PhysicsSystem::onSphereAdded>(this);
    engine->entities.on_construct<CapsuleColliderShape>()	.connect<&PhysicsSystem::onCapsuleAdded>(this);
    engine->entities.on_construct<ConvexColliderShape>()	.connect<&PhysicsSystem::onConvexAdded>(this);
    engine->entities.on_construct<ConcaveColliderShape>()	.connect<&PhysicsSystem::onConcaveAdded>(this);
    
    engine->entities.on_destroy<BoxColliderShape>()			.connect<&PhysicsSystem::onBoxRemoved>(this);
    engine->entities.on_destroy<SphereColliderShape>()		.connect<&PhysicsSystem::onSphereRemoved>(this);
    engine->entities.on_destroy<CapsuleColliderShape>()		.connect<&PhysicsSystem::onCapsuleRemoved>(this);
    engine->entities.on_destroy<ConvexColliderShape>()		.connect<&PhysicsSystem::onConvexRemoved>(this);
    engine->entities.on_destroy<ConcaveColliderShape>()		.connect<&PhysicsSystem::onConcaveRemoved>(this);
    engine->entities.on_replace<BoxColliderShape>()			.connect<&PhysicsSystem::onBoxRemoved>(this);
    engine->entities.on_replace<SphereColliderShape>()		.connect<&PhysicsSystem::onSphereRemoved>(this);
    engine->entities.on_replace<CapsuleColliderShape>()		.connect<&PhysicsSystem::onCapsuleRemoved>(this);
    engine->entities.on_replace<ConvexColliderShape>()		.connect<&PhysicsSystem::onConvexRemoved>(this);
    engine->entities.on_replace<ConcaveColliderShape>()		.connect<&PhysicsSystem::onConcaveRemoved>(this);

    // todo/note: replacing components is not supported by the lua api, however, if done from C++, things like colliders probably wont be added.
}


bool PhysicsSystem::loadColliderMeshesFromGLTF(const char *path, bool force, bool convex)
{
    /*
    if (!force && modelFileLoadTime.find(path) != modelFileLoadTime.end())
        return false;

    GltfModelLoader loader(VertAttributes().add_(VertAttributes::POSITION));

    if (stringEndsWith(path, ".glb"))
        loader.fromBinaryFile(path);
    else
        loader.fromASCIIFile(path);

    for (auto &mesh : loader.meshes)
        colliderMeshes[mesh->name] = mesh;

    if (!convex)
    {
        for (auto &mesh : loader.meshes)
        {
            auto triMesh = reactCommon.createTriangleMesh();

            for (auto &part : mesh->parts)
            {
                if (mesh->vertices.empty())
                    continue;

                if (part.indices.empty())
                    continue;

                reactphysics3d::TriangleVertexArray *tris = new reactphysics3d::TriangleVertexArray(
                    mesh->nrOfVertices(),
                    &mesh->vertices.at(0),
                    VertAttributes::POSITION.byteSize,
                    part.indices.size() / 3,
                    &part.indices.at(0),
                    3 * sizeof(unsigned short),
                    reactphysics3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                    reactphysics3d::TriangleVertexArray::IndexDataType::INDEX_SHORT_TYPE
                );
                reactTris.push_back(tris);
                triMesh->addSubpart(tris);
            }

            reactConcaveMeshes[mesh->name] = reactCommon.createConcaveMeshShape(triMesh);
        }
    }
    else
    {
        for (auto &mesh : loader.meshes)
        {
            if (mesh->vertices.empty())
                continue;
            if (mesh->parts.empty())
                continue;
            auto &part = mesh->parts[0];
            if (part.indices.empty())
                continue;

            auto &faces = polygonFaces.emplace_front();
            faces.reserve(part.indices.size() / 3);

            for (uint i = 0; i < part.indices.size(); i += 3)
            {
                faces.emplace_back();
                faces.back().indexBase = i;
                faces.back().nbVertices = 3;
            }

            auto polys = new reactphysics3d::PolygonVertexArray(
                mesh->nrOfVertices(),
                &mesh->vertices.at(0),
                VertAttributes::POSITION.byteSize,
                &part.indices.at(0),
                sizeof(unsigned short),
                faces.size(),
                &faces.at(0),
                reactphysics3d::PolygonVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                reactphysics3d::PolygonVertexArray::IndexDataType::INDEX_SHORT_TYPE
            );
            auto polyMesh = reactCommon.createPolyhedronMesh(polys);
            reactConvexMeshes[mesh->name] = reactCommon.createConvexMeshShape(polyMesh);
        }
    }

    modelFileLoadTime[path] = glfwGetTime();
    */
    return true;
}

void PhysicsSystem::rayTest(const vec3 &from, const vec3 &to, const RayHitCallback &cb, int mask, int category)
{
    if (!room)
        throw gu_err("Not initialized!");

    auto btFrom = vec3ToBt(from);
    auto btTo = vec3ToBt(to);

    /*
    struct Callback : public btCollisionWorld::ClosestRayResultCallback
    {
        const RayHitCallback &cb;
        const entt::registry &reg;

        Callback(const entt::registry &reg, const RayHitCallback &cb, btVector3 &from, btVector3 &to) : reg(reg), cb(cb), ClosestRayResultCallback(from, to)
        {}

        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult &rayResult, bool normalInWorldSpace)
        {
            auto hF = btCollisionWorld::ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);

            entt::entity e = entt::entity(rayResult.m_collisionObject->getUserIndex());

            if (!reg.valid(e))
                throw gu_err("Hit an object using a raycast, but its UserIndex does not return a valid entity!");

            return cb(e, btToVec3(m_hitPointWorld), btToVec3(m_hitNormalWorld)) ? ; // return 1 to skip
        }
    } btCallback(room->entities, cb, btFrom, btTo);

    */
    btCollisionWorld::ClosestRayResultCallback btCallback(btFrom, btTo);

    btCallback.m_collisionFilterGroup = category;
    btCallback.m_collisionFilterMask = mask;

    bullet->world.rayTest(btFrom, btTo, btCallback);

    if (btCallback.hasHit())
    {
        entt::entity e = entt::entity(btCallback.m_collisionObject->getUserIndex());

        if (!room->entities.valid(e))
            throw gu_err("Hit an object using a raycast, but its UserIndex does not return a valid entity!");

        cb(e, btToVec3(btCallback.m_hitPointWorld), btToVec3(btCallback.m_hitNormalWorld));
    }
}

void PhysicsSystem::debugDraw(const std::function<void(const vec3 &a, const vec3 &b, const vec3 &color)> &lineCallback)
{
    struct Drawer : public btIDebugDraw
    {
        decltype(lineCallback) &cb;

        int debugMode;

        Drawer(decltype(lineCallback) &cb) : cb(cb)
        {}

        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override
        {
            cb(btToVec3(from), btToVec3(to), btToVec3(color));
        }

        virtual void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override
        {}

        virtual void reportErrorWarning(const char *warningString) override
        {}

        virtual void draw3dText(const btVector3 &location, const char *textString) override
        {}

        virtual void setDebugMode(int debugMode) override
        {
            this->debugMode = debugMode;
        }

        virtual int getDebugMode() const override
        {
            return debugMode;
        }
    } drawer(lineCallback);

    bullet->world.setDebugDrawer(&drawer);
    int mode = 0;
    if (Game::settings.graphics.debugColliders)
        mode |= btIDebugDraw::DBG_DrawWireframe;
    if (Game::settings.graphics.debugColliderAABBs)
        mode |= btIDebugDraw::DBG_DrawAabb;
    if (Game::settings.graphics.debugConstraints)
        mode |= btIDebugDraw::DBG_DrawConstraints;
    if (Game::settings.graphics.debugConstraintLimits)
        mode |= btIDebugDraw::DBG_DrawConstraintLimits;
    bullet->world.getDebugDrawer()->setDebugMode(mode);
    bullet->world.debugDrawWorld();
    bullet->world.setDebugDrawer(NULL);
}

PhysicsSystem::~PhysicsSystem()
{
    delete bullet;
}

void PhysicsSystem::update(double deltaTime, EntityEngine* room)
{
    room->entities.view<BoxColliderShape>().each([&](auto e, BoxColliderShape &shape) {
        if (!shape.anyDirty()) return;
        assert(shape.bt);

        readdBodyForChange(room->entities, e, bullet, [&] {
            shape.bt->setImplicitShapeDimensions(vec3ToBt(shape.halfExtents));
        });
        shape.undirtAll();
    });
    room->entities.view<SphereColliderShape>().each([&](auto e, SphereColliderShape &shape) {
        if (!shape.anyDirty()) return;
        assert(shape.bt);

        shape.radius = max(0.f, shape.radius);

        readdBodyForChange(room->entities, e, bullet, [&] {
            shape.bt->setUnscaledRadius(shape.radius);
        });

        shape.undirtAll();
    });
    room->entities.view<CapsuleColliderShape>().each([&](auto e, CapsuleColliderShape &shape) {
        if (!shape.anyDirty()) return;
        assert(shape.bt);

        shape.sphereDistance = max(0.f, shape.sphereDistance);
        shape.sphereRadius = max(0.f, shape.sphereRadius);

        onCapsuleRemoved(room->entities, e);
        onCapsuleAdded(room->entities, e);

        shape.undirtAll();
    });
    /*
    room->entities.view<ConvexColliderShape, Collider>().each([&](auto e, ConvexColliderShape &shape, Collider &collider) {
        if (!shape.anyDirty()) return;

        if (shape.dirty<&ConvexColliderShape::meshName>())
        {
            onConvexRemoved(room->entities, e);
            onConvexAdded(room->entities, e);
        }

        wakeCollidersRigidBody(collider, room->entities);
        shape.undirtAll();
    });
    room->entities.view<ConcaveColliderShape, Collider>().each([&](auto e, ConcaveColliderShape &shape, Collider &collider) {
        if (!shape.anyDirty()) return;

        if (shape.dirty<&ConcaveColliderShape::meshName>())
        {
            onConcaveRemoved(room->entities, e);
            onConcaveAdded(room->entities, e);
        }

        wakeCollidersRigidBody(collider, room->entities);
        shape.undirtAll();
    });
    */

    {
        gu::profiler::Zone z("bullet");
        bullet->world.stepSimulation(updateFrequency ? 1.f / updateFrequency : deltaTime);
    }

    room->entities.view<Transform, RigidBody>().each([&](auto e, Transform &transform, RigidBody &body) {

        assert(body.bt);
        auto &btTrans = body.bt->getWorldTransform();

        // copy body transform to entity transform:
        if (body.allowExternalTransform)
        {
            auto posDiff = transform.position - body.prevPosition;
            auto rotDiff = transform.rotation * inverse(body.prevRotation);
            bool rotDirty = transform.rotation != body.prevRotation;

            transform.position = btToVec3(btTrans.getOrigin()) + posDiff;
            transform.rotation = rotDiff * btToQuat(btTrans.getRotation());

            if (posDiff != mu::ZERO_3)
            {
                btTrans.setOrigin(vec3ToBt(transform.position));
                body.bt->activate();
            }
            if (rotDirty)
            {
                btTrans.setRotation(quatToBt(transform.rotation));
                body.bt->activate();
            }
            body.prevPosition = transform.position;
            body.prevRotation = transform.rotation;
        }
        else
        {
            transform.position = btToVec3(btTrans.getOrigin());
            transform.rotation = btToQuat(btTrans.getRotation());
        }

        // sync component settings with bullet:
        if (body.collider.dirty<&Collider::collisionCategoryBits>() || body.collider.dirty<&Collider::collideWithMaskBits>())
        {
            bullet->world.removeRigidBody(body.bt);
            addRigidBody(bullet, &body);
        }
        else syncRigidBody(body, bullet);
    });
}

void PhysicsSystem::onRigidBodyRemoved(entt::registry &reg, entt::entity e)
{
    auto &body = reg.get<RigidBody>(e);
    assert(body.bt);

    bullet->world.removeRigidBody(body.bt);

    delete body.bt->getMotionState();
    delete body.bt;
    body.bt = NULL;
}

void PhysicsSystem::onRigidBodyAdded(entt::registry &reg, entt::entity e)
{
    auto &body = reg.get<RigidBody>(e);
    
    if (body.bt)
        throw gu_err("This rigid body already has a bullet instance.");

    btTransform btTrans = btTransform::getIdentity();
    if (auto transform = reg.try_get<Transform>(e))
    {
        btTrans.setOrigin(vec3ToBt(transform->position));
        btTrans.setRotation(quatToBt(transform->rotation));

        body.prevPosition = transform->position;
        body.prevRotation = transform->rotation;
    }

    body.bt = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
        body.mass,
        new btDefaultMotionState(btTrans),
        &bullet->emptyShape
    ));
    body.bt->setUserIndex(int(e));

    if (reg.has<BoxColliderShape>(e))
        onBoxAdded(reg, e);
    else if (reg.has<SphereColliderShape>(e))
        onSphereAdded(reg, e);
    else if (reg.has<CapsuleColliderShape>(e))
        onCapsuleAdded(reg, e);
    else if (reg.has<ConvexColliderShape>(e))
        onConvexAdded(reg, e);
    else if (reg.has<ConcaveColliderShape>(e))
        onConcaveAdded(reg, e);
    else
        addRigidBody(bullet, &body);
}

void onShapeAdded(entt::registry &reg, entt::entity e, btCollisionShape *shape, BulletStuff *bullet)
{
    auto rigidBody = reg.try_get<RigidBody>(e);
    if (!rigidBody) return;

    assert(rigidBody->bt);
    readdBodyForChange(reg, e, bullet, [&] {
        rigidBody->bt->setCollisionShape(shape);
    });
}

void PhysicsSystem::onBoxAdded(entt::registry &reg, entt::entity e)
{
    auto &box = reg.get<BoxColliderShape>(e);
    if (!box.bt)
        box.bt = new btBoxShape(vec3ToBt(box.halfExtents));

    onShapeAdded(reg, e, box.bt, bullet);
}

void PhysicsSystem::onSphereAdded(entt::registry &reg, entt::entity e)
{
    auto &sphere = reg.get<SphereColliderShape>(e);
    if (!sphere.bt)
        sphere.bt = new btSphereShape(sphere.radius);

    onShapeAdded(reg, e, sphere.bt, bullet);
}

void PhysicsSystem::onCapsuleAdded(entt::registry &reg, entt::entity e)
{
    auto &cap = reg.get<CapsuleColliderShape>(e);
    if (!cap.bt)
        cap.bt = new btCapsuleShape(cap.sphereRadius, cap.sphereDistance);

    onShapeAdded(reg, e, cap.bt, bullet);
}

void PhysicsSystem::onConvexAdded(entt::registry &reg, entt::entity e)
{
    auto &convex = reg.get<ConvexColliderShape>(e);
    /*
    
    if (!convex.shapeReact)
    {
        auto it = reactConvexMeshes.find(convex.meshName);
        if (it != reactConvexMeshes.end())
        {
            convex.shapeReact = it->second;
            onShapeAdded(reg, e, convex.shapeReact);
        }
    }
    */
}

void PhysicsSystem::onConcaveAdded(entt::registry &reg, entt::entity e)
{
    auto &concave = reg.get<ConcaveColliderShape>(e);
    /*
    
    if (!concave.shapeReact)
    {
        auto it = reactConcaveMeshes.find(concave.meshName);
        if (it != reactConcaveMeshes.end())
        {
            concave.shapeReact = it->second;
            onShapeAdded(reg, e, concave.shapeReact);
        }
    }
    */
}

void onShapeRemoved(entt::registry &reg, entt::entity e, btCollisionShape *shape, BulletStuff *bullet)
{
    auto rigidBody = reg.try_get<RigidBody>(e);
    if (!rigidBody) return;

    assert(rigidBody->bt);

    if (rigidBody->bt->getCollisionShape() == shape)
    {
        readdBodyForChange(reg, e, bullet, [&] {
            rigidBody->bt->setCollisionShape(&bullet->emptyShape);
        });
        rigidBody->bt->activate();
    }
    
}

void PhysicsSystem::onBoxRemoved(entt::registry &reg, entt::entity e)
{
    auto &box = reg.get<BoxColliderShape>(e);
    if (box.bt == NULL) return;
    onShapeRemoved(reg, e, box.bt, bullet);
    delete box.bt;
    box.bt = NULL;
}

void PhysicsSystem::onSphereRemoved(entt::registry &reg, entt::entity e)
{
    auto &sphere = reg.get<SphereColliderShape>(e);
    if (sphere.bt == NULL) return;
    onShapeRemoved(reg, e, sphere.bt, bullet);
    delete sphere.bt;
    sphere.bt = NULL;
}

void PhysicsSystem::onCapsuleRemoved(entt::registry &reg, entt::entity e)
{
    auto &capsule = reg.get<CapsuleColliderShape>(e);
    if (capsule.bt == NULL) return;
    onShapeRemoved(reg, e, capsule.bt, bullet);
    delete capsule.bt;
    capsule.bt = NULL;
}

void PhysicsSystem::onConvexRemoved(entt::registry &reg, entt::entity e)
{
    auto &convex = reg.get<ConvexColliderShape>(e);
    /*
    if (convex.shapeReact == NULL) return;
    onShapeRemoved(reg, e, convex.shapeReact);
    convex.shapeReact = NULL;
    */
}

void PhysicsSystem::onConcaveRemoved(entt::registry &reg, entt::entity e)
{
    auto &concave = reg.get<ConcaveColliderShape>(e);
    /*
    if (concave.shapeReact == NULL) return;
    onShapeRemoved(reg, e, concave.shapeReact);
    concave.shapeReact = NULL;
    */
}
