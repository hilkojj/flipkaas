
#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include <ecs/systems/EntitySystem.h>
#include <reactphysics3d/reactphysics3d.h>
#include "../../level/room/Room3D.h"

class PhysicsSystem : public EntitySystem
{
public:
    using EntitySystem::EntitySystem;

    reactphysics3d::PhysicsWorld *getReactWorld()
    {
        return reactWorld;
    }

protected:
    void init(EntityEngine* engine) override;

    void update(double deltaTime, EntityEngine* engine) override;

private:
    reactphysics3d::PhysicsCommon reactCommon;
    reactphysics3d::PhysicsWorld *reactWorld = NULL;

    void onRigidBodyRemoved(entt::registry &, entt::entity);
    void onRigidBodyAdded(entt::registry &, entt::entity);

    void onBoxAdded(entt::registry &, entt::entity);
    void onSphereAdded(entt::registry &, entt::entity);
    void onCapsuleAdded(entt::registry &, entt::entity);
    void onConvexAdded(entt::registry &, entt::entity);
    void onConcaveAdded(entt::registry &, entt::entity);

    void onBoxRemoved(entt::registry &, entt::entity);
    void onSphereRemoved(entt::registry &, entt::entity);
    void onCapsuleRemoved(entt::registry &, entt::entity);
    void onConvexRemoved(entt::registry &, entt::entity);
    void onConcaveRemoved(entt::registry &, entt::entity);

    void onColliderRemoved(entt::registry &, entt::entity);
    void onColliderAdded(entt::registry &, entt::entity);
};

#endif
