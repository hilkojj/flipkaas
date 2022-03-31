
#ifndef GAME_SPRITERENDERER_H
#define GAME_SPRITERENDERER_H


#include <graphics/3d/mesh.h>
#include <graphics/camera.h>
#include <graphics/shader_asset.h>
#include "../../../external/entt/src/entt/entity/registry.hpp"
#include "../../generated/AsepriteView.hpp"

/**
 * Used to render all entities with a AsepriteView component.
 */
class SpriteRenderer
{
    SharedMesh quad;
    VertData instancedData;
    int instancedDataID = -1;

    ShaderAsset shader;

  public:
    SpriteRenderer();

    void add(const AsepriteView &, const ivec2 &position);

    void render(const Camera &);

};


#endif
