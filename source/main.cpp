#include <asset_manager/asset.h>
#include <game/dibidab.h>
#include <utils/json_model_loader.h>
#include <utils/aseprite/AsepriteLoader.h>

#include "game/Game.h"
#include "rendering/GameScreen.h"
#include "level/room/Room3D.h"
#include "rendering/level/room/EnvironmentMap.h"


float AsepriteView::playTag(const char *tagName, bool unpause)
{
    int i = 0;
    for (auto &tag : sprite->tags)
    {
        if (tag.name == tagName)
        {
            frame = tag.from;
            frameTimer = 0;
            pong = false;
            playingTag = i;
            paused = unpause ? false : paused;
            return tag.duration;
        }
        i++;
    }
    return 0;
}

void addAssetLoaders()
{
    // more loaders are added in dibidab::init()

    AssetManager::addAssetLoader<aseprite::Sprite>(".ase", [&](auto path) {

        auto sprite = new aseprite::Sprite;
        aseprite::Loader(path.c_str(), *sprite);
        Game::spriteSheet.add(*sprite);
        return sprite;
    });
    AssetManager::addAssetLoader<Palette>(".gpl", [](auto path) {

        return new Palette(path.c_str());
    });
}

void initLuaStuff()
{
    auto &env = luau::getLuaState();

    env["gameEngineSettings"] = dibidab::settings;
    env["gameSettings"] = Game::settings;
    env["saveGameSettings"] = [] {
        Game::saveSettings();
    };

    // colors:
    auto colorTable = sol::table::create(env.lua_state());
    Palette &palette = Game::palettes->effects.at(0).lightLevels[0].get();
    int i = 0;
    for (auto &[name, color] : palette.colors)
        colorTable[name] = i++;
    env["colors"] = colorTable;

    // sprite function: // todo: maybe this should not be in main.cpp
    env["playAsepriteTag"] = [] (AsepriteView &view, const char *tagName, sol::optional<bool> unpause) -> float {
        return view.playTag(tagName, unpause.value_or(false));
    };
    env["asepriteTagFrames"] = [] (AsepriteView &view, const char *tagName) {
        assert(view.sprite.isSet());
        auto &tag = view.sprite->getTagByName(tagName);
        return std::make_tuple(tag.from, tag.to);
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

    addAssetLoaders();

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

    auto onResize = gu::onResize += [] {
        ShaderDefinitions::defineInt("PIXEL_SCALING", Game::settings.graphics.uiPixelScaling);
    };
    gu::onResize();

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
