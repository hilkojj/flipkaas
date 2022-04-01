#include "CustomShaderSystem.h"
#include <map>
#include <utils/hashing.h>

#include "CustomShaderSystem.h"

static std::map<size_t, std::shared_ptr<ShaderAsset>> shaders;
static std::map<Room3D *, CustomShaderSystem::EntitiesPerShader> collections;

void setHash(CustomShader &custom)
{
    std::string combined = custom.vertexShaderPath + custom.fragmentShaderPath;
    for (auto &[key, value] : custom.defines)
        combined += key + value;
    custom.currentHash = hashStringCrossPlatform(combined);
}

void checkUseCount(size_t hash)
{
    auto &ptr = shaders[hash];
    if (!ptr)
        return;

    if (ptr.use_count() == 1)
    {
        shaders.erase(hash);
    }
}

const CustomShaderSystem::EntitiesPerShader &CustomShaderSystem::getEntitiesPerShaderForRoom(Room3D *room)
{
    return collections[room];
}

void CustomShaderSystem::init(EntityEngine *engine)
{
    room = dynamic_cast<Room3D *>(engine);
    if (!room) throw gu_err("engine is not a room");
    collections[room];

    room->entities.on_construct<CustomShader>().connect<&CustomShaderSystem::onComponentAdded>(this);
    room->entities.on_destroy<CustomShader>().connect<&CustomShaderSystem::onComponentRemoved>(this);
}

void CustomShaderSystem::update(double deltaTime, EntityEngine *)
{
    assert(room);

    room->entities.view<CustomShader, RenderModel>().each([&](auto e, CustomShader &custom, auto &) {

        if (custom.anyDirty())
        {
            if (custom.shader)
            {
                collections[room][custom.shader].erase(e);
            }

            auto prevHash = custom.currentHash;
            setHash(custom);
            
            if (shaders.find(custom.currentHash) == shaders.end())
            {
                std::string name = custom.vertexShaderPath + " " + custom.fragmentShaderPath;
                for (auto &[key, value] : custom.defines)
                {
                    name += " #" + key + " " + value;
                }

                shaders[custom.currentHash] = std::make_shared<ShaderAsset>(name, custom.vertexShaderPath, custom.fragmentShaderPath, custom.defines.empty());

                for (auto &[key, value] : custom.defines)
                    shaders[custom.currentHash]->definitions.define(key.c_str(), value);
            }
            custom.shader = shaders[custom.currentHash];
            collections[room][custom.shader].insert(e);

            checkUseCount(prevHash);

            custom.undirtAll();
        }
    });
}

CustomShaderSystem::~CustomShaderSystem()
{
    if (room)
        collections.erase(room);
}

void CustomShaderSystem::onComponentAdded(entt::registry &reg, entt::entity e)
{
    reg.get<CustomShader>(e).bedirtAll();
}

void CustomShaderSystem::onComponentRemoved(entt::registry &reg, entt::entity e)
{
    auto &custom = reg.get<CustomShader>(e);
    custom.shader = NULL; // decrease use count
    checkUseCount(custom.currentHash);
}
