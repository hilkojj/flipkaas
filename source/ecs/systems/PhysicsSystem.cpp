#include "PhysicsSystem.h"
#include "../../generated/Physics.hpp"
#include "../../game/Game.h"
#include <utils/gltf_model_loader.h>
#include <btBulletCollisionCommon.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <stdio.h>
#include <stdlib.h>

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

    std::map<std::string, btConvexHullShape *> convexMeshes;
    std::map<std::string, btConcaveShape *> concaveMeshes;
    std::vector<btTriangleMesh *> concaveMeshInterfaces;

    BulletStuff()
        :
        dispatcher(&collisionConfig),
        world(&dispatcher, &overlappingPairCache, &solver, &collisionConfig)
    {
        world.setGravity(btVector3(0, 0, 0));
        world.getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback);
    }

    ~BulletStuff()
    {
        for (auto &[name, ptr] : convexMeshes)
            delete ptr;
        for (auto &[name, ptr] : concaveMeshes)
            delete ptr;
        for (auto ptr : concaveMeshInterfaces)
            delete ptr;
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

    if (collider.dirty<&Collider::registerCollisions>())
    {
        obj->setUserIndex2(collider.registerCollisions);
    }

    // if (collider.dirty<&Collider::bodyOffsetTranslation>() || collider.dirty<&Collider::bodyOffsetRotation>())
    // {
        // TODO: use btCompoundShape for this
        // https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=9546
    // }
    collider.undirtAll();
}

void syncGhostBody(GhostBody &body, BulletStuff *bullet)
{
    assert(body.bt);
    if (body.collider.anyDirty())
        syncCollider(body.bt, body.collider);

    if (!body.anyDirty())
        return;

    // something is dirty
    //body.bt->activate();

    body.undirtAll();
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
        if (dynamic_cast<btTriangleMeshShape *>(body.bt->getCollisionShape()))
        {
            body.mass = 0;
        }
        else
        {
            if (!dynamic_cast<btEmptyShape *>(body.bt->getCollisionShape()))
                body.bt->getCollisionShape()->calculateLocalInertia(body.mass, inertia);
        }
        body.bt->setMassProps(body.mass, inertia);
    }

    if (body.dirty<&RigidBody::angularAxisFactor>())
        body.bt->setAngularFactor(vec3ToBt(body.angularAxisFactor));
    if (body.dirty<&RigidBody::linearAxisFactor>())
        body.bt->setLinearFactor(vec3ToBt(body.linearAxisFactor));

    body.undirtAll();
}

void addGhostBody(BulletStuff *bullet, GhostBody *gb)
{
    bullet->world.addCollisionObject(gb->bt, gb->collider.collisionCategoryBits, gb->collider.collideWithMaskBits);
    gb->bedirtAll();
    gb->collider.bedirtAll();
    syncGhostBody(*gb, bullet);
    gb->bt->activate();
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

    updateFrequency = 60;

    engine->entities.on_destroy<RigidBody>()				.connect<&PhysicsSystem::onRigidBodyRemoved>(this);
    engine->entities.on_replace<RigidBody>()				.connect<&PhysicsSystem::onRigidBodyRemoved>(this);
    engine->entities.on_construct<RigidBody>()				.connect<&PhysicsSystem::onRigidBodyAdded>(this);

    engine->entities.on_destroy<GhostBody>()                .connect<&PhysicsSystem::onGhostBodyRemoved>(this);
    engine->entities.on_replace<GhostBody>()                .connect<&PhysicsSystem::onGhostBodyRemoved>(this);
    engine->entities.on_construct<GhostBody>()              .connect<&PhysicsSystem::onGhostBodyAdded>(this);

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


PhysicsSystem::PhysicsSystem(std::string name) : EntitySystem(name)
{
    bullet = new BulletStuff;
}

bool PhysicsSystem::loadColliderMeshesFromObj(const char *path, bool convex)
{
    FILE *file = fopen(path, "r");
    if (!file)
        throw gu_err("File " + std::string(path) + " cannot be opened!");
    
    char objName[128];
    btConvexHullShape *convexShape = NULL;
    btTriangleMesh *concaveMeshInterface = NULL;
    int nrOfVertsBeforeThisObj = 0;
    int nrOfVertsInThisObj = 0;

    while (true)
    {

        char lineHeader[128] = { 0 };

        // read the first word of the line
        int res = fscanf(file, "%127s", lineHeader);
        bool eof = res == EOF;
        bool newObj = !eof && strcmp(lineHeader, "o") == 0;

        if (eof || newObj)
        {
            if (concaveMeshInterface != NULL)
            {
                bullet->concaveMeshes[objName] = new btBvhTriangleMeshShape(concaveMeshInterface, true, true);
                bullet->concaveMeshInterfaces.push_back(concaveMeshInterface);
                
            }   

            nrOfVertsBeforeThisObj += nrOfVertsInThisObj;
            nrOfVertsInThisObj = 0;
        }

        if (eof)
            break; // EOF = End Of File. Quit the loop.

        if (newObj && fscanf(file, " %127s\n", &objName))
        {
            if (convex)
            {
                convexShape = new btConvexHullShape;
                bullet->convexMeshes[objName] = convexShape;
            }
            else
            {
                concaveMeshInterface = new btTriangleMesh;
            }
        }
        else if (strcmp(lineHeader, "v") == 0)
        {
            vec3 vert;
            if (fscanf(file, " %f %f %f\n", &vert.x, &vert.y, &vert.z) == 3)
            {
                nrOfVertsInThisObj++;
                if (convexShape)
                    convexShape->addPoint(vec3ToBt(vert));
                else
                    concaveMeshInterface->findOrAddVertex(vec3ToBt(vert), false);
            }
        }
        else if (!convexShape && strcmp(lineHeader, "f") == 0)
        {
            ivec4 indices;
            int scan = fscanf(file, " %d %d %d %d\n", &indices.x, &indices.y, &indices.z, &indices.w);
            if (scan == 4)
                throw gu_err("Concave collider meshes must be triangulated! File: " + std::string(path));
            if (scan == 3)
            {
                if (concaveMeshInterface->getIndexedMeshArray().size() == 0)
                    throw gu_err("Object '" + std::string(objName) + "' has a face but no vertices (?). File: " + std::string(path));

                indices -= nrOfVertsBeforeThisObj;
                indices -= 1; // .obj starts counting at 1.

                if (any(lessThan(ivec3(indices), ivec3(0))))
                {
                    throw gu_err("Object '" + std::string(objName) + "' has a face with one or more indices referencing vertices from a previous object. File: " + std::string(path));
                }

                if (any(greaterThanEqual(ivec3(indices), ivec3(concaveMeshInterface->getIndexedMeshArray().at(0).m_numVertices))))
                {
                    throw gu_err("Object '" + std::string(objName) + "' has a face with one or more indices > number of verticies. File: " + std::string(path));
                }
                concaveMeshInterface->addTriangleIndices(indices.x, indices.y, indices.z);
            }
        }
    }
    fclose(file);
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
    room->entities.view<ConvexColliderShape>().each([&](auto e, ConvexColliderShape &shape) {
        if (shape.undirt<&ConvexColliderShape::meshName>())
        {
            onConvexRemoved(room->entities, e);
            onConvexAdded(room->entities, e);
        }
    });
    room->entities.view<ConcaveColliderShape>().each([&](auto e, ConcaveColliderShape &shape) {
        if (shape.undirt<&ConcaveColliderShape::meshName>())
        {
            onConcaveRemoved(room->entities, e);
            onConcaveAdded(room->entities, e);
        }
    });
    
    {
        gu::profiler::Zone z("bullet");
        bullet->world.stepSimulation(updateFrequency ? 1.f / updateFrequency : deltaTime);
    }



    {
        std::map<std::pair<entt::entity, entt::entity>, std::shared_ptr<Collision>> toEmit;

        int nrOfManifolds = bullet->dispatcher.getNumManifolds();

        for (int i = 0; i < nrOfManifolds; i++)
        {
            auto *manifold = bullet->dispatcher.getManifoldByIndexInternal(i);
            if (manifold->getNumContacts() <= 0)
                continue;

            const btCollisionObject *obj[2] {
                manifold->getBody0(),
                manifold->getBody1()
            };

            bvec2 objRegisters(obj[0]->getUserIndex2(), obj[1]->getUserIndex2());

            if (!any(objRegisters))
                continue;

            entt::entity e[2]{
                entt::entity(obj[0]->getUserIndex()),
                entt::entity(obj[1]->getUserIndex())
            };

            if (!room->entities.valid(e[0]) || !room->entities.valid(e[1]))
            {
                std::cerr << "Collision manifold between one or two invalid entities: " << int(e[0]) << " & " << int(e[1]) << std::endl;
                continue;
            }

            Collider *colliders[2];
            for (int j = 0; j < 2; j++)
            {
                switch (obj[j]->getInternalType())
                {
                    case btCollisionObject::CollisionObjectTypes::CO_RIGID_BODY:
                        colliders[j] = &room->entities.get<RigidBody>(e[j]).collider;
                        break;
                    case btCollisionObject::CollisionObjectTypes::CO_GHOST_OBJECT:
                        colliders[j] = &room->entities.get<GhostBody>(e[j]).collider;
                        break;
                    default:
                        throw gu_err("Collision object type not implemented!");
                        break;
                }
            }
            //assert(objRegisters[0] == colliders[0]->registerCollisions);  fails because this is synced after this.
            //assert(objRegisters[1] == colliders[1]->registerCollisions);

            for (int j = 0; j < 2; j++)
            {
                if (!objRegisters[j])
                    continue;

                auto otherE = e[j ^ 1];

                std::shared_ptr<Collision> col;

                auto it = colliders[j]->collisions.find(otherE);
                if (it == colliders[j]->collisions.end())
                {
                    col = colliders[j]->collisions[otherE] = std::make_shared<Collision>();
                    col->otherEntity = otherE;

                    // emit event
                    toEmit[{e[j], otherE}] = col;
                }
                else
                {
                    col = it->second;
                    col->duration += deltaTime;
                    col->hasEndedTest = false;
                }
                col->contactPoints.clear(); // todo: could be more manifolds???

                for (int p = 0; p < manifold->getNumContacts(); p++)
                {
                    const btManifoldPoint &pt = manifold->getContactPoint(p);
                    
                    col->contactPoints.push_back(btToVec3(j == 0 ? pt.getPositionWorldOnA() : pt.getPositionWorldOnB()));
                    // todo: lots of interesting stuff like normals
                }
            }
        }

        for (auto &[pair, col] : toEmit)
        {
            if (room->entities.valid(pair.first) && room->entities.valid(pair.second))
                room->emitEntityEvent<Collision>(pair.first, *col.get());
        }
    }

    std::map<std::pair<entt::entity, entt::entity>, std::shared_ptr<Collision>> collisionsEndedToEmit;

    auto updateColliderCollisions = [&](entt::entity e, Collider &collider) {
        if (collider.registerCollisions)
        {
            std::vector<entt::entity> endedCollisions;
            for (auto &[otherE, col] : collider.collisions)
            {
                if (col->hasEndedTest)
                {
                    collisionsEndedToEmit[{e, otherE}] = col;
                    endedCollisions.push_back(otherE);
                }
                col->hasEndedTest = true;
            }
            for (auto endedCol : endedCollisions)
                collider.collisions.erase(endedCol);
        }
    };

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

        // sync component settings with bullet: // TODO: this should probably also work without Transform.
        if (body.collider.dirty<&Collider::collisionCategoryBits>() || body.collider.dirty<&Collider::collideWithMaskBits>())
        {
            bullet->world.removeRigidBody(body.bt);
            addRigidBody(bullet, &body);
        }
        else syncRigidBody(body, bullet);

        updateColliderCollisions(e, body.collider);
    });

    room->entities.view<Transform, GhostBody>().each([&](auto e, Transform &transform, GhostBody &body) {

        assert(body.bt);
        // sync transform:
        auto &btTrans = body.bt->getWorldTransform();
        btTrans.setOrigin(vec3ToBt(transform.position));
        btTrans.setRotation(quatToBt(transform.rotation));

        // sync component settings with bullet:
        if (body.collider.dirty<&Collider::collisionCategoryBits>() || body.collider.dirty<&Collider::collideWithMaskBits>())
        {
            bullet->world.removeCollisionObject(body.bt);
            addGhostBody(bullet, &body);
        }
        else syncGhostBody(body, bullet);

        updateColliderCollisions(e, body.collider);
    });

    for (auto &[pair, col] : collisionsEndedToEmit)
    {
        if (room->entities.valid(pair.first))// && room->entities.valid(pair.second))
            room->emitEntityEvent<Collision>(pair.first, *col.get(), "CollisionEnded");
    }
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
    reg.remove_if_exists<GhostBody>(e);

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

void PhysicsSystem::onGhostBodyRemoved(entt::registry &reg, entt::entity e)
{
    auto &body = reg.get<GhostBody>(e);
    assert(body.bt);

    bullet->world.removeCollisionObject(body.bt);

    delete body.bt;
    body.bt = NULL;
}

void PhysicsSystem::onGhostBodyAdded(entt::registry &reg, entt::entity e)
{
    reg.remove_if_exists<RigidBody>(e);

    auto &body = reg.get<GhostBody>(e);

    if (body.bt)
        throw gu_err("This ghost body already has a bullet instance.");

    body.bt = new btGhostObject;
    body.bt->setUserIndex(int(e));
    body.bt->setCollisionShape(&bullet->emptyShape);
    body.bt->setCollisionFlags(body.bt->getCollisionFlags() | btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);

    btTransform btTrans = btTransform::getIdentity();
    if (auto transform = reg.try_get<Transform>(e))
    {
        btTrans.setOrigin(vec3ToBt(transform->position));
        btTrans.setRotation(quatToBt(transform->rotation));
        body.bt->setWorldTransform(btTrans);
    }

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
        addGhostBody(bullet, &body);
}

void onShapeAdded(entt::registry &reg, entt::entity e, btCollisionShape *shape, BulletStuff *bullet)
{
    if (auto rigidBody = reg.try_get<RigidBody>(e))
    {
        assert(rigidBody->bt);
        readdBodyForChange(reg, e, bullet, [&] {
            rigidBody->bt->setCollisionShape(shape);
        });
    }
    else if (auto ghostBody = reg.try_get<GhostBody>(e))
    {
        assert(ghostBody->bt);
        bullet->world.removeCollisionObject(ghostBody->bt);
        ghostBody->bt->setCollisionShape(shape);
        addGhostBody(bullet, ghostBody);
    }
    
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

    auto it = bullet->convexMeshes.find(convex.meshName);

    if (it != bullet->convexMeshes.end())
    {
        convex.bt = it->second;
        onShapeAdded(reg, e, convex.bt, bullet);
    }
}

void PhysicsSystem::onConcaveAdded(entt::registry &reg, entt::entity e)
{
    auto &concave = reg.get<ConcaveColliderShape>(e);
    
    auto it = bullet->concaveMeshes.find(concave.meshName);

    if (it != bullet->concaveMeshes.end())
    {
        concave.bt = it->second;
        onShapeAdded(reg, e, concave.bt, bullet);
    }
}

void onShapeRemoved(entt::registry &reg, entt::entity e, btCollisionShape *shape, BulletStuff *bullet)
{
    if (auto rigidBody = reg.try_get<RigidBody>(e))
    {
        assert(rigidBody->bt);

        if (rigidBody->bt->getCollisionShape() == shape)
        {
            readdBodyForChange(reg, e, bullet, [&] {
                rigidBody->bt->setCollisionShape(&bullet->emptyShape);
             });
            rigidBody->bt->activate();
        }
    }
    else if (auto ghostBody = reg.try_get<GhostBody>(e))
    {
        assert(ghostBody->bt);
        if (ghostBody->bt->getCollisionShape() == shape)
        {
            bullet->world.removeCollisionObject(ghostBody->bt);
            ghostBody->bt->setCollisionShape(&bullet->emptyShape);
            addGhostBody(bullet, ghostBody);
        }
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
    if (convex.bt == NULL) return;
    onShapeRemoved(reg, e, convex.bt, bullet);
    convex.bt = NULL;
}

void PhysicsSystem::onConcaveRemoved(entt::registry &reg, entt::entity e)
{
    auto &concave = reg.get<ConcaveColliderShape>(e);
    if (concave.bt == NULL) return;
    onShapeRemoved(reg, e, concave.bt, bullet);
    concave.bt = NULL;
}
