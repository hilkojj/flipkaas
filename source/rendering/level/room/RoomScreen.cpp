
#include <utils/code_editor/CodeEditor.h>
#include <level/Level.h>
#include <game/dibidab.h>
#include <graphics/orthographic_camera.h>
#include <utils/quad_renderer.h>
#include "RoomScreen.h"
#include "../../../generated/Camera.hpp"
#include "../../../generated/Model.hpp"
#include "../../../generated/Light.hpp"
#include "../../../generated/Physics.hpp"
#include "../../../game/Game.h"
#include "EnvironmentMap.h"
#include "../../../ecs/systems/PhysicsSystem.h"

#include <generated/Inspecting.hpp>
#include <im3d.h>

const GLubyte dummyTexData[] = {0, 0, 0};

bool irradianceMapAsSkyBox = true;

RoomScreen::RoomScreen(Room3D *room, bool showRoomEditor)
        :
        room(room), showRoomEditor(showRoomEditor), inspector(*room, "Room"),

        defaultShader(
            "default shader",
            "shaders/default.vert", "shaders/default.frag"
        ),
        riggedShader(
                "rigged shader",
                "shaders/rigged.vert", "shaders/default.frag"
        ),
        depthShader(
            "depth shader",
            "shaders/depth.vert", "shaders/depth.frag"
        ),
        riggedDepthShader(
            "rigged depth shader",
            "shaders/rigged_depth.vert", "shaders/depth.frag"
        ),

        blurShader(
            "blur shader",
            "shaders/fullscreen_quad.vert", "shaders/gaussian_blur.frag"
        ),
        hdrShader(
            "hdr shader",
            "shaders/fullscreen_quad.vert", "shaders/tone_mapping.frag"
        ),
        skyShader(
            "sky shader",
//            "shaders/fullscreen_quad.vert", "shaders/sky.frag"
            "shaders/skybox.vert", "shaders/skybox.frag"
        ),

        dummyTexture(Texture::fromByteData(&dummyTexData[0], GL_RGB, 1, 1, GL_NEAREST, GL_NEAREST))
{
    assert(room != NULL);
    inspector.createEntity_showSubFolder = "level_room";
    EnvironmentMap::getBRDFLookUpTexture(); // create once.
}

OrthographicCamera camForDirLight(Transform &t, ShadowRenderer &sr)
{
    OrthographicCamera orthoCam(sr.nearClipPlane, sr.farClipPlane, sr.frustrumSize.x, sr.frustrumSize.y);

    auto transform = Room3D::transformFromComponent(t);
    orthoCam.position = t.position;
    orthoCam.direction = normalize(transform * vec4(-mu::Y, 0));
    orthoCam.up = normalize(transform * vec4(mu::Z, 0));
    orthoCam.right = normalize(cross(orthoCam.up, orthoCam.direction));
    orthoCam.update();

    return orthoCam;
}

void RoomScreen::render(double deltaTime)
{
    gu::profiler::Zone z("Room");

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!room->camera)
        return;

    // todo sort models by depth

    if (Game::settings.graphics.shadows)
    {
        gu::profiler::Zone z1("shadow maps");

        room->entities.view<Transform, DirectionalLight, ShadowRenderer>().each([&](Transform &t, DirectionalLight &dl, ShadowRenderer &sr) {

            auto orthoCam = camForDirLight(t, sr);
            sr.shadowSpace = orthoCam.combined;

            if (!sr.fbo || sr.fbo->width != sr.resolution.x || sr.fbo->height != sr.resolution.y)
            {
                sr.fbo = std::make_shared<FrameBuffer>(sr.resolution.x, sr.resolution.y);
                sr.fbo->addDepthTexture(GL_LINEAR, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            }

            sr.fbo->bind();
            glClear(GL_DEPTH_BUFFER_BIT);

            RenderContext shadowMapCon { orthoCam, depthShader, riggedDepthShader };
            shadowMapCon.mask = sr.visibilityMask;
            shadowMapCon.lights = false;
            shadowMapCon.materials = false;
            shadowMapCon.filter = [&] (auto e) { return room->entities.has<ShadowCaster>(e); };
            glCullFace(GL_FRONT);
            renderRoom(shadowMapCon);
            glCullFace(GL_BACK);
            sr.fbo->unbind();
        });
    }

    RenderContext finalImg { *room->camera, defaultShader, riggedShader };
    finalImg.mask = ~0u;
    finalImg.shadows = Game::settings.graphics.shadows;
    finalImg.skyShader = &skyShader;

    if (room->environmentMap.isSet())
        finalImg.skyBox = (irradianceMapAsSkyBox ? room->environmentMap->irradianceMap : room->environmentMap->original).get();

    if (room->entities.valid(room->cameraEntity))
        if (auto *cp = room->entities.try_get<CameraPerspective>(room->cameraEntity))
            finalImg.mask = cp->visibilityMask;

    assert(fbo != NULL);

    fbo->bind();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderRoom(finalImg);
    fbo->unbind();

    glDisable(GL_DEPTH_TEST);

    FrameBuffer *bloomBlurFbo = NULL;

    if (Game::settings.graphics.bloomBlurIterations)
    {
        // todo: better bloom implementation: https://www.youtube.com/watch?v=tI70-HIc5ro

        gu::profiler::Zone z1("bloom");
        bool horizontal = true, firstIteration = true;
        blurShader.use();
        for (unsigned int i = 0; i < Game::settings.graphics.bloomBlurIterations; i++)
        {
            bloomBlurFbo = blurPingPongFbos[horizontal];
            bloomBlurFbo->bind();
            glUniform1i(blurShader.location("horizontal"), horizontal);

            (firstIteration ? fbo->colorTextures.at(1) : blurPingPongFbos[!horizontal]->colorTexture)->bind(0, blurShader, "image");
            Mesh::getQuad()->render();
            bloomBlurFbo->unbind();
            horizontal = !horizontal;
            firstIteration = false;
        }

    }

    hdrShader.use();
    fbo->colorTexture->bind(0, hdrShader, "hdrImage");
    if (bloomBlurFbo)
        bloomBlurFbo->colorTexture->bind(1, hdrShader, "blurImage");
    glUniform1f(hdrShader.location("exposure"), hdrExposure);
    Mesh::getQuad()->render();

    glEnable(GL_DEPTH_TEST);

    renderDebugStuff(deltaTime);
}

void RoomScreen::onResize()
{
    // using GL_RGBA16F, because without the alpha channel webgl will give errors.

    delete fbo;
    fbo = new FrameBuffer(gu::widthPixels, gu::heightPixels, Game::settings.graphics.msaaSamples);
    fbo->addColorTexture(GL_RGBA16F, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_FLOAT);  // normal HDR color
    if (Game::settings.graphics.bloomBlurIterations)
        fbo->addColorTexture(GL_RGBA16F, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_FLOAT);  // bright HDR color, to be blurred.
    fbo->addDepthBuffer(GL_DEPTH24_STENCIL8);   // GL_DEPTH24_STENCIL8 is the same format as the default depth buffer. This is necessary for blitting to the default buffer.

    for (int i = 0; i < 2; i++)
    {
        delete blurPingPongFbos[i];
        blurPingPongFbos[i] = NULL;
        if (Game::settings.graphics.bloomBlurIterations)
        {
            blurPingPongFbos[i] = new FrameBuffer(gu::widthPixels, gu::heightPixels);//, Game::settings.graphics.msaaSamples);
            blurPingPongFbos[i]->addColorTexture(GL_RGBA16F, GL_RGBA, GL_NEAREST, GL_NEAREST, GL_FLOAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
}

int debugTextI = 0;

void RoomScreen::renderDebugStuff(double deltaTime)
{
    if (!dibidab::settings.showDeveloperOptions)
        return;

    gu::profiler::Zone z("debug");
    assert(room->camera != NULL);
    auto &cam = *room->camera;
    lineRenderer.projection = cam.combined;

    debugTextI = 0;

    static bool debugBRDFLUT = false;

    fbo->blitTo(GL_DEPTH_BUFFER_BIT);   // the debug lines will be depth-tested with the depth of the rendered scene

    {
        // x-axis:
        lineRenderer.line(vec3(cam.position.x - 1000, 0, 0), vec3(cam.position.x + 1000, 0, 0), mu::X);
        // y-axis:
        lineRenderer.line(vec3(0, cam.position.y - 1000, 0), vec3(0, cam.position.y + 1000, 0), mu::Y);
        // z-axis:
        lineRenderer.line(vec3(0, 0, cam.position.z - 1000), vec3(0, 0, cam.position.z + 1000), mu::Z);
    }
    debugLights();
    if (Game::settings.graphics.debugArmatures)
        debugArmatures();

    auto physicsSystem = room->tryFindSystem<PhysicsSystem>();
    
    physicsSystem->debugDraw([&](auto a, auto b, auto color) {
        lineRenderer.line(a, b, color);
    });

    gizmoRenderer.beginFrame(deltaTime, vec2(gu::width, gu::height), cam);
    inspector.drawGUI(&cam, lineRenderer, gizmoRenderer);
    gizmoRenderer.endFrame(cam);

    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Room"))
    {
        ImGui::Separator();
        ImGui::Checkbox("Show shadow boxes", &Game::settings.graphics.debugShadowBoxes);
        ImGui::Checkbox("Show armatures", &Game::settings.graphics.debugArmatures);

        {
            ImGui::Checkbox("Show colliders", &Game::settings.graphics.debugColliders);
            ImGui::Checkbox("Show collider AABBs", &Game::settings.graphics.debugColliderAABBs);
            ImGui::Checkbox("Show constraints", &Game::settings.graphics.debugConstraints);
            ImGui::Checkbox("Show constraint limits", &Game::settings.graphics.debugConstraintLimits);
        }

        ImGui::DragFloat("HDR exposure", &hdrExposure, .1, 0, 10);

        if (ImGui::BeginMenu("Environment map"))
        {
            for (auto &[path, asset] : AssetManager::getLoadedAssetsForType<EnvironmentMap>())
            {
                if (path != asset->fullPath)
                    continue;

                bool selected = room->environmentMap.isSet() && room->environmentMap.getLoadedAsset().fullPath == asset->fullPath;

                if (ImGui::MenuItem(asset->shortPath.c_str(), NULL, selected))
                    room->environmentMap.set(asset->fullPath);
            }
            ImGui::EndMenu();
        }
        ImGui::MenuItem("Irradiance map as skybox", NULL, &irradianceMapAsSkyBox);

        ImGui::MenuItem("Debug BRDF lookup texture", NULL, &debugBRDFLUT);

        ImGui::Separator();
        if (ImGui::MenuItem("View as JSON"))
        {
            auto &tab = CodeEditor::tabs.emplace_back();
            tab.title = room->name.empty() ? "Untitled room" : room->name;
            tab.title += " (READ-ONLY!)";

            json j;
            room->toJson(j);
            tab.code = j.dump(2);
            tab.languageDefinition = TextEditor::LanguageDefinition::C();
            tab.revert = [j] (auto &tab) {
                tab.code = j.dump(2);
            };
            tab.save = [] (auto &tab) {
                std::cerr << "Saving not supported" << std::endl;
            };
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    if (debugBRDFLUT)
        QuadRenderer::render(&EnvironmentMap::getBRDFLookUpTexture());
}

RoomScreen::~RoomScreen()
{
    room->entities.view<ShadowRenderer>().each([&](ShadowRenderer &sr) {
        sr.fbo = NULL;
    });
    delete fbo;
    delete blurPingPongFbos[0];
    delete blurPingPongFbos[1];
}

void RoomScreen::renderRoom(const RenderContext &con)
{
    gu::profiler::Zone z("render models");

    initializeShader(con, con.shader);
    room->entities.view<Transform, RenderModel>(entt::exclude<Rigged>).each([&](auto e, Transform &t, RenderModel &rm) {
        if (con.filter && !con.filter(e)) return;
        renderModel(con, con.shader, e, t, rm);
    });

    if (con.riggedModels)
    {
        gu::profiler::Zone z1("rigged models");

        bool first = true;
        room->entities.view<Transform, RenderModel, Rigged>().each([&](auto e, Transform &t, RenderModel &rm, Rigged &rig) {
            if (con.filter && !con.filter(e)) return;
            if (first)
            {
                initializeShader(con, con.riggedShader);
                first = false;
            }
            renderModel(con, con.riggedShader, e, t, rm, &rig);
        });
    }

    if (con.skyShader)
    {
        con.skyShader->use();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        if (con.skyBox)
        {
            con.skyBox->bind(0);
            glUniform1i(con.skyShader->location("skyBox"), 0);

            glUniformMatrix4fv(con.skyShader->location("projection"), 1, GL_FALSE, &con.cam.projection[0][0]);
            glUniformMatrix4fv(con.skyShader->location("view"), 1, GL_FALSE, &con.cam.view[0][0]);

            Mesh::getCube()->render();
        }
        else
            Mesh::getQuad()->render();

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
}

const int
    DIFFUSE_TEX_UNIT = 0,
    MET_ROUG_TEX_UNIT = 1,
    NORMAL_TEX_UNIT = 2,

    IRRADIANCE_MAP_TEX_UNIT = 3,
    PREFILTER_MAP_TEX_UNIT = 4,
    BRDF_LUT_TEX_UNIT = 5,

    FIRST_SHADOW_MAP_TEX_UNIT = 6;
    // NOTE: shadow maps might also use 7, 8, and 9

const char
    *DIFFUSE_UNI_NAME = "diffuseTexture",
    *MET_ROUG_UNI_NAME = "metallicRoughnessTexture",
    *NORMAL_UNI_NAME = "normalMap",

    *IRRADIANCE_UNI_NAME = "irradianceMap",
    *PREFILTER_UNI_NAME = "prefilterMap",
    *BRDF_LUT_UNI_NAME = "brdfLUT";

void RoomScreen::renderModel(const RenderContext &con, ShaderProgram &shader, entt::entity e, const Transform &t, const RenderModel &rm, const Rigged *rig)
{
    if (!(rm.visibilityMask & con.mask))
        return;

    auto &model = room->models[rm.modelName];
    if (!model)
        return;

    auto transform = Room3D::transformFromComponent(t);

    mat4 mvp = con.cam.combined * transform;
    glUniformMatrix4fv(shader.location("mvp"), 1, GL_FALSE, &mvp[0][0]);
    if (con.materials)
        glUniformMatrix4fv(shader.location("transform"), 1, GL_FALSE, &transform[0][0]);

    for (auto &modelPart : model->parts)
    {
        if (!modelPart.material || !modelPart.mesh) // TODO: give warning if modelpart has no material?
            continue;

        if (!modelPart.mesh->vertBuffer)
            throw gu_err("Model (" + model->name + ") mesh (" + modelPart.mesh->name + ") is not assigned to any VertBuffer!");

        if (!modelPart.mesh->vertBuffer->isUploaded())
            modelPart.mesh->vertBuffer->upload(true);

        if (con.materials)
        {
            bool useDiffuseTexture = modelPart.material->diffuseTexture.isSet();
            glUniform1i(shader.location("useDiffuseTexture"), useDiffuseTexture);
            if (useDiffuseTexture)
                modelPart.material->diffuseTexture.get().bind(DIFFUSE_TEX_UNIT, shader, DIFFUSE_UNI_NAME);
            else
            {
                glUniform3fv(shader.location("diffuse"), 1, &modelPart.material->diffuse[0]);
                dummyTexture.bind(DIFFUSE_TEX_UNIT, shader, DIFFUSE_UNI_NAME);
            }

            bool useMetallicRoughnessTexture = modelPart.material->metallicRoughnessTexture.isSet();
            glUniform1i(shader.location("useMetallicRoughnessTexture"), useMetallicRoughnessTexture);
            if (useMetallicRoughnessTexture)
                modelPart.material->metallicRoughnessTexture.get().bind(MET_ROUG_TEX_UNIT, shader, MET_ROUG_UNI_NAME);
            else
            {
                glUniform2fv(shader.location("metallicRoughnessFactors"), 1, &modelPart.material->metallic);
                dummyTexture.bind(MET_ROUG_TEX_UNIT, shader, MET_ROUG_UNI_NAME);
            }

            bool useNormalMap = modelPart.material->normalMap.isSet();
            glUniform1i(shader.location("useNormalMap"), useNormalMap);
            if (useNormalMap)
                modelPart.material->normalMap.get().bind(NORMAL_TEX_UNIT, shader, NORMAL_UNI_NAME);
            else
                dummyTexture.bind(NORMAL_TEX_UNIT, shader, NORMAL_UNI_NAME);

            if (con.shadows)
                glUniform1i(shader.location("useShadows"), room->entities.has<ShadowReceiver>(e));
        }

        if (rig && !modelPart.armature->bones.empty())
        {
            std::vector<mat4> matrices(modelPart.armature->bones.size());
            int i = 0;
            for (auto &bone : modelPart.armature->bones)
            {
                auto it = rig->bonePoseTransform.find(bone);
                matrices[i++] = it == rig->bonePoseTransform.end() ? mat4(1) : it->second;
            }
            glUniformMatrix4fv(shader.location("bonePoseTransforms"), matrices.size(), false, &matrices[0][0][0]);
        }

        modelPart.mesh->render(modelPart.meshPartIndex);
    }
}

void RoomScreen::initializeShader(const RenderContext &con, ShaderProgram &shader)
{
    if (con.lights && con.uploadLightData)
    {
        auto plView = room->entities.view<Transform, PointLight>();
        int nrOfPointLights = plView.size();
        if (nrOfPointLights != prevNrOfPointLights)
        {
            ShaderDefinitions::global().defineInt("NR_OF_POINT_LIGHTS", nrOfPointLights);
            prevNrOfPointLights = nrOfPointLights;
        }

        auto dlView = room->entities.view<Transform, DirectionalLight>(entt::exclude<ShadowRenderer>);
        int nrOfDirLights = dlView.size();
        if (nrOfDirLights != prevNrOfDirLights)
        {
            ShaderDefinitions::global().defineInt("NR_OF_DIR_LIGHTS", nrOfDirLights);
            prevNrOfDirLights = nrOfDirLights;
        }

        auto dShadowLView = room->entities.view<Transform, DirectionalLight, ShadowRenderer>();
        int nrOfDirShadowLights = dShadowLView.size();
        if (nrOfDirShadowLights != prevNrOfDirShadowLights)
        {
            ShaderDefinitions::global().defineInt("NR_OF_DIR_SHADOW_LIGHTS", nrOfDirShadowLights);
            prevNrOfDirShadowLights = nrOfDirShadowLights;
        }

        shader.use();

        int pointLightI = 0;
        plView.each([&](Transform &t, PointLight &pl) {

            std::string arrEl = "pointLights[" + std::to_string(pointLightI++) + "]";

            glUniform3fv(shader.location((arrEl + ".position").c_str()), 1, &t.position[0]);
            glUniform3fv(shader.location((arrEl + ".attenuation").c_str()), 1, &vec3(pl.constant, pl.linear, pl.quadratic)[0]); // todo, point to pl.constant?
            glUniform3fv(shader.location((arrEl + ".color").c_str()), 1, &pl.color[0]);
        });

        int dirLightI = 0;
        dlView.each([&](auto e, Transform &t, DirectionalLight &dl) {

            std::string arrEl = "dirLights[" + std::to_string(dirLightI++) + "]";

            auto transform = Room3D::transformFromComponent(t);
            vec3 direction = transform * vec4(-mu::Y, 0);

            glUniform3fv(shader.location((arrEl + ".direction").c_str()), 1, &direction[0]);
            glUniform3fv(shader.location((arrEl + ".color").c_str()), 1, &dl.color[0]);
        });

        int dirShadowLightI = 0;
        dShadowLView.each([&](auto e, Transform &t, DirectionalLight &dl, ShadowRenderer &sr) {

            std::string arrEl = "dirShadowLights[" + std::to_string(dirShadowLightI) + "]";

            auto transform = Room3D::transformFromComponent(t);
            vec3 direction = transform * vec4(-mu::Y, 0);

            glUniform3fv(shader.location((arrEl + ".light.direction").c_str()), 1, &direction[0]);
            glUniform3fv(shader.location((arrEl + ".light.color").c_str()), 1, &dl.color[0]);

            assert(sr.fbo != NULL && sr.fbo->depthTexture != NULL);
            sr.fbo->depthTexture->bind(FIRST_SHADOW_MAP_TEX_UNIT + dirShadowLightI, shader, ("dirShadowMaps[" + std::to_string(dirShadowLightI) + "]").c_str());
            glUniformMatrix4fv(shader.location((arrEl + ".shadowSpace").c_str()), 1, GL_FALSE, &sr.shadowSpace[0][0]);
            dirShadowLightI++;
        });
    }
    else shader.use();

    if (room->environmentMap.isSet())
    {
        room->environmentMap->irradianceMap->bind(IRRADIANCE_MAP_TEX_UNIT);
        glUniform1i(shader.location(IRRADIANCE_UNI_NAME), IRRADIANCE_MAP_TEX_UNIT);

        room->environmentMap->prefilteredReflectionMap->bind(PREFILTER_MAP_TEX_UNIT);
        glUniform1i(shader.location(PREFILTER_UNI_NAME), PREFILTER_MAP_TEX_UNIT);

        EnvironmentMap::getBRDFLookUpTexture().bind(BRDF_LUT_TEX_UNIT);
        glUniform1i(shader.location(BRDF_LUT_UNI_NAME), BRDF_LUT_TEX_UNIT);
    }
    // todo: else dummy cube texture?

    glUniform3fv(shader.location("camPosition"), 1, &con.cam.position[0]);
}

void RoomScreen::debugLights()
{
    gu::profiler::Zone z("lights");
    // directional lights:
    room->entities.view<Transform, DirectionalLight>().each([&](auto e, Transform &t, DirectionalLight &dl) {

        auto transform = Room3D::transformFromComponent(t);
        vec3 direction = transform * vec4(-mu::Y, 0);

        lineRenderer.axes(t.position, .1, vec3(1, 1, 0));
        for (int i = 0; i < 100; i++)
            lineRenderer.line(t.position + direction * float(i * .2), t.position + direction * float(i * .2 + .1), vec3(1, 1, 0));

        if (Game::settings.graphics.debugShadowBoxes) if (ShadowRenderer *sr = room->entities.try_get<ShadowRenderer>(e))
        {
            auto orthoCam = camForDirLight(t, *sr);

            // topleft
            auto A = t.position + orthoCam.up * sr->frustrumSize.y * .5f - orthoCam.right * sr->frustrumSize.x * .5f;
            // topright
            auto B = t.position + orthoCam.up * sr->frustrumSize.y * .5f + orthoCam.right * sr->frustrumSize.x * .5f;
            // bottomright
            auto C = t.position - orthoCam.up * sr->frustrumSize.y * .5f + orthoCam.right * sr->frustrumSize.x * .5f;
            // bottomleft
            auto D = t.position - orthoCam.up * sr->frustrumSize.y * .5f - orthoCam.right * sr->frustrumSize.x * .5f;

            auto depthDir = orthoCam.direction * orthoCam.far_;

            auto color = vec3(1, 1, 0);

            lineRenderer.line(A, B, color);
            lineRenderer.line(B, C, color);
            lineRenderer.line(C, D, color);
            lineRenderer.line(D, A, color);

            lineRenderer.line(A, A + depthDir, color);
            lineRenderer.line(B, B + depthDir, color);
            lineRenderer.line(C, C + depthDir, color);
            lineRenderer.line(D, D + depthDir, color);

            lineRenderer.line(A + depthDir, B + depthDir, color);
            lineRenderer.line(B + depthDir, C + depthDir, color);
            lineRenderer.line(C + depthDir, D + depthDir, color);
            lineRenderer.line(D + depthDir, A + depthDir, color);
        }

        if (auto name = room->getName(e))
            debugText(name, t.position);
    });

    // point lights:
    room->entities.view<Transform, PointLight>().each([&](auto e, Transform &t, PointLight &pl) {
        lineRenderer.axes(t.position, .1, vec3(1, 1, 0));
        if (auto name = room->getName(e))
            debugText(name, t.position);
    });
}

void RoomScreen::debugArmatures()
{
    gu::profiler::Zone z("armatures");
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.f);
    room->entities.view<Transform, RenderModel, Rigged>().each([&](auto e, Transform &t, RenderModel &rm, Rigged &rig) {
        auto &model = room->models[rm.modelName];
        if (!model) return;

        auto transform = Room3D::transformFromComponent(t);

        for (auto &modelPart : model->parts)
        {
            if (!modelPart.armature) continue;

            auto &arm = *modelPart.armature.get();

            int depth = 0;
            std::function<void(SharedBone &, mat4)> renderBone;
            renderBone = [&] (SharedBone &bone, mat4 globalParent) {
                depth++;

                auto global = rig.bonePoseTransform[bone] * inverse(bone->inverseBindMatrix);
                global = transform * global;

                vec3 parentJointPos = globalParent * vec4(mu::ZERO_3, 1);
                vec3 jointPos = global * vec4(mu::ZERO_3, 1);
                vec3 debugOffset = global * vec4(0, .2, 0, 1);

                lineRenderer.axes(jointPos, .05, vec3(1));

                debugText(bone->name, debugOffset);

                vec3 color = std::vector<vec3>{
                        vec3(52, 235, 164),
                        vec3(233, 166, 245),
                        vec3(242, 214, 131)
                }[depth % 3] / 255.f;

                if (depth > 1)
                    lineRenderer.line(parentJointPos, jointPos, color);
                if (bone->children.empty())
                    lineRenderer.line(jointPos, debugOffset, mu::X);

                for (auto &child : bone->children)
                    renderBone(child, global);

                depth--;
            };
            for (auto &bone : arm.bones)
            {
                if (!bone->parent)
                    renderBone(bone, mat4(1));
            }


            break;
        }
    });
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.f);
}

void RoomScreen::debugText(const std::string &text, const vec3 &pos)
{
    bool inScreen = false;
    assert(room->camera != NULL);
    vec2 screenPos = room->camera->projectPixels(pos, inScreen);
    if (inScreen)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::SetNextWindowBgAlpha(0);
        ImGui::SetNextWindowPos(ImVec2(screenPos.x - 5, screenPos.y - 5));
        ImGui::SetNextWindowSize(ImVec2(200, 30));
        ImGui::Begin(("__debugtextpopup__" + std::to_string(debugTextI++)).c_str(), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs);
        ImGui::SetWindowFontScale(.9);
        ImGui::Text("%s", text.c_str());
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
