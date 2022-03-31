
#ifndef POSITIONEDAUDIO_SYSTEM_H
#define POSITIONEDAUDIO_SYSTEM_H

#include <ecs/systems/EntitySystem.h>
#include "../../level/room/Room3D.h"

class PositionedAudioSystem : public EntitySystem
{
  public:
    using EntitySystem::EntitySystem;

  protected:
    Room3D *room = NULL;

    void init(EntityEngine *) override;

    void update(double deltaTime, EntityEngine *) override;

  private:
    
    void onComponentAdded(entt::registry &, entt::entity);

};

#endif
