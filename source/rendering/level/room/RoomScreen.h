
#ifndef GAME_ROOMSCREEN_H
#define GAME_ROOMSCREEN_H

#include <gu/screen.h>
#include <gl_includes.h>
#include <graphics/3d/debug_line_renderer.h>
#include <graphics/camera.h>
#include <graphics/shader_asset.h>
#include <graphics/frame_buffer.h>
#include "EntityInspector3D.h"
#include "../../../level/room/Room3D.h"

struct RenderModel;
struct Rigged;

class RoomScreen : public Screen
{
    struct RenderContext
    {
        Camera &cam;
        ShaderProgram &shader, &riggedShader;
        uint mask;
        std::function<bool(entt::entity)> filter;
        bool
            lights = true,
            uploadLightData = true,
            shadows = true,
            materials = true,
            riggedModels = true;
    };

    bool showRoomEditor = false;

    DebugLineRenderer lineRenderer;

    EntityInspector3D inspector;

    ShaderAsset
        defaultShader, riggedShader, depthShader, riggedDepthShader,

        hdrShader;

    int prevNrOfPointLights = -1, prevNrOfDirLights = -1, prevNrOfDirShadowLights = -1;

    FrameBuffer *fbo = NULL;

    float hdrExposure = 1.0;

  public:

    Room3D *room;

    RoomScreen(Room3D *room, bool showRoomEditor=false);

    void render(double deltaTime) override;

    void onResize() override;

    void renderDebugStuff();

    ~RoomScreen() override;

  private:

    void renderRoom(const RenderContext &);

    void initializeShader(const RenderContext &, ShaderProgram &);

    void renderModel(const RenderContext &, ShaderProgram &, entt::entity, const Transform &, const RenderModel &, const Rigged *rig=NULL);


    // debug:
    void debugText(const std::string &, const vec3 &);

    void debugLights();

    void debugArmatures();
};


#endif
