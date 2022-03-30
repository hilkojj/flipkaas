#include "CharacterMovementSystem.h"
#include "../../generated/Physics.hpp"
#include "../../generated/Transform.hpp"
#include "../../generated/Character.hpp"
#include "../../generated/Camera.hpp"
#include "../../game/Game.h"
#include "PhysicsSystem.h"
#include <generated/PlayerControlled.hpp>
#include <input/key_input.h>

void CharacterMovementSystem::init(EntityEngine *engine)
{
    room = dynamic_cast<Room3D *>(engine);
    if (!room) throw gu_err("engine is not a room");
    updateFrequency = 60;
}

void CharacterMovementSystem::update(double deltaTime, EntityEngine *)
{
    assert(room);
    float dT(deltaTime);

    room->entities.view<CharacterMovement, LocalPlayer>().each([&](auto e, CharacterMovement &cm, auto) {

        cm.walkDirInput.y = KeyInput::pressed(Game::settings.keyInput.walkForwards) - KeyInput::pressed(Game::settings.keyInput.walkBackwards);
        cm.walkDirInput.x = KeyInput::pressed(Game::settings.keyInput.walkRight) - KeyInput::pressed(Game::settings.keyInput.walkLeft);
        cm.jumpInput = KeyInput::pressed(Game::settings.keyInput.jump);
    });

    room->entities.view<CharacterMovement, Transform, RigidBody>().each([&](auto e, CharacterMovement &cm, Transform &t, RigidBody &rb) {

        vec3 oldForward = rotate(t.rotation, -mu::Z);  // based on camera.cpp // todo: normalize?
        vec3 oldRight = rotate(t.rotation, mu::X);
        vec3 oldUp = normalize(cross(oldRight, oldForward));

        // ----------- new rotation:
        const static float MIN_GRAV = .1;

        float gravLen = length(rb.gravity);
        vec3 gravDir = gravLen < MIN_GRAV ? -oldUp : (rb.gravity / gravLen);

        vec3 up = -gravDir;
        vec3 forward = cross(oldRight, gravDir);
        float forwardLen = length(forward);

        if (forwardLen == 0)
        {
            std::cerr << "forwardLen == 0" << std::endl;
            // TODO
            return;
        }

        forward /= forwardLen;
        t.rotation = slerp(t.rotation, quatLookAt(forward, up), min(dT * 10.f, 1.f));
        vec3 interpolatedForward = rotate(t.rotation, -mu::Z);

        // --------------------

        if (!rb.collider.registerCollisions)
        {
            rb.collider.registerCollisions = true;
            rb.collider.bedirt<&Collider::registerCollisions>();
            return;
        }

        auto localToWorld = Room3D::transformFromComponent(t);
        auto worldToLocal = inverse(localToWorld);

        bool collisionWithGround = false;

        for (auto &[otherE, col] : rb.collider.collisions)
        {
            if (!room->entities.valid(otherE) || !room->entities.has<RigidBody>(otherE))
                continue;

            for (auto &cP : col->contactPoints)
            {
                vec3 local = worldToLocal * vec4(cP, 1);
                // TODO: bullet gives them in local too I think, just store that, saves some calculations.

                if (dot(local, -mu::Y) > .7) // TODO does this number make sense?
                {
                    collisionWithGround = true;
                    break;
                }
            }
            if (collisionWithGround)
                break;
        }

        auto &physics = room->getPhysics();
        vec3 currVelocity = physics.getLinearVelocity(rb);
        vec3 currVelLocal = worldToLocal * vec4(currVelocity, 0);
        bool justLanded = !cm.onGround;

        if (!collisionWithGround)
        {
            justLanded = false;

            if (cm.jumpStarted)
                cm.leftGroundSinceJumpStarted = true;
            cm.coyoteTime += dT;
            if (cm.jumpStarted)
                cm.onGround = false;
            else
                cm.onGround = cm.coyoteTime < .3; // TODO make variable
        }
        else
        {
            cm.coyoteTime = 0;
            cm.onGround = true;

            if (cm.leftGroundSinceJumpStarted && cm.jumpStarted)
            {
                cm.jumpStarted = false;
            }
            if (!cm.jumpStarted)
                physics.applyForce(rb, gravDir * 1000.f * dT);
        }

        if (cm.onGround)
        {
            if (cm.jumpInput && !cm.jumpStarted)
            {
                physics.applyForce(rb, up * cm.jumpForce);
                cm.jumpStarted = true;
                cm.leftGroundSinceJumpStarted = false;
                cm.holdingJumpEnded = false;
                cm.jumpDescend = false;
            }

            vec3 currVerticalVel = localToWorld * vec4(0, currVelLocal.y, 0, 0);

            vec3 velocity = forward * cm.walkDirInput.y * cm.walkSpeed + currVerticalVel;

            velocity = mix(currVelocity, velocity, min(1.f, dT * 10.f + justLanded));
            physics.setLinearVelocity(rb, velocity);
        }

        if (cm.jumpStarted)
        {
            if (currVelLocal.y < 0 && cm.leftGroundSinceJumpStarted)
            {
                cm.jumpDescend = true;
            }
            if (!cm.jumpInput)
            {
                cm.holdingJumpEnded = true;
            }
            
            if (cm.jumpDescend || cm.holdingJumpEnded)
            {
                physics.applyForce(rb, gravDir * cm.fallingForce * dT);
            }
        }

        t.rotation = rotate(t.rotation, cm.walkDirInput.x * dT * -3.f, mu::Y);
    });

    room->entities.view<Transform, ThirdPersonFollowing>().each([&](auto e, Transform &t, ThirdPersonFollowing &following) {

        if (!room->entities.valid(following.target) || !room->entities.has<Transform>(following.target))
            return;

        auto &targetTrans = room->entities.get<Transform>(following.target);

        // target's space:
        {
            auto targetToWorld = Room3D::transformFromComponent(targetTrans);   // todo remove scale, if scale is used for animations?
            auto worldToTarget = inverse(targetToWorld);

            vec3 targetForwardWorldSpace = targetToWorld * vec4(-mu::Z, 0);
            vec3 targetUpWorldSpace = targetToWorld * vec4(mu::Y, 0);
            vec3 camOffsetDir = vec3(-targetForwardWorldSpace.x, 0, -targetForwardWorldSpace.z);
            auto camOffsetLen = length(camOffsetDir);
            if (camOffsetLen > 0)
            {
                static const float MIN_OFFSET_LEN = .9;

                if (camOffsetLen < MIN_OFFSET_LEN)
                {
                    camOffsetDir += targetUpWorldSpace * 2.f * (1.f - (camOffsetLen / MIN_OFFSET_LEN));
                    camOffsetLen = length(camOffsetDir);
                }

                camOffsetDir /= camOffsetLen;

                //vec3 camOffsetDirTargetSpace = worldToTarget * vec4(camOffsetDir, 0.f);

                vec3 currentPos = t.position;//worldToTarget * vec4(t.position, 1);
                vec3 newPos = targetTrans.position + camOffsetDir * following.backwardsDistance + targetUpWorldSpace * following.upwardsDistance;

                auto currCamTargetDiff = targetTrans.position - t.position;
                auto currCamTargetDist = length(currCamTargetDiff);

                static const float SMOOTH_MIN_MAX = 4;

                float changeSpeedMultiplier = 1.f - min(1.f, max(0.f, (currCamTargetDist - following.minDistance) / SMOOTH_MIN_MAX));
                
                changeSpeedMultiplier += Interpolation::powIn(min(1.f, max(0.f, (currCamTargetDist - (following.maxDistance - SMOOTH_MIN_MAX)) / SMOOTH_MIN_MAX)), 2);
                changeSpeedMultiplier = min(1.f, changeSpeedMultiplier);

                /*
                auto &physics = room->getPhysics();
                physics.rayTest(targetTrans.position, t.position, [&](auto, const vec3 &hitPoint, const vec3 &normal) {


                }, true, following.visibilityRayMask);
                */

                vec3 interpolatedPos = mix(currentPos, newPos, min(1.f, dT * 3.f * changeSpeedMultiplier));
                t.position = interpolatedPos;//targetToWorld * vec4(interpolatedPos, 1);
            }


            
        }

        auto camTargetDiff = targetTrans.position - t.position;
        auto camTargetDist = length(camTargetDiff);

        if (camTargetDist != 0)
        {
            auto camTargetDir = camTargetDiff / camTargetDist;
            t.rotation = slerp(t.rotation, quatLookAt(camTargetDir, mu::Y), dT * 20.f);
        }
    });
}
