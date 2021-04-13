
#ifndef GAME_UISCREEN_H
#define GAME_UISCREEN_H


#include <graphics/orthographic_camera.h>
#include <graphics/frame_buffer.h>
#include <ecs/EntityEngine.h>
#include <graphics/3d/debug_line_renderer.h>
#include <ecs/EntityInspector.h>

class UIScreen : public EntityEngine, public Screen
{
    asset<luau::Script> script;

    EntityInspector inspector;

    bool initialized = false;

    bool renderingOrUpdating = false;

  public:

    UIScreen(const asset<luau::Script> &);

    void render(double deltaTime) override;

    void onResize() override;

    void renderDebugStuff();

    bool isRenderingOrUpdating() const { return renderingOrUpdating; }

    ~UIScreen() override;

};


#endif
