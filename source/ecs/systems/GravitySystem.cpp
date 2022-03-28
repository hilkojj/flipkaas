#include "GravitySystem.h"
#include "../../generated/Gravity.hpp"
#include "../../generated/Physics.hpp"
#include "../../generated/Transform.hpp"

void GravitySystem::init(EntityEngine *engine)
{
    room = dynamic_cast<Room3D *>(engine);
    if (!room) throw gu_err("engine is not a room");
}

void GravitySystem::update(double deltaTime, EntityEngine *)
{
    assert(room);

    room->entities.view<Transform, GhostBody, GravityField, SphereGravityFunction>().each([&](auto e, const Transform &t, const GhostBody &ghost, GravityField &field, auto) {

        forEachVictim(field, ghost, [&](auto victim, const Transform &victimTrans) {

            auto diff = t.position - victimTrans.position;
            auto len = length(diff);

            if (len > .01)
                return diff / len;
            else
                return vec3(0);
        });
    });

    room->entities.view<Transform, GhostBody, GravityField, DonutGravityFunction>().each([&](auto e, const Transform &t, const GhostBody &ghost, GravityField &field, DonutGravityFunction &donut) {

        if (donut.donutRadius < .01 || donut.gravityRadius < .01)
            return;

        auto donutToWorld = glm::translate(mat4(1), t.position);
        donutToWorld *= glm::toMat4(t.rotation);
        donutToWorld = glm::scale(donutToWorld, vec3(donut.donutRadius));
        auto worldToDonut = inverse(donutToWorld);

        float localGravityRadius = donut.gravityRadius / donut.donutRadius;

        forEachVictim(field, ghost, [&](auto victim, const Transform &victimTrans) {

            vec3 victimPosDonutSpace = worldToDonut * vec4(victimTrans.position, 1);
            vec3 victimPosDonutSpaceProjected = victimPosDonutSpace;
            victimPosDonutSpaceProjected.y = 0;

            // donut radius is implicitly 1

            auto lenProj = length(victimPosDonutSpaceProjected);
            if (lenProj < .01)
                return vec3(0);

            vec3 closestCirclePoint = victimPosDonutSpaceProjected / lenProj;


            vec3 diff = closestCirclePoint - victimPosDonutSpace;
            auto len = length(diff);
            if (len < .01 || len > localGravityRadius)
                return vec3(0);

            vec3 diffWorld = donutToWorld * vec4(diff, 0);

            return normalize(diffWorld);
        });

    });

    room->entities.view<RigidBody, GravityFieldAffected>().each([&](auto e, RigidBody &body, GravityFieldAffected &affected) {

        vec3 newGravity(0);

        float leftToGive = 1;

        for (int prio = affected.dirPerPriority.size() - 1; prio >= 0; prio--)
        {
            auto &dir = affected.dirPerPriority[prio];

            if (!all(lessThanEqual(abs(dir), vec3(.01))))
                newGravity += leftToGive * normalize(dir);


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

void GravitySystem::forEachVictim(const GravityField &field, const GhostBody &ghost, std::function<vec3(entt::entity, const Transform &)> func)
{
    for (auto &[victim, collision] : ghost.collider.collisions)
    {
        auto *affected = room->entities.try_get<GravityFieldAffected>(victim);
        auto *trans = room->entities.try_get<Transform>(victim);
        if (!affected || !trans)
            continue;

        if (affected->dirPerPriority.size() <= field.priority)
            affected->dirPerPriority.resize(field.priority + 1, vec3(0));

        auto dir = func(victim, *trans);

        if (dir.x != dir.x || dir.y != dir.y || dir.z != dir.z)
        {
            std::cerr << "Gravity field returned direction with NaN(s)! Victim: #" << int(victim) << std::endl;
            continue;
        }
            

        affected->dirPerPriority[field.priority] += dir * field.strength;
    }
}
