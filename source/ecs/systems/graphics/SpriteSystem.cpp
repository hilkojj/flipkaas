
#include "SpriteSystem.h"
#include <ecs/EntityEngine.h>
#include "../../../generated/AsepriteView.hpp"

void SpriteSystem::update(double deltaTime, EntityEngine *room)
{
    this->room = room;

    updateAnimations(deltaTime);
}

void SpriteSystem::updateAnimations(double deltaTime)
{
    room->entities.view<AsepriteView>().each([&](AsepriteView &view) {

        if (!view.sprite.isSet())
            return;

        if (view.frame >= view.sprite->frameCount)
            view.frame = 0;

        if (int(view.playingTag) >= int(view.sprite->tags.size()))
            view.playingTag = -1;

        if (view.playingTag < 0)
            return;

        if (!view.paused)
            view.frameTimer += deltaTime;

        const auto *frame = &view.sprite->frames.at(view.frame);

        while (view.frameTimer >= frame->duration)
        {
            frame = &view.sprite->frames.at(view.frame);
            view.frameTimer -= frame->duration;

            const auto &tag = view.sprite->tags[view.playingTag];

            if (tag.loopDirection == aseprite::Tag::pingPong)
            {
                if (view.pong && view.frame-- <= tag.from)
                {
                    view.pong = false;
                    if (!view.loop)
                    {
                        view.playingTag = -1;
                        view.frame = tag.from;
                        return;
                    }
                    view.frame = tag.from == tag.to ? tag.from : tag.from + 1;
                } else if (!view.pong && view.frame++ >= tag.to)
                {
                    view.pong = true;
                    view.frame = tag.from == tag.to ? tag.from : tag.to - 1;
                }
            } else if (tag.loopDirection == aseprite::Tag::reverse)
            {
                if (view.frame-- <= tag.from)
                {
                    if (!view.loop)
                    {
                        view.playingTag = -1;
                        view.frame = tag.from;
                        return;
                    }
                    view.frame = tag.to;
                }
            } else if (view.frame++ >= tag.to)
            {
                if (!view.loop)
                {
                    view.playingTag = -1;
                    view.frame = tag.to;
                    return;
                }
                view.frame = tag.from;
            }
        }
    });
}
