#include "PositionedAudioSystem.h"
#include "../../generated/PositionedAudio.hpp"

#include <generated/SoundSpeaker.hpp>
#include <audio/audio.h>

void PositionedAudioSystem::init(EntityEngine *engine)
{
    room = dynamic_cast<Room3D *>(engine);
    if (!room) throw gu_err("engine is not a room");
    updateFrequency = 60;

    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED); // "it’s the default and the one most suited for video game applications". Ok. https://indiegamedev.net/2020/04/12/the-complete-guide-to-openal-with-c-part-3-positioning-sounds/
}

void PositionedAudioSystem::update(double deltaTime, EntityEngine *)
{
    assert(room);

    if (!room->camera)
        return;
    
    auto cameraToWorld = translate(mat4(1.0f), room->camera->position);
    cameraToWorld *= toMat4(quatLookAt(room->camera->direction, room->camera->up));
    auto worldToCamera = inverse(cameraToWorld);

    room->entities.view<Transform, SoundSpeaker, PositionedAudio>().each([&](const Transform &t, SoundSpeaker &speaker, PositionedAudio &posAud) {

        if (!speaker.source)
            return;

        if (posAud.anyDirty())
        {
            au::alCall(alSourcef, speaker.source->source, AL_ROLLOFF_FACTOR, posAud.rollOffFactor);
            au::alCall(alSourcef, speaker.source->source, AL_REFERENCE_DISTANCE, posAud.referenceDistance);
            au::alCall(alSourcef, speaker.source->source, AL_MAX_DISTANCE, posAud.maxDistance);

            posAud.undirtAll();
        }

        vec3 camSpacePos = worldToCamera * vec4(t.position, 1);

        speaker.source->setPosition(camSpacePos);

    });
}

void PositionedAudioSystem::onComponentAdded(entt::registry &reg, entt::entity e)
{
    reg.get<PositionedAudio>(e).bedirtAll();
}
