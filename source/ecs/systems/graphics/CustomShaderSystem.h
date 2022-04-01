
#ifndef CUSTOMSHADER_SYSTEM_H
#define CUSTOMSHADER_SYSTEM_H

#include <ecs/systems/EntitySystem.h>
#include <map>
#include <set>
#include "../../../level/room/Room3D.h"
#include "../../../generated/Model.hpp"

class CustomShaderSystem : public EntitySystem
{
  public:
    using EntitySystem::EntitySystem;

    typedef std::map<std::shared_ptr<ShaderAsset>, std::set<entt::entity>> EntitiesPerShader;

    static const EntitiesPerShader &getEntitiesPerShaderForRoom(Room3D *);

  protected:
    Room3D *room = NULL;

    void init(EntityEngine *) override;

    void update(double deltaTime, EntityEngine *) override;

    virtual ~CustomShaderSystem() override;

  private:
    
    void onComponentAdded(entt::registry &, entt::entity);
    void onComponentRemoved(entt::registry &, entt::entity);

};

#endif
