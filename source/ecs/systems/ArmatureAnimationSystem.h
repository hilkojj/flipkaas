
#ifndef ARMATURE_ANIMATION_SYSTEM_H
#define ARMATURE_ANIMATION_SYSTEM_H

#include <ecs/systems/EntitySystem.h>

class ArmatureAnimationSystem : public EntitySystem
{
  public:
    using EntitySystem::EntitySystem;

  protected:
    void update(double deltaTime, EntityEngine *engine) override;

};

#endif
