
#ifndef GAME_ROOMSCREEN_H
#define GAME_ROOMSCREEN_H

#include <gu/screen.h>
#include <gl_includes.h>
#include <graphics/3d/debug_line_renderer.h>

class RoomScreen : public Screen
{
    bool showRoomEditor = false;

    DebugLineRenderer lineRenderer;

  public:

    Room *room;

    RoomScreen(Room *room, bool showRoomEditor=false);

    void render(double deltaTime) override;

    void onResize() override;

    void renderDebugStuff();

    ~RoomScreen() override;
};


#endif
