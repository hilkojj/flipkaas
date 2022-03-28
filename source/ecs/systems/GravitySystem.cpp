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
                return mu::ZERO_3;
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
                return mu::ZERO_3;

            vec3 closestCirclePoint = victimPosDonutSpaceProjected / lenProj;


            vec3 diff = closestCirclePoint - victimPosDonutSpace;
            auto len = length(diff);
            if (len < .01 || len > localGravityRadius)
                return mu::ZERO_3;

            vec3 diffWorld = donutToWorld * vec4(diff, 0);

            return normalize(diffWorld);
        });

    });

    room->entities.view<Transform, GhostBody, GravityField, DiscGravityFunction>().each([&](auto e, const Transform &t, const GhostBody &ghost, GravityField &field, DiscGravityFunction &disc) {
        // TODO: code is 90% same as for Donut

        if (disc.radius < .01 || disc.gravityRadius < .01)
            return;

        auto discToWorld = glm::translate(mat4(1), t.position);
        discToWorld *= glm::toMat4(t.rotation);
        discToWorld = glm::scale(discToWorld, vec3(disc.radius));
        auto worldToDisc = inverse(discToWorld);

        float localGravityRadius = disc.gravityRadius / disc.radius;

        forEachVictim(field, ghost, [&](auto victim, const Transform &victimTrans) {

            vec3 victimPosDiscSpace = worldToDisc * vec4(victimTrans.position, 1);
            vec3 victimPosDiscSpaceProjected = victimPosDiscSpace;
            victimPosDiscSpaceProjected.y = 0;

            // disc radius is implicitly 1

            auto lenProj = length(victimPosDiscSpaceProjected);
            if (lenProj < .01)
                return mu::ZERO_3;

            vec3 closestCirclePoint = lenProj > 1 ? victimPosDiscSpaceProjected / lenProj : victimPosDiscSpaceProjected;

            vec3 diff = closestCirclePoint - victimPosDiscSpace;
            auto len = length(diff);
            if (len < .01 || len > localGravityRadius)
                return mu::ZERO_3;

            vec3 diffWorld = discToWorld * vec4(diff, 0);

            return normalize(diffWorld);
        });

    });

    room->entities.view<Transform, GhostBody, GravityField, CylinderGravityFunction>().each([&](auto e, const Transform &t, const GhostBody &ghost, GravityField &field, CylinderGravityFunction &cyl) {
        
        if (cyl.height < .01 || cyl.gravityRadius < .01)
            return;

        vec3 dirBottomTop = rotate(t.rotation, mu::Y);
        vec3 bottom = t.position - (dirBottomTop * (cyl.height * .5f));
        vec3 top = t.position + (dirBottomTop * (cyl.height * .5f));

        forEachVictim(field, ghost, [&](auto victim, const Transform &victimTrans) {

            // https://math.stackexchange.com/questions/1905533/find-perpendicular-distance-from-point-to-line-in-3d

            vec3 bottomToVic = victimTrans.position - bottom;
            float t = dot(bottomToVic, dirBottomTop);
            vec3 projected = bottom + t * dirBottomTop;
            vec3 diff = projected - victimTrans.position;
            float len = length(diff);
            if (len < .01 || len > cyl.gravityRadius)
                return mu::ZERO_3;

            return diff / len;
        });
    });

    room->entities.view<Transform, GhostBody, GravityField, CylinderToPlaneGravityFunction>().each([&](auto e, const Transform &t, const GhostBody &ghost, GravityField &field, CylinderToPlaneGravityFunction &ctp) {

        if (ctp.height < .01 || ctp.gravityRadius < .01)
            return;

        auto ctpToWorld = glm::translate(mat4(1), t.position);
        ctpToWorld *= glm::toMat4(t.rotation);
        auto worldToCtp = inverse(ctpToWorld);

        forEachVictim(field, ghost, [&](auto victim, const Transform &victimTrans) {

            vec3 victimPosCtpSpace = worldToCtp * vec4(victimTrans.position, 1);

            float height01 = 1 - (victimPosCtpSpace.y + ctp.height * .5) / ctp.height;
            if (height01 < 0 || height01 > 1)
                return mu::ZERO_3;

            vec3 victimPosCtpSpaceProjected = vec3(0, victimPosCtpSpace.y, 0);

            vec3 diffCenter = victimPosCtpSpaceProjected - victimPosCtpSpace;
            float distCenter = length(diffCenter);
            if (distCenter < .01 || distCenter > ctp.gravityRadius)
                return mu::ZERO_3;
            vec3 dirCenter = diffCenter / distCenter;

            vec3 rotatePoint = -dirCenter * ctp.gravityRadius;
            rotatePoint.y = ctp.height * .5;

            vec3 diff = victimPosCtpSpace - rotatePoint;
            float len = length(diff);
            if (len < .01)
                return mu::ZERO_3;

            return vec3(ctpToWorld * vec4(diff / len, 0));
        });
    });

    room->entities.view<Transform, GhostBody, GravityField, PlaneGravityFunction>().each([&](auto e, const Transform &t, const GhostBody &ghost, GravityField &field, auto) {

        vec3 dir = rotate(t.rotation, -mu::Y);
        forEachVictim(field, ghost, [&](auto victim, const Transform &victimTrans) {
            return dir;
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
