
#include <utils/code_editor/CodeEditor.h>
#include <level/Level.h>
#include <game/dibidab.h>
#include "RoomScreen.h"
#include "../../../game/Game.h"

RoomScreen::RoomScreen(Room *room, bool showRoomEditor)
        :
        room(room), showRoomEditor(showRoomEditor)
{
    assert(room != NULL);
}

void RoomScreen::render(double deltaTime)
{

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
