
#include <utils/code_editor/CodeEditor.h>
#include <level/Level.h>
#include <game/dibidab.h>
#include <graphics/orthographic_camera.h>
#include <graphics/frame_buffer.h>
#include "RoomScreen.h"
#include "../../../generated/Camera.hpp"
#include "../../../generated/Model.hpp"
#include "../../../generated/Light.hpp"
#include "../../../game/Game.h"

RoomScreen::RoomScreen(Room3D *room, bool showRoomEditor)
        :
        room(room), showRoomEditor(showRoomEditor), inspector(*room, "Room"),

        defaultShader(
            "default shader",
            "shaders/default.vert", "shaders/default.frag"
        ),
        depthShader(
            "depth shader",
            "shaders/depth.vert", "shaders/depth.frag"
        )
{
    assert(room != NULL);
    inspector.createEntity_showSubFolder = "level_room";
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

            RenderContext shadowMapCon { orthoCam, depthShader };
            shadowMapCon.mask = sr.visibilityMask;
            shadowMapCon.lights = false;
            shadowMapCon.materials = false;
            glCullFace(GL_FRONT);
            renderRoom(shadowMapCon);
            glCullFace(GL_BACK);
            sr.fbo->unbind();
        });
    }

    RenderContext finalImg { *room->camera, defaultShader };
    finalImg.mask = ~0u;
    finalImg.shadows = Game::settings.graphics.shadows;
    if (room->entities.valid(room->cameraEntity))
        if (auto *cp = room->entities.try_get<CameraPerspective>(room->cameraEntity))
            finalImg.mask = cp->visibilityMask;

    renderRoom(finalImg);

    renderDebugStuff();
}

void RoomScreen::onResize()
{

}

void RoomScreen::renderDebugStuff()
{
    if (!dibidab::settings.showDeveloperOptions)
        return;

    gu::profiler::Zone z("debug");
    assert(room->camera != NULL);
    auto &cam = *room->camera;
    lineRenderer.projection = cam.combined;

    {
        // x-axis:
        lineRenderer.line(vec3(cam.position.x - 1000, 0, 0), vec3(cam.position.x + 1000, 0, 0), mu::X);
        // y-axis:
        lineRenderer.line(vec3(0, cam.position.y - 1000, 0), vec3(0, cam.position.y + 1000, 0), mu::Y);
        // z-axis:
        lineRenderer.line(vec3(0, 0, cam.position.z - 1000), vec3(0, 0, cam.position.z + 1000), mu::Z);
    }
    {
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
        });

        // point lights:
        room->entities.view<Transform, PointLight>().each([&](Transform &t, PointLight &pl) {
            lineRenderer.axes(t.position, .1, vec3(1, 1, 0));
        });
    }
    if (Game::settings.graphics.debugArmatures)
    {
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
                std::function<void(SharedBone &, mat4 &)> renderBone;
                renderBone = [&] (SharedBone &bone, mat4 &parent) {
                    depth++;

                    mat4 mat = glm::translate(mat4(parent), bone->translation);
                    mat *= glm::toMat4(bone->rotation);
                    mat = glm::scale(mat, bone->scale);

                    if (rig.boneAnimTransform.find(bone) != rig.boneAnimTransform.end())
                        mat = mat * rig.boneAnimTransform[bone];

                    vec3 p0 = parent * vec4(0, 0, 0, 1);
                    vec3 p1 = mat * vec4(0, 0, 0, 1);
                    vec3 p2 = mat * vec4(0, 0, -.3, 1); // slight offset just for debugging

                    vec3 color = std::vector<vec3>{
                        vec3(52, 235, 164),
                        vec3(233, 166, 245),
                        vec3(242, 214, 131)
                    }[depth % 3] / 255.f;

                    if (depth > 1) lineRenderer.line(p0, p1, color);
                    lineRenderer.axes(p1, .05, vec3(1));

                    vec2 screenPos = cam.projectPixels(p2);
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
                        ImGui::SetNextWindowBgAlpha(0);
                        ImGui::SetNextWindowPos(ImVec2(screenPos.x - 5, screenPos.y - 5));
                        ImGui::SetNextWindowSize(ImVec2(200, 30));
                        ImGui::Begin((bone->name + "__namepopup__").c_str(), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoInputs);
                        ImGui::SetWindowFontScale(.9);
                        ImGui::Text("%s", splitString(bone->name, "__").back().c_str());
                        ImGui::End();
                        ImGui::PopStyleVar();
                    }

                    for (auto &child : bone->children)
                        renderBone(child, mat);

                    if (bone->children.empty())
                        lineRenderer.line(p1, p2, mu::X);
                    depth--;
                };
                if (arm.root)
                    renderBone(arm.root, transform);

                break;
            }
        });
        glEnable(GL_DEPTH_TEST);
        glLineWidth(1.f);
    }

    inspector.drawGUI(&cam, lineRenderer);

    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Room"))
    {
        ImGui::Separator();
        ImGui::Checkbox("Show shadow boxes", &Game::settings.graphics.debugShadowBoxes);
        ImGui::Checkbox("Show armatures", &Game::settings.graphics.debugArmatures);

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
}

RoomScreen::~RoomScreen()
{
    room->entities.view<ShadowRenderer>().each([&](ShadowRenderer &sr) {
        sr.fbo = NULL;
    });
}

void RoomScreen::renderRoom(const RenderContext &con)
{
    gu::profiler::Zone z("render models");

    int texSlot = 3;

    if (con.lights && con.uploadLightData)
    {
        auto plView = room->entities.view<Transform, PointLight>();
        int nrOfPointLights = plView.size();
        if (nrOfPointLights != prevNrOfPointLights)
        {
            ShaderDefinitions::defineInt("NR_OF_POINT_LIGHTS", nrOfPointLights);
            prevNrOfPointLights = nrOfPointLights;
        }

        auto dlView = room->entities.view<Transform, DirectionalLight>(entt::exclude<ShadowRenderer>);
        int nrOfDirLights = dlView.size();
        if (nrOfDirLights != prevNrOfDirLights)
        {
            ShaderDefinitions::defineInt("NR_OF_DIR_LIGHTS", nrOfDirLights);
            prevNrOfDirLights = nrOfDirLights;
        }

        auto dShadowLView = room->entities.view<Transform, DirectionalLight, ShadowRenderer>();
        int nrOfDirShadowLights = dShadowLView.size();
        if (nrOfDirShadowLights != prevNrOfDirShadowLights)
        {
            ShaderDefinitions::defineInt("NR_OF_DIR_SHADOW_LIGHTS", nrOfDirShadowLights);
            prevNrOfDirShadowLights = nrOfDirShadowLights;
        }

        con.shader.use();

        int pointLightI = 0;
        plView.each([&](Transform &t, PointLight &pl) {

            std::string arrEl = "pointLights[" + std::to_string(pointLightI++) + "]";

            glUniform3fv(con.shader.location((arrEl + ".position").c_str()), 1, &t.position[0]);
            glUniform3fv(con.shader.location((arrEl + ".attenuation").c_str()), 1, &vec3(pl.constant, pl.linear, pl.quadratic)[0]); // todo, point to pl.constant?
            glUniform3fv(con.shader.location((arrEl + ".ambient").c_str()), 1, &pl.ambient[0]);
            glUniform3fv(con.shader.location((arrEl + ".diffuse").c_str()), 1, &pl.diffuse[0]);
            glUniform3fv(con.shader.location((arrEl + ".specular").c_str()), 1, &pl.specular[0]);
        });

        int dirLightI = 0;
        dlView.each([&](auto e, Transform &t, DirectionalLight &dl) {

            std::string arrEl = "dirLights[" + std::to_string(dirLightI++) + "]";

            auto transform = Room3D::transformFromComponent(t);
            vec3 direction = transform * vec4(-mu::Y, 0);

            glUniform3fv(con.shader.location((arrEl + ".direction").c_str()), 1, &direction[0]);
            glUniform3fv(con.shader.location((arrEl + ".ambient").c_str()), 1, &dl.ambient[0]);
            glUniform3fv(con.shader.location((arrEl + ".diffuse").c_str()), 1, &dl.diffuse[0]);
            glUniform3fv(con.shader.location((arrEl + ".specular").c_str()), 1, &dl.specular[0]);
        });

        int dirShadowLightI = 0;
        dShadowLView.each([&](auto e, Transform &t, DirectionalLight &dl, ShadowRenderer &sr) {

            std::string arrEl = "dirShadowLights[" + std::to_string(dirShadowLightI) + "]";

            auto transform = Room3D::transformFromComponent(t);
            vec3 direction = transform * vec4(-mu::Y, 0);

            glUniform3fv(con.shader.location((arrEl + ".light.direction").c_str()), 1, &direction[0]);
            glUniform3fv(con.shader.location((arrEl + ".light.ambient").c_str()), 1, &dl.ambient[0]);
            glUniform3fv(con.shader.location((arrEl + ".light.diffuse").c_str()), 1, &dl.diffuse[0]);
            glUniform3fv(con.shader.location((arrEl + ".light.specular").c_str()), 1, &dl.specular[0]);

            assert(sr.fbo != NULL && sr.fbo->depthTexture != NULL);
            sr.fbo->depthTexture->bind(++texSlot, con.shader, ("dirShadowMaps[" + std::to_string(dirShadowLightI) + "]").c_str());
            glUniformMatrix4fv(con.shader.location((arrEl + ".shadowSpace").c_str()), 1, GL_FALSE, &sr.shadowSpace[0][0]);
            dirShadowLightI++;
        });
    }
    else con.shader.use();

    glUniform3fv(con.shader.location("camPosition"), 1, &con.cam.position[0]);

    room->entities.view<Transform, RenderModel>().each([&](auto e, Transform &t, RenderModel &rm) {

        if (!(rm.visibilityMask & con.mask))
            return;

        auto &model = room->models[rm.modelName];
        if (!model)
            return;

        auto transform = Room3D::transformFromComponent(t);

        mat4 mvp = con.cam.combined * transform;
        glUniformMatrix4fv(con.shader.location("mvp"), 1, GL_FALSE, &mvp[0][0]);
        if (con.materials)
            glUniformMatrix4fv(con.shader.location("transform"), 1, GL_FALSE, &transform[0][0]);

        for (auto &modelPart : model->parts)
        {
            if (!modelPart.material || !modelPart.mesh)
                continue;

            if (!modelPart.mesh->vertBuffer)
                throw gu_err("Model (" + model->name + ") mesh (" + modelPart.mesh->name + ") is not assigned to any VertBuffer!");

            if (!modelPart.mesh->vertBuffer->isUploaded())
                modelPart.mesh->vertBuffer->upload(true);

            if (con.materials)
            {
                glUniform3fv(con.shader.location("diffuse"), 1, &modelPart.material->diffuse[0]);

                vec4 specAndExp = vec4(vec3(modelPart.material->specular) * modelPart.material->specular.a, modelPart.material->shininess);
                glUniform4fv(con.shader.location("specular"), 1, &specAndExp[0]);

                bool useDiffuseTexture = modelPart.material->diffuseTexture.isSet();
                glUniform1i(con.shader.location("useDiffuseTexture"), useDiffuseTexture);
                if (useDiffuseTexture)
                    modelPart.material->diffuseTexture->bind(0, con.shader, "diffuseTexture");

                bool useSpecularMap = modelPart.material->specularMap.isSet();
                glUniform1i(con.shader.location("useSpecularMap"), useSpecularMap);
                if (useSpecularMap)
                    modelPart.material->specularMap->bind(1, con.shader, "specularMap");

                bool useNormalMap = modelPart.material->normalMap.isSet();
                glUniform1i(con.shader.location("useNormalMap"), useNormalMap);
                if (useNormalMap)
                    modelPart.material->normalMap->bind(2, con.shader, "normalMap");
            }

            modelPart.mesh->render(modelPart.meshPartIndex);
        }
    });
}
