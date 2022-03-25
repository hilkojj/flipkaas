#include "GravitySystem.h"
#include "../../level/room/Room3D.h"
#include "../../generated/Gravity.hpp"
#include "../../generated/Physics.hpp"
#include "../../generated/Transform.hpp"

void GravitySystem::update(double deltaTime, EntityEngine *engine)
{

    engine->entities.view<Transform, GhostBody, GravityField, SphereGravityFunction>().each([&](auto e, const Transform &t, const GhostBody &ghost, GravityField &field, SphereGravityFunction &func) {

        
        for (auto &[victim, collision] : ghost.collider.collisions)
        {
            auto *affected = engine->entities.try_get<GravityFieldAffected>(victim);
            if (!affected)
                continue;

            if (affected->dirPerPriority.size() <= field.priority)
                affected->dirPerPriority.resize(field.priority + 1, vec3(0));

            affected->dirPerPriority[field.priority] += vec3(0, 1, 0);

        }


    });

    engine->entities.view<RigidBody, GravityFieldAffected>().each([&](auto e, RigidBody &body, GravityFieldAffected &affected) {
        
        vec3 newGravity(0);

        float leftToGive = 1;

        for (int prio = affected.dirPerPriority.size() - 1; prio >= 0; prio--)
        {
            newGravity += leftToGive * affected.dirPerPriority[prio];

            leftToGive = max(0.f, 1.f - length(newGravity));

            if (leftToGive <= 0)
                break;
        }

        affected.dirPerPriority.clear();
        newGravity *= affected.gravityScale;

        newGravity += affected.defaultGravity * leftToGive;

        body.gravity = newGravity;
        body.bedirt<&RigidBody::gravity>();
    });

}
