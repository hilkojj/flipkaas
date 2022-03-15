
#include <game/dibidab.h>
#include "Game.h"

Game::Settings Game::settings;

#define SETTINGS_FILE_PATH "./settings.json"

void setShaderDefinitions()
{
    ShaderDefinitions::global().defineFloat("GAMMA", Game::settings.graphics.gammaCorrection);
    ShaderDefinitions::global().defineInt("SHADOWS", Game::settings.graphics.shadows);
    ShaderDefinitions::global().defineInt("MAX_BONES", Game::settings.graphics.maxArmatureBones);
    ShaderDefinitions::global().defineInt("BLOOM", Game::settings.graphics.bloomBlurIterations);
    ShaderDefinitions::global().defineFloat("BLOOM_THRESHOLD", Game::settings.graphics.bloomThreshold);
    ShaderDefinitions::global().defineFloat("FOG_START", Game::settings.graphics.fogStart);
    ShaderDefinitions::global().defineFloat("FOG_END", Game::settings.graphics.fogEnd);
    ShaderDefinitions::global().defineInt("FOG", Game::settings.graphics.fogEnabled);
    ShaderDefinitions::global().define("PREFILTER_SAMPLE_COUNT", std::to_string(Game::settings.graphics.prefilteredReflectionMapSamples) + "u");
    #ifdef EMSCRIPTEN
    ShaderDefinitions::global().define("WEBGL");
    #endif
}

void Game::loadSettings()
{
    if (!File::exists(SETTINGS_FILE_PATH))
    {
        settings = Settings();
        std::cerr << "Settings file (" << SETTINGS_FILE_PATH << ") not found!" << std::endl;
        saveSettings();
        loadSettings();// hack for loading font
        return;
    }
    try {
        json j = json::parse(File::readString(SETTINGS_FILE_PATH));
        settings = j;
        dibidab::settings = j;
    }
    catch (std::exception &e)
    {
        settings = Settings();
        std::cerr << "Error while loading settings:\n" << e.what() << std::endl;
    }
    setShaderDefinitions();
}

void Game::saveSettings()
{
    setShaderDefinitions();
    json j = dibidab::settings;
    j.merge_patch(settings);

    File::writeBinary(SETTINGS_FILE_PATH, j.dump(4));
}

cofu<UIScreenManager> Game::uiScreenManager;
