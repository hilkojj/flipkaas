#include <asset_manager/asset.h>
#include <game/dibidab.h>
#include <utils/json_model_loader.h>

#include "game/Game.h"
#include "rendering/GameScreen.h"
#include "level/room/Room3D.h"
#include "rendering/level/room/EnvironmentMap.h"

void initLuaStuff()
{
    auto &env = luau::getLuaState();

    env["gameEngineSettings"] = dibidab::settings;
    env["gameSettings"] = Game::settings;
    env["saveGameSettings"] = [] {
        Game::saveSettings();
    };
}

void addAssetLoaders()
{
    AssetManager::addAssetLoader<EnvironmentMap>(".hdr", [] (auto &path) {

        auto map = new EnvironmentMap;
        map->original = SharedCubeMap(new CubeMap(CubeMap::fromHDRFile(path.c_str(), 512)));

        // this order is important:
        map->prefilterReflectionMap(Game::settings.graphics.prefilteredReflectionMapResolution);
        map->createIrradianceMap(32, Game::settings.graphics.convolutionStepDelta);

        return map;
    });
}

int main(int argc, char *argv[])
{
    addAssetLoaders();
    Game::loadSettings();

    Level::customRoomLoader = [] (const json &j) {
        auto *room = new Room3D;
        room->fromJson(j);
        return room;
    };

    dibidab::init(argc, argv);

    // enable seamless cubemap sampling for lower mip levels in the pre-filter cubemap.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

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
