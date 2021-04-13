
#include <graphics/3d/perspective_camera.h>
#include "Room3D.h"
#include "../../generated/Camera.hpp"

vec3 Room3D::getPosition(entt::entity e) const
{
    return entities.has<Transform>(e) ? entities.get<Transform>(e).position : vec3(0);
}

void Room3D::setPosition(entt::entity e, const vec3 &pos)
{
    entities.get_or_assign<Transform>(e).position = pos;
}

void Room3D::initializeLuaEnvironment()
{
    Room::initializeLuaEnvironment();

    luaEnvironment["currentRoom"] = luaEnvironment;
    luaEnvironment["setMainCamera"] = [&] (entt::entity e) {
        cameraEntity = e;
    };
    luaEnvironment["getMainCamera"] = [&] {
        return cameraEntity;
    };
}

void Room3D::update(double deltaTime)
{
    Room::update(deltaTime);

    delete camera;
    camera = cameraFromEntity(cameraEntity);

    if (!camera)
    {
        camera = new PerspectiveCamera(.1, 1000, 1, 1, 75);
        camera->position = vec3(10);
        camera->lookAt(mu::ZERO_3);
        camera->viewportWidth = gu::width;
        camera->viewportHeight = gu::height;
        camera->update();
    }
}

Room3D::~Room3D()
{
    delete camera;
}

mat4 Room3D::transformFromComponent(const Transform &t)
{
    mat4 mat(1.0f);

    mat = glm::translate(mat, t.position);

    // rotation.. http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
    mat *= glm::toMat4(t.rotation);

    mat = glm::scale(mat, t.scale);

    return mat;
}

Camera *Room3D::cameraFromEntity(entt::entity e) const
{
    if (!entities.valid(e))
        return NULL;

    auto *pers = entities.try_get<CameraPerspective>(e);
    auto *trans = entities.try_get<Transform>(e);

    if (pers && trans)
    {
        auto cam = new PerspectiveCamera(pers->nearClipPlane, pers->farClipPlane, gu::width, gu::height, pers->fieldOfView);
        cam->position = trans->position;

        mat4 transform = transformFromComponent(*trans);
        cam->up = transform * vec4(cam->up, 0);
        cam->direction = transform * vec4(cam->direction, 0);
        cam->right = transform * vec4(cam->right, 0);
        cam->update();
        return cam;
    }

    return NULL;
}
