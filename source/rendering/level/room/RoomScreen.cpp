
#include <utils/code_editor/CodeEditor.h>
#include <level/Level.h>
#include <game/dibidab.h>
#include "RoomScreen.h"
#include "../../../generated/Model.hpp"
#include "../../../generated/Light.hpp"

RoomScreen::RoomScreen(Room3D *room, bool showRoomEditor)
        :
        room(room), showRoomEditor(showRoomEditor), inspector(*room, "Room"),

        defaultShader(
            "default shader",
            "shaders/default.vert", "shaders/default.frag"
        )
{
    assert(room != NULL);
    inspector.createEntity_showSubFolder = "level_room";
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

    uint mask = ~0u;
    renderRoomWithCam(*room->camera, mask);

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
        room->entities.view<Transform, DirectionalLight>().each([&](Transform &t, DirectionalLight &dl) {

            auto transform = Room3D::transformFromComponent(t);
            vec3 direction = transform * vec4(-mu::Y, 0);

            lineRenderer.axes(t.position, .1, vec3(1, 1, 0));
            for (int i = 0; i < 100; i++)
                lineRenderer.line(t.position + direction * float(i * .2), t.position + direction * float(i * .2 + .1), vec3(1, 1, 0));
        });

        // point lights:
        room->entities.view<Transform, PointLight>().each([&](Transform &t, PointLight &pl) {
            lineRenderer.axes(t.position, .1, vec3(1, 1, 0));
        });
    }

    inspector.drawGUI(&cam, lineRenderer);

    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Room"))
    {
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
}

void RoomScreen::renderRoomWithCam(Camera &cam, uint mask)
{

    auto plView = room->entities.view<Transform, PointLight>();
    int nrOfPointLights = plView.size();
    if (nrOfPointLights != prevNrOfPointLights)
    {
        ShaderDefinitions::defineInt("NR_OF_POINT_LIGHTS", nrOfPointLights);
        prevNrOfPointLights = nrOfPointLights;
    }

    auto dlView = room->entities.view<Transform, DirectionalLight>();
    int nrOfDirLights = dlView.size();
    if (nrOfDirLights != prevNrOfDirLights)
    {
        ShaderDefinitions::defineInt("NR_OF_DIR_LIGHTS", nrOfDirLights);
        prevNrOfDirLights = nrOfDirLights;
    }

    defaultShader.use();

    glUniform3fv(defaultShader.location("camPosition"), 1, &cam.position[0]);

    int pointLightI = 0;
    plView.each([&](Transform &t, PointLight &pl) {

        std::string arrEl = "pointLights[" + std::to_string(pointLightI++) + "]";

        glUniform3fv(defaultShader.location((arrEl + ".position").c_str()), 1, &t.position[0]);
        glUniform3fv(defaultShader.location((arrEl + ".attenuation").c_str()), 1, &vec3(pl.constant, pl.linear, pl.quadratic)[0]); // todo, point to pl.constant?
        glUniform3fv(defaultShader.location((arrEl + ".ambient").c_str()), 1, &pl.ambient[0]);
        glUniform3fv(defaultShader.location((arrEl + ".diffuse").c_str()), 1, &pl.diffuse[0]);
        glUniform3fv(defaultShader.location((arrEl + ".specular").c_str()), 1, &pl.specular[0]);
    });

    int dirLightI = 0;
    dlView.each([&](Transform &t, DirectionalLight &dl) {

        std::string arrEl = "dirLights[" + std::to_string(dirLightI++) + "]";

        auto transform = Room3D::transformFromComponent(t);
        vec3 direction = transform * vec4(-mu::Y, 0);

        glUniform3fv(defaultShader.location((arrEl + ".direction").c_str()), 1, &direction[0]);
        glUniform3fv(defaultShader.location((arrEl + ".ambient").c_str()), 1, &dl.ambient[0]);
        glUniform3fv(defaultShader.location((arrEl + ".diffuse").c_str()), 1, &dl.diffuse[0]);
        glUniform3fv(defaultShader.location((arrEl + ".specular").c_str()), 1, &dl.specular[0]);
    });

    room->entities.view<Transform, RenderModel>().each([&](auto e, Transform &t, RenderModel &rm) {

        if (!(rm.visibilityMask & mask))
            return;

        auto &model = room->models[rm.modelName];
        if (!model)
            return;

        auto transform = Room3D::transformFromComponent(t);

        mat4 mvp = cam.combined * transform;
        glUniformMatrix4fv(defaultShader.location("mvp"), 1, GL_FALSE, &mvp[0][0]);
        glUniformMatrix4fv(defaultShader.location("transform"), 1, GL_FALSE, &transform[0][0]);

        for (auto &modelPart : model->parts)
        {
            if (!modelPart.material || !modelPart.mesh)
                continue;

            if (!modelPart.mesh->vertBuffer)
                throw gu_err("Model (" + model->name + ") mesh (" + modelPart.mesh->name + ") is not assigned to any VertBuffer!");

            if (!modelPart.mesh->vertBuffer->isUploaded())
                modelPart.mesh->vertBuffer->upload(true);

            glUniform3fv(defaultShader.location("diffuse"), 1, &modelPart.material->diffuse[0]);

            vec4 specAndExp = vec4(vec3(modelPart.material->specular) * modelPart.material->specular.a, modelPart.material->shininess);
            glUniform4fv(defaultShader.location("specular"), 1, &specAndExp[0]);

            bool useDiffuseTexture = modelPart.material->diffuseTexture.isSet();
            glUniform1i(defaultShader.location("useDiffuseTexture"), useDiffuseTexture);
            if (useDiffuseTexture)
                modelPart.material->diffuseTexture->bind(1, defaultShader, "diffuseTexture");

            bool useSpecularMap = modelPart.material->specularMap.isSet();
            glUniform1i(defaultShader.location("useSpecularMap"), useSpecularMap);
            if (useSpecularMap)
                modelPart.material->specularMap->bind(2, defaultShader, "specularMap");

            bool useNormalMap = modelPart.material->normalMap.isSet();
            glUniform1i(defaultShader.location("useNormalMap"), useNormalMap);
            if (useNormalMap)
                modelPart.material->normalMap->bind(3, defaultShader, "normalMap");

            modelPart.mesh->render(modelPart.meshPartIndex);
        }
    });
}
