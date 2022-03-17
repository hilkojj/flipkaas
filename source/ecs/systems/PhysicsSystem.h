
#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include <ecs/systems/EntitySystem.h>
#include <reactphysics3d/reactphysics3d.h>
#include <forward_list>
#include "../../level/room/Room3D.h"

class PhysicsSystem : public EntitySystem
{
public:
    using EntitySystem::EntitySystem;

    reactphysics3d::PhysicsWorld *getReactWorld()
    {
        return reactWorld;
    }

    bool loadColliderMeshesFromGLTF(const char *path, bool force = false, bool convex = false);

protected:
    void init(EntityEngine* engine) override;

    void update(double deltaTime, EntityEngine* engine) override;

    virtual ~PhysicsSystem();

private:
    reactphysics3d::PhysicsCommon reactCommon;
    reactphysics3d::PhysicsWorld *reactWorld = NULL;
    std::unordered_map<std::string, long> modelFileLoadTime;
    std::unordered_map<std::string, SharedMesh> colliderMeshes;
    std::unordered_map<std::string, reactphysics3d::ConcaveMeshShape *> reactConcaveMeshes;
    std::unordered_map<std::string, reactphysics3d::ConvexMeshShape *> reactConvexMeshes;
    std::vector<reactphysics3d::TriangleVertexArray *> reactTris;
    std::vector<reactphysics3d::PolygonVertexArray *> reactPolys;
    std::forward_list<std::vector<reactphysics3d::PolygonVertexArray::PolygonFace>> polygonFaces;

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
