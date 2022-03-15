
#include "EntityInspector3D.h"

void EntityInspector3D::pickEntityGUI(const Camera *camera, DebugLineRenderer &renderer)
{
    pickEntity = false;
}

void EntityInspector3D::moveEntityGUI(const Camera *camera, DebugLineRenderer &renderer)
{
    moveEntity = false;
}

void EntityInspector3D::highLightEntity(entt::entity entity, const Camera *camera, DebugLineRenderer &renderer)
{
}
