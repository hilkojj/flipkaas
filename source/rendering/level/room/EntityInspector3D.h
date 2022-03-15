
#ifndef GAME_ENTITYINSPECTOR3D_H
#define GAME_ENTITYINSPECTOR3D_H

#include "GizmoRenderer.h"
#include <ecs/EntityInspector.h>

class EntityInspector3D : public EntityInspector
{
  public:
    using EntityInspector::EntityInspector;

    void drawGUI(const Camera *, DebugLineRenderer &, GizmoRenderer &);

  protected:
    void pickEntityGUI(const Camera *camera, DebugLineRenderer &renderer) override;

    entt::entity pickEntityInRoom(const Camera *camera, DebugLineRenderer &renderer, bool preferBody=false);

    void moveEntityGUI(const Camera *camera, DebugLineRenderer &renderer) override;

    void highLightEntity(entt::entity entity, const Camera *camera, DebugLineRenderer &renderer) override;
};


#endif
