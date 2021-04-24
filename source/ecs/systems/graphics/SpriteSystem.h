
#ifndef GAME_SPRITESYSTEM_H
#define GAME_SPRITESYSTEM_H

#include <ecs/systems/EntitySystem.h>

struct AABB;

/**
 * see SpriteAnchor documentation
 */
class SpriteSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  protected:

    EntityEngine *room = NULL;

    void update(double deltaTime, EntityEngine *room) override;

    void updateAnimations(double deltaTime);
};

#endif
