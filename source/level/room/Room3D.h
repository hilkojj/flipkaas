
#ifndef GAME_ROOM3D_H
#define GAME_ROOM3D_H

#include <level/room/Room.h>
#include <graphics/camera.h>
#include <graphics/3d/model.h>
#include "../../generated/Transform.hpp"
#include "../../rendering/level/room/EnvironmentMap.h"

struct PhysicsSystem;

class Room3D : public Room
{
    std::unordered_map<std::string, long> modelFileLoadTime;
    VertBuffer
        *uploadingTo = NULL,
        *uploadingRiggedTo = NULL;

    PhysicsSystem *physics = NULL;

  public:

    struct ModelInstances
    {
        std::vector<SharedMesh> meshes; // keep a reference to the meshes, to keep the vertBuffers alive, so that the pointers in the following map stay valid.
        std::map<VertBuffer *, int> vertDataIdPerBuffer;

        VertData transforms = VertData(VertAttributes()
            .add_(VertAttributes::TRANSFORM_COL_A)
            .add_(VertAttributes::TRANSFORM_COL_B)
            .add_(VertAttributes::TRANSFORM_COL_C)
            .add_(VertAttributes::TRANSFORM_COL_D), std::vector<unsigned char>());
        
        ~ModelInstances();
    };

    Camera *camera = NULL;
    entt::entity cameraEntity = entt::null;

    VertAttributes loadedMeshAttributes, loadedRiggedMeshAttributes;
    std::unordered_map<std::string, SharedModel> models;

    asset<EnvironmentMap> environmentMap;

    Room3D();

    void update(double deltaTime) override;

    vec3 getPosition(entt::entity) const override;

    void setPosition(entt::entity, const vec3 &) override;

    PhysicsSystem &getPhysics();

    Camera *cameraFromEntity(entt::entity) const;

    static mat4 transformFromComponent(const Transform &);

    virtual void toJson(json &) override;

    virtual void fromJson(const json &) override;

    ~Room3D() override;

  protected:
    void initializeLuaEnvironment() override;

    void updateOrCreateCamera(double deltaTime);

    bool loadModels(const char *path, bool force, VertBuffer **, const VertAttributes &);

};


#endif
