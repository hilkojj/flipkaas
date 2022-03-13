#include "PhysicsSystem.h"
#include "../../generated/Physics.hpp"
#include "../../game/Game.h"
#include <set>

void PhysicsSystem::init(EntityEngine* engine)
{
    auto room = dynamic_cast<Room3D *>(engine);
    if (!room) throw gu_err("engine is not a room");
 
    auto logger = reactCommon.createDefaultLogger();
    uint logLevel = 0u;
    if (Game::settings.physicsSettings.logError)
        logLevel |= static_cast<uint>(reactphysics3d::Logger::Level::Error);
    if (Game::settings.physicsSettings.logWarning)
        logLevel |= static_cast<uint>(reactphysics3d::Logger::Level::Warning);
    if (Game::settings.physicsSettings.logInfo)
        logLevel |= static_cast<uint>(reactphysics3d::Logger::Level::Information);
    logger->addStreamDestination(std::cout, logLevel, reactphysics3d::DefaultLogger::Format::Text);
    reactCommon.setLogger(logger);

    reactphysics3d::PhysicsWorld::WorldSettings settings;
    settings.worldName = room->name + " (room index: " + std::to_string(room->getIndexInLevel()) + ")";
    //settings.gravity.setToZero();
    reactWorld = reactCommon.createPhysicsWorld(settings);

    updateFrequency = 60;

    std::cerr << "disable reactphysics debug mode in release builds" << std::endl;

    // todo: if system is removed, these functions should be disconnected:
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

    engine->entities.on_destroy<Collider>()					.connect<&PhysicsSystem::onColliderRemoved>(this);
    engine->entities.on_replace<Collider>()					.connect<&PhysicsSystem::onColliderRemoved>(this);
    engine->entities.on_construct<Collider>()				.connect<&PhysicsSystem::onColliderAdded>(this);

    // todo/note: replacing components is not supported by the lua api, however, if done from C++, things like colliders probably wont be added.
}

void transformToReact(const vec3 &translate, const quat &rotate, reactphysics3d::Transform &out)
{
    out.setPosition(reactphysics3d::Vector3(translate.x, translate.y, translate.z));
    out.setOrientation(reactphysics3d::Quaternion(rotate.x, rotate.y, rotate.z, rotate.w));
    if (!out.isValid())
    {
        out.setPosition(reactphysics3d::Vector3::zero());
        out.setOrientation(reactphysics3d::Quaternion::identity());
        std::cerr << "transformToReact warning: transform is not valid" << std::endl;
    }
}

void transformToReact(const Transform &t, reactphysics3d::Transform &out)
{
    transformToReact(t.position, t.rotation, out);
}


void reactToTransform(const reactphysics3d::Transform &t, Transform &out)
{
    auto &pos = t.getPosition();
    out.position.x = pos.x;
    out.position.y = pos.y;
    out.position.z = pos.z;
    auto &rot = t.getOrientation();
    out.rotation.x = rot.x;
    out.rotation.y = rot.y;
    out.rotation.z = rot.z;
    out.rotation.w = rot.w;
}

void syncReactRigidBody(RigidBody &body)
{
    if (!body.anyDirty())
        return;
    // something is dirty
    body.reactBody->setIsSleeping(false);	// enabling gravity for example has no effect if the body is sleeping.

    if (body.undirt<&RigidBody::gravity>())
        body.reactBody->enableGravity(body.gravity);
    if (body.undirt<&RigidBody::linearDamping>())
    {
        body.linearDamping = max(body.linearDamping, 0.f);
        body.reactBody->setLinearDamping(body.linearDamping);
    }
    if (body.undirt<&RigidBody::angularDamping>())
    {
        body.angularDamping = max(body.angularDamping, 0.f);
        body.reactBody->setAngularDamping(body.angularDamping);
    }
    if (body.undirt<&RigidBody::allowSleep>())
        body.reactBody->setIsAllowedToSleep(body.allowSleep);
    if (body.undirt<&RigidBody::dynamic>())
        body.reactBody->setType(body.dynamic ? reactphysics3d::BodyType::DYNAMIC : reactphysics3d::BodyType::STATIC);
    if (body.undirt<&RigidBody::mass>())
        body.reactBody->setMass(body.mass);
    if (body.undirt<&RigidBody::angularAxisFactor>())
        body.reactBody->setAngularLockAxisFactor(reactphysics3d::Vector3(body.angularAxisFactor.x, body.angularAxisFactor.y, body.angularAxisFactor.z));
    if (body.undirt<&RigidBody::linearAxisFactor>())
        body.reactBody->setLinearLockAxisFactor(reactphysics3d::Vector3(body.linearAxisFactor.x, body.linearAxisFactor.y, body.linearAxisFactor.z));

    body.undirtAll();
}

void PhysicsSystem::update(double deltaTime, EntityEngine* room)
{
    {
        gu::profiler::Zone z("ReactPhysics3D");
        reactWorld->update(deltaTime);
    }

    room->entities.view<Transform, RigidBody>().each([&](auto e, Transform &transform, RigidBody &body) {

        assert(body.reactBody);
        
        // copy body transform to entity transform:
        reactToTransform(body.reactBody->getTransform(), transform);
        
        // sync component settings with react:
        syncReactRigidBody(body);
    });

    room->entities.view<Collider>().each([&](auto e, Collider &collider) {
        
        if (collider.rigidBodyEntity != collider.prevRigidBodyEntity)
        {
            auto newRigidBody = collider.rigidBodyEntity;
            collider.rigidBodyEntity = collider.prevRigidBodyEntity;
            onColliderRemoved(room->entities, e);

            collider.rigidBodyEntity = newRigidBody;
            collider.prevRigidBodyEntity = newRigidBody;

            onColliderAdded(room->entities, e);
        }

    });
}

void PhysicsSystem::onRigidBodyRemoved(entt::registry &reg, entt::entity e)
{
    auto &body = reg.get<RigidBody>(e);
    
    if (body.reactBody)
    {
        for (int i = 0; i < body.reactBody->getNbColliders(); i++)
        {
            auto colliderEntity = entt::entity(reinterpret_cast<uintptr_t>(body.reactBody->getCollider(i)->getUserData()));
            if (!reg.valid(colliderEntity))
            {
                throw gu_err("Collider userdata did not give an valid entity");
            }
            if (!reg.has<Collider>(colliderEntity))
            {
                throw gu_err("Collider userdata gave an entity that does not have a Collider component");
            }
            auto &collider = reg.get<Collider>(colliderEntity);
            collider.reactCollider = NULL;
        }
        reactWorld->destroyRigidBody(body.reactBody);
    }
}

struct HasColliders
{
    std::set<entt::entity> colliders;
};

void PhysicsSystem::onRigidBodyAdded(entt::registry &reg, entt::entity e)
{
    reactphysics3d::Transform reactTransform;

    if (auto transform = reg.try_get<Transform>(e))
        transformToReact(*transform, reactTransform);

    auto &body = reg.get<RigidBody>(e);
    body.reactBody = reactWorld->createRigidBody(reactTransform);

    if (auto has = reg.try_get<HasColliders>(e))
    {
        for (auto collider : has->colliders)
        {
            onColliderAdded(reg, collider);
        }
    }
    body.bedirtAll();
    syncReactRigidBody(body);
}

void setColliderUserData(reactphysics3d::Collider *collider, entt::entity colliderEntity)
{
    uintptr_t e = uintptr_t(colliderEntity);
    collider->setUserData((void *)e);
}

void onShapeAdded(entt::registry &reg, entt::entity e, reactphysics3d::CollisionShape *shape)
{
    auto collider = reg.try_get<Collider>(e);
    if (!collider) return;
    if (!reg.valid(collider->rigidBodyEntity)) return;

    auto rigidBody = reg.try_get<RigidBody>(collider->rigidBodyEntity);
    if (!rigidBody) return;

    assert(rigidBody->reactBody);
    
    reactphysics3d::Transform reactTransform;
    transformToReact(collider->bodyOffsetTranslation, collider->bodyOffsetRotation, reactTransform);

    // remove older collider first:
    if (collider->reactCollider)
        rigidBody->reactBody->removeCollider(collider->reactCollider);

    collider->reactCollider = rigidBody->reactBody->addCollider(shape, reactTransform);
    setColliderUserData(collider->reactCollider, e);
}

void PhysicsSystem::onBoxAdded(entt::registry &reg, entt::entity e)
{
    auto &box = reg.get<BoxColliderShape>(e);
    if (!box.shapeReact)
        box.shapeReact = reactCommon.createBoxShape(reactphysics3d::Vector3(box.halfExtents.x, box.halfExtents.y, box.halfExtents.z));	// todo: half extents or full?

    onShapeAdded(reg, e, box.shapeReact);
}

void PhysicsSystem::onSphereAdded(entt::registry &reg, entt::entity e)
{
    auto &sphere = reg.get<SphereColliderShape>(e);
    if (!sphere.shapeReact)
        sphere.shapeReact = reactCommon.createSphereShape(sphere.radius);

    onShapeAdded(reg, e, sphere.shapeReact);
}

void PhysicsSystem::onCapsuleAdded(entt::registry &reg, entt::entity e)
{
    auto &cap = reg.get<CapsuleColliderShape>(e);
    if (!cap.shapeReact)
        cap.shapeReact = reactCommon.createCapsuleShape(cap.sphereRadius, cap.sphereDistance);

    onShapeAdded(reg, e, cap.shapeReact);
}

void PhysicsSystem::onConvexAdded(entt::registry &reg, entt::entity e)
{
}

void PhysicsSystem::onConcaveAdded(entt::registry &reg, entt::entity e)
{
}

void onShapeRemoved(entt::registry &reg, entt::entity e, reactphysics3d::CollisionShape *shape)
{
    // remove reactCollider if box is in use:
    auto collider = reg.try_get<Collider>(e);
    if (collider && collider->reactCollider && collider->reactCollider->getCollisionShape() == shape && reg.valid(collider->rigidBodyEntity))
    {
        auto rigidBody = reg.try_get<RigidBody>(collider->rigidBodyEntity);
        if (rigidBody)
        {
            assert(rigidBody->reactBody);
            rigidBody->reactBody->removeCollider(collider->reactCollider);
            collider->reactCollider = NULL;
        }
    }
}

void PhysicsSystem::onBoxRemoved(entt::registry &reg, entt::entity e)
{
    auto &box = reg.get<BoxColliderShape>(e);
    if (box.shapeReact == NULL) return;
    onShapeRemoved(reg, e, box.shapeReact);
    reactCommon.destroyBoxShape(box.shapeReact);
}

void PhysicsSystem::onSphereRemoved(entt::registry &reg, entt::entity e)
{
    auto &sphere = reg.get<SphereColliderShape>(e);
    if (sphere.shapeReact == NULL) return;
    onShapeRemoved(reg, e, sphere.shapeReact);
    reactCommon.destroySphereShape(sphere.shapeReact);
}

void PhysicsSystem::onCapsuleRemoved(entt::registry &reg, entt::entity e)
{
    auto &capsule = reg.get<CapsuleColliderShape>(e);
    if (capsule.shapeReact == NULL) return;
    onShapeRemoved(reg, e, capsule.shapeReact);
    reactCommon.destroyCapsuleShape(capsule.shapeReact);
}

void PhysicsSystem::onConvexRemoved(entt::registry &, entt::entity)
{
}

void PhysicsSystem::onConcaveRemoved(entt::registry &, entt::entity)
{
}

void PhysicsSystem::onColliderRemoved(entt::registry &reg, entt::entity e)
{
    auto &collider = reg.get<Collider>(e);
    if (reg.valid(collider.rigidBodyEntity))
    {
        if (auto has = reg.try_get<HasColliders>(collider.rigidBodyEntity))
        {
            has->colliders.erase(e);
        }
    }
    if (collider.reactCollider)
    {
        collider.reactCollider->getBody()->removeCollider(collider.reactCollider);
        collider.reactCollider = NULL;
    }
}

void PhysicsSystem::onColliderAdded(entt::registry &reg, entt::entity e)
{
    auto &collider = reg.get<Collider>(e);
    if (reg.valid(collider.rigidBodyEntity))
    {
        auto &has = reg.get_or_assign<HasColliders>(collider.rigidBodyEntity);
        has.colliders.insert(e);
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
}
