
#include "LevelScreen.h"
#include <game/dibidab.h>
#include <imgui.h>

LevelScreen::LevelScreen(Level *lvl) : lvl(lvl)
{
    onPlayerEnteredRoom = lvl->onPlayerEnteredRoom += [this](Room *room, int playerId)
    {
        auto localPlayer = dibidab::getCurrentSession().getLocalPlayer();
        if (!localPlayer || localPlayer->id != playerId)
            return;
        std::cout << "Local player entered room. Show RoomScreen\n";
        showRoom(dynamic_cast<Room3D *>(room));
    };

    onRoomDeletion = lvl->beforeRoomDeletion += [this](Room *r)
    {
        if (roomScreen && r == roomScreen->room)
        {
            delete roomScreen;
            roomScreen = NULL;
        }
    };
}

void LevelScreen::render(double deltaTime)
{
    renderDebugTools();
    if (roomScreen)
        roomScreen->render(deltaTime);
}

void LevelScreen::onResize()
{
    if (roomScreen)
        roomScreen->onResize();
}

void LevelScreen::renderDebugTools()
{
    if (!dibidab::settings.showDeveloperOptions)
        return;

    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu("Level"))
    {
        auto str = std::to_string(lvl->getNrOfRooms()) + " room(s) active.";
        ImGui::MenuItem(str.c_str(), NULL, false, false);

        if (ImGui::MenuItem("Create new Room"))
            lvl->addRoom(new Room3D);

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void LevelScreen::showRoom(Room3D *r)
{
    delete roomScreen;
    roomScreen = NULL;
    if (r)
    {
        roomScreen = new RoomScreen(r, true);
        roomScreen->onResize();
    }
}

