
#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include <functional>
#include <ecs/systems/EntitySystem.h>
#include "../../level/room/Room3D.h"

struct BulletStuff;

using RayHitCallback = std::function<void(entt::entity, const vec3 &hitPoint, const vec3 &normal)>;

class PhysicsSystem : public EntitySystem
{
public:
    using EntitySystem::EntitySystem;

    bool loadColliderMeshesFromGLTF(const char *path, bool force = false, bool convex = false);

    void rayTest(const vec3 &from, const vec3 &to, const RayHitCallback &, int mask=-1, int category=-1);

    void debugDraw(const std::function<void(const vec3 &a, const vec3 &b, const vec3 &color)> &lineCallback);

protected:
    void init(EntityEngine* engine) override;

    void update(double deltaTime, EntityEngine* engine) override;

    virtual ~PhysicsSystem();

private:

    Room3D *room = NULL;
    BulletStuff *bullet = NULL;

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
};

#endif
