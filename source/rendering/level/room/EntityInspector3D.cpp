
#include "EntityInspector3D.h"
#include "../../../ecs/systems/PhysicsSystem.h"
#include <reactphysics3d/reactphysics3d.h>
#include <generated/Inspecting.hpp>
#include "../../../generated/Physics.hpp"
#include <input/key_input.h>

void EntityInspector3D::drawGUI(const Camera *cam, DebugLineRenderer &dlr, GizmoRenderer &gizmoRenderer)
{
    EntityInspector::drawGUI(cam, dlr);

    if (movingEntity != entt::null && engine.entities.valid(movingEntity))
    {
        if (auto t = engine.entities.try_get<Transform>(movingEntity))
        {
            if (gizmoRenderer.gizmo(("entity_gizmo_" + std::to_string(int(movingEntity))).c_str(), t->position, t->rotation, t->scale))
            {
                if (RigidBody *rigidBody = engine.entities.try_get<RigidBody>(movingEntity))
                {
                    rigidBody->reactBody->setIsSleeping(true);
                }
            }
        }
    }
}

void EntityInspector3D::pickEntityGUI(const Camera *cam, DebugLineRenderer &renderer)
{
    pickEntity = false;

    if (KeyInput::justPressed(GLFW_KEY_ESCAPE))
        return;

    pickEntity = true;

    auto toInspect = pickEntityInRoom(cam, renderer);

    if (toInspect != entt::null)
    {
        MouseInput::capture(GLFW_MOUSE_BUTTON_LEFT, 10, 10);
        if (MouseInput::justPressed(GLFW_MOUSE_BUTTON_LEFT, 10))
        {
            reg.assign_or_replace<Inspecting>(toInspect);
            pickEntity = false;
        }
    }
}

entt::entity EntityInspector3D::pickEntityInRoom(const Camera *cam, DebugLineRenderer &renderer, bool preferBody)
{
    auto ps = engine.tryFindSystem<PhysicsSystem>();
    if (!ps) return entt::null;
    auto reactWorld = ps->getReactWorld();
    if (!reactWorld) return entt::null;

    struct RaycastCallback : public reactphysics3d::RaycastCallback {

        EntityEngine *engine;
        entt::entity out = entt::null;
        vec3 pos;

        RaycastCallback(EntityEngine *engine) : engine(engine) {}

        virtual reactphysics3d::decimal notifyRaycastHit(const reactphysics3d::RaycastInfo &info) {

            auto colliderEntity = entt::entity(reinterpret_cast<uintptr_t>(info.collider->getUserData()));
            if (!engine->entities.valid(colliderEntity))
            {
                throw gu_err("Collider userdata did not give an valid entity");
            }

            out = colliderEntity;
            pos.x = info.worldPoint.x;
            pos.y = info.worldPoint.y;
            pos.z = info.worldPoint.z;

            return reactphysics3d::decimal(0);
        }
    };

    reactphysics3d::Vector3 startPoint(cam->position.x, cam->position.y, cam->position.z);
    auto cursorRayEnd = cam->position + normalize(cam->getCursorRayDirection()) * cam->far_;
    reactphysics3d::Vector3 endPoint(cursorRayEnd.x, cursorRayEnd.y, cursorRayEnd.z);
    reactphysics3d::Ray ray(startPoint, endPoint);

    RaycastCallback cb(&engine);
    reactWorld->raycast(ray, &cb);

    if (cb.out != entt::null)
    {
        auto toInspect = cb.out;

        if (auto collider = engine.entities.try_get<Collider>(toInspect))
        {
            if ((preferBody || KeyInput::pressed(GLFW_KEY_LEFT_CONTROL)) && engine.entities.valid(collider->rigidBodyEntity))
                toInspect = collider->rigidBodyEntity;
        }

        if (auto name = engine.getName(toInspect))
            ImGui::SetTooltip((std::string(name) + " #%d").c_str(), int(toInspect));
        else
            ImGui::SetTooltip("#%d", int(toInspect));
        renderer.axes(cb.pos, 3, vec3(sin(glfwGetTime() * 4.f), 1, sin(glfwGetTime() * 4.f)));

        return toInspect;
    }
    return entt::null;
}

void EntityInspector3D::moveEntityGUI(const Camera *cam, DebugLineRenderer &renderer)
{
    if (KeyInput::justPressed(GLFW_KEY_ESCAPE))
    {
        moveEntity = false;
        movingEntity = entt::null;
        return;
    }
    moveEntity = true;
    movingEntity = entt::null;

    auto toInspect = pickEntityInRoom(cam, renderer, true);

    if (toInspect != entt::null)
    {
        MouseInput::capture(GLFW_MOUSE_BUTTON_LEFT, 10, 10);
        if (MouseInput::justPressed(GLFW_MOUSE_BUTTON_LEFT, 10))
        {
            movingEntity = toInspect;
            moveEntity = false;
        }
    }
}

void EntityInspector3D::highLightEntity(entt::entity entity, const Camera *camera, DebugLineRenderer &renderer)
{
}
