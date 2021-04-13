
#ifndef GAME_ROOMSCREEN_H
#define GAME_ROOMSCREEN_H

#include <gu/screen.h>
#include <gl_includes.h>
#include <graphics/3d/debug_line_renderer.h>
#include <graphics/camera.h>
#include "EntityInspector3D.h"
#include "../../../level/room/Room3D.h"

class RoomScreen : public Screen
{
    bool showRoomEditor = false;

    DebugLineRenderer lineRenderer;

    EntityInspector3D inspector;

  public:

    Room3D *room;

    RoomScreen(Room3D *room, bool showRoomEditor=false);

    void render(double deltaTime) override;

    void onResize() override;

    void renderDebugStuff();

    ~RoomScreen() override;
};


#endif
