#include <asset_manager/asset.h>
#include <game/dibidab.h>
#include <utils/json_model_loader.h>

#include "game/Game.h"
#include "rendering/GameScreen.h"
#include "level/room/Room3D.h"

void initLuaStuff()
{
    auto &env = luau::getLuaState();

    env["gameEngineSettings"] = dibidab::settings;
    env["gameSettings"] = Game::settings;
    env["saveGameSettings"] = [] {
        Game::saveSettings();
    };
}

int main(int argc, char *argv[])
{
    Game::loadSettings();

    Level::customRoomLoader = [] (const json &j) {
        auto *room = new Room3D;
        room->fromJson(j);
        return room;
    };

    dibidab::init(argc, argv);

    File::createDir("./saves"); // todo, see dibidab trello
    gu::setScreen(new GameScreen);

    initLuaStuff();

    Game::uiScreenManager->openScreen(asset<luau::Script>("scripts/ui_screens/StartupScreen"));

    dibidab::run();

    Game::saveSettings();

    #ifdef EMSCRIPTEN
    return 0;
    #else
    quick_exit(0);
    #endif
}
