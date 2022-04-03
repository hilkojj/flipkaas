
#ifndef GRAVITY_SYSTEM_H
#define GRAVITY_SYSTEM_H

#include <ecs/systems/EntitySystem.h>
#include <functional>
#include "../../level/room/Room3D.h"

struct GhostBody;
struct GravityField;

class GravitySystem : public EntitySystem
{
public:
    using EntitySystem::EntitySystem;

protected:
    Room3D *room = NULL;

    void init(EntityEngine *) override;

    void update(double deltaTime, EntityEngine *) override;

    void forEachVictim(entt::entity fieldE, const GravityField &, const GhostBody &, std::function<vec3(entt::entity, const Transform &)>);

};

#endif
