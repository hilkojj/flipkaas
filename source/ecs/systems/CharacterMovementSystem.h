
#ifndef CHARACTERMOVEMENT_SYSTEM_H
#define CHARACTERMOVEMENT_SYSTEM_H

#include <ecs/systems/EntitySystem.h>
#include "../../level/room/Room3D.h"

class CharacterMovementSystem : public EntitySystem
{
public:
    using EntitySystem::EntitySystem;

protected:
    Room3D *room = NULL;

    void init(EntityEngine *) override;

    void update(double deltaTime, EntityEngine *) override;

};

#endif
