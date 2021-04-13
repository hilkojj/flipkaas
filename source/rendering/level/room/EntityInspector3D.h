
#ifndef GAME_ENTITYINSPECTOR3D_H
#define GAME_ENTITYINSPECTOR3D_H


#include <ecs/EntityInspector.h>

class EntityInspector3D : public EntityInspector
{
  public:
    using EntityInspector::EntityInspector;
  protected:
    void pickEntityGUI(const Camera *camera, DebugLineRenderer &renderer) override;

    void moveEntityGUI(const Camera *camera, DebugLineRenderer &renderer) override;

    void highLightEntity(entt::entity entity, const Camera *camera, DebugLineRenderer &renderer) override;
};


#endif
