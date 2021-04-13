#include <asset_manager/asset.h>
#include <game/dibidab.h>
#include <graphics/3d/model.h>
#include <utils/json_model_loader.h>

#include "game/Game.h"
#include "rendering/GameScreen.h"
#include "level/room/Room3D.h"

bool assetsLoaded = false;

void addAssetLoaders()
{
    // more loaders are added in dibidab::init()

    static VertBuffer *vertBuffer = NULL;

    AssetManager::addAssetLoader<std::vector<SharedModel>>(".ubj", [](auto path) {

        VertAttributes vertAttributes;
        vertAttributes.add(VertAttributes::POSITION);
        vertAttributes.add(VertAttributes::NORMAL);

        auto collection = new std::vector<SharedModel>(JsonModelLoader::fromUbjsonFile(path.c_str(), &vertAttributes));

        if (assetsLoaded && vertBuffer && !vertBuffer->isUploaded())
            vertBuffer->upload(true);   // upload before previous models get unloaded.

        if (!vertBuffer || vertBuffer->isUploaded())
            vertBuffer = VertBuffer::with(vertAttributes);

        for (auto &model : *collection)
            for (auto &part : model->parts)
            {
                if (part.mesh == NULL)
                    throw gu_err(model->name + " has part without a mesh!");

                if (part.mesh->vertBuffer)
                    continue; // this mesh was handled before

                for (int i = 0; i < part.mesh->nrOfVertices(); i++)
                    part.mesh->set<vec3>(part.mesh->get<vec3>(i, 0) * 16.f, i, 0);
                vertBuffer->add(part.mesh);
            }

        return collection;
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
}

int main(int argc, char *argv[])
{
    Game::loadSettings();

    addAssetLoaders();

    Level::customRoomLoader = [] (const json &j) {
        auto *room = new Room3D;
        room->fromJson(j);
        return room;
    };

    dibidab::init(argc, argv);
    assetsLoaded = true;

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
