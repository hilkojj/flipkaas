
#ifndef GAME_ROOM3D_H
#define GAME_ROOM3D_H


#include <level/room/Room.h>
#include <graphics/camera.h>
#include <graphics/3d/model.h>
#include "../../generated/Transform.hpp"
#include "../../rendering/level/room/EnvironmentMap.h"

class Room3D : public Room
{
    std::unordered_map<std::string, long> modelFileLoadTime;
    VertBuffer
        *uploadingTo = NULL,
        *uploadingRiggedTo = NULL;

  public:

    Camera *camera = NULL;
    entt::entity cameraEntity = entt::null;

    VertAttributes loadedMeshAttributes, loadedRiggedMeshAttributes;
    std::unordered_map<std::string, SharedModel> models;

    asset<EnvironmentMap> environmentMap;

    Room3D();

    void update(double deltaTime) override;

    vec3 getPosition(entt::entity) const override;

    void setPosition(entt::entity, const vec3 &) override;

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
