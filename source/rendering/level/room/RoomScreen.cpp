
#include <utils/code_editor/CodeEditor.h>
#include <level/Level.h>
#include <game/dibidab.h>
#include "RoomScreen.h"
#include "../../../generated/Model.hpp"

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
    defaultShader.use();

    glUniform3fv(defaultShader.location("sunDirection"), 1, &vec3(1, 0, 0)[0]);
    glUniform3fv(defaultShader.location("camPosition"), 1, &cam.position[0]);

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
            glUniform4fv(defaultShader.location("specular"), 1, &modelPart.material->specular[0]);

            glUniform1i(defaultShader.location("useTexture"), modelPart.material->diffuseTexture.isSet() ? 1 : 0);
            if (modelPart.material->diffuseTexture.isSet())
                modelPart.material->diffuseTexture->bind(1, defaultShader, "diffuseTexture");


            modelPart.mesh->render(modelPart.meshPartIndex);
        }
    });
}
