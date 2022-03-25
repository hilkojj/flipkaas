
#ifndef GRAVITY_SYSTEM_H
#define GRAVITY_SYSTEM_H

#include <ecs/systems/EntitySystem.h>

class GravitySystem : public EntitySystem
{
public:
    using EntitySystem::EntitySystem;

protected:
    void update(double deltaTime, EntityEngine *engine) override;

};

#endif
