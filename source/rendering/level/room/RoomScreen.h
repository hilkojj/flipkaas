
#ifndef GAME_ROOMSCREEN_H
#define GAME_ROOMSCREEN_H

#include <gu/screen.h>
#include <gl_includes.h>
#include <graphics/3d/debug_line_renderer.h>
#include <graphics/camera.h>
#include <graphics/shader_asset.h>
#include <graphics/frame_buffer.h>
#include <graphics/cube_map.h>
#include "EntityInspector3D.h"
#include "GizmoRenderer.h"
#include "../../../level/room/Room3D.h"

struct RenderModel;
struct Rigged;

class RoomScreen : public Screen
{
    struct RenderContext
    {
        Camera &cam;
        ShaderProgram &shader, &riggedShader, &instancedShader;
        uint mask;
        std::function<bool(entt::entity)> filter;
        bool
            lights = true,
            uploadLightData = true,
            shadows = true,
            materials = true,
            riggedModels = true,
            customShaders = true;
        ShaderProgram *skyShader = NULL;
        CubeMap *skyBox = NULL;
    };

    bool showRoomEditor = false;

    DebugLineRenderer lineRenderer;
    GizmoRenderer gizmoRenderer;

    EntityInspector3D inspector;

    ShaderAsset
        defaultShader, riggedShader, instancedShader, depthShader, riggedDepthShader, instancedDepthShader,

        blurShader, hdrShader, skyShader;

    int prevNrOfPointLights = -1, prevNrOfDirLights = -1, prevNrOfDirShadowLights = -1;

    FrameBuffer *fbo = NULL;
    FrameBuffer *blurPingPongFbos[2] = {NULL, NULL};

    float hdrExposure = 1.0;

    Texture dummyTexture;

  public:
      
    Room3D *room;

    RoomScreen(Room3D *room, bool showRoomEditor=false);

    void render(double deltaTime) override;

    void onResize() override;

    void renderDebugStuff(double deltaTime);

    ~RoomScreen() override;

  private:

    void updateInstancedModels();

    void renderRoom(const RenderContext &);

    void initializeShader(const RenderContext &, ShaderProgram &);

    void prepareMaterial(entt::entity, const RenderContext &, const ModelPart &, ShaderProgram &);

    void renderModel(const RenderContext &, ShaderProgram &, entt::entity, const Transform &, const RenderModel &, const Rigged *rig=NULL);

    void renderInstancedModels(const RenderContext &, ShaderProgram &, entt::entity, const RenderModel &, const Room3D::ModelInstances &);


    // debug:
    void debugText(const std::string &, const vec3 &);

    void debugLights();

    void debugArmatures();
};


#endif
