
#ifndef GRAVITY_SYSTEM_H
#define GRAVITY_SYSTEM_H

#include <ecs/systems/EntitySystem.h>
#include <functional>
#include "../../level/room/Room3D.h"

struct GhostBody;
struct GravityField;
struct Room3D;

class GravitySystem : public EntitySystem
{
public:
    using EntitySystem::EntitySystem;

protected:
    Room3D *room = NULL;

    void init(EntityEngine *engine) override;

    void update(double deltaTime, EntityEngine *engine) override;

    void forEachVictim(const GravityField &, const GhostBody &, std::function<vec3(entt::entity, const Transform &)>);

};

#endif
