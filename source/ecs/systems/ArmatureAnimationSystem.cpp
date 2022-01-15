
#include "ArmatureAnimationSystem.h"
#include "../../level/room/Room3D.h"
#include "../../generated/Model.hpp"
#include <utils/math/interpolation.h>

SharedArmature getArmatureOfModel(SharedModel &model)
{
    for (auto &part : model->parts)
    {
        if (!part.armature) continue;
        return part.armature;
    }
    return NULL;
}

// todo: this function can be optimized a lot.
void ArmatureAnimationSystem::update(double deltaTime, EntityEngine *engine)
{
    auto room = dynamic_cast<Room3D *>(engine);
    if (!room) return;

    std::unordered_map<entt::entity, std::list<PlayAnimation>> finishedAnimations;

    room->entities.view<Transform, RenderModel, Rigged>().each([&](auto e, Transform &t, RenderModel &rm, Rigged &rig) {

        rig.bonePoseTransform.clear();

        auto &model = room->models[rm.modelName];
        if (!model) return;

        auto arm = getArmatureOfModel(model);
        if (!arm) return;

        auto it = rig.playingAnimations.begin();
        while (it != rig.playingAnimations.end())
        {
            std::unordered_map<SharedBone, vec3> boneTranslations;
            std::unordered_map<SharedBone, quat> boneRotations;
            std::unordered_map<SharedBone, vec3> boneScales;

            auto &play = *it;

            auto &anim = arm->animations[play.name];
            play.timer += deltaTime * play.timeMultiplier;
            play.timer = max(0.f, min(play.timer, anim.duration));

            for (auto &channel : anim.channels)
            {
                int firstI = 0, secondI = 0;
                float progress = 0;
                for (; secondI < channel.timeline->times.size(); secondI++)
                {
                    auto time = channel.timeline->times[secondI];
                    if (time >= play.timer)
                    {
                        firstI = max(0, firstI - 1);
                        float firstTime = channel.timeline->times[firstI];
                        float timeBetweenFirstAndSecond = time - firstTime;

                        if (timeBetweenFirstAndSecond > 0 && play.timer > firstTime)
                        {
                            progress = (play.timer - firstTime) / timeBetweenFirstAndSecond;
                        }
                        break;
                    }
                    firstI++;
                }
                if (secondI >= channel.timeline->times.size())
                    continue;

                vec3 *vec3Out = NULL;
                switch (channel.targetProperty)
                {
                    case Armature::Animation::Channel::TRANSLATION:
                    {
                        vec3Out = &boneTranslations[channel.target];
                    }
                    case Armature::Animation::Channel::SCALE:
                    {
                        if (!vec3Out)
                            vec3Out = &boneScales[channel.target];

                        auto &first = channel.propertyValues->vec3Values.at(firstI);
                        auto &second = channel.propertyValues->vec3Values.at(secondI);

                        *vec3Out = mix(first, second, progress);
                        break;
                    }
                    case Armature::Animation::Channel::ROTATION:
                    {
                        auto &first = channel.propertyValues->quatValues.at(firstI);
                        auto &second = channel.propertyValues->quatValues.at(secondI);

                        boneRotations[channel.target] = slerp(first, second, progress);
                        break;
                    }
                }
            }

            for (auto &bone : arm->bones)
            {
                auto trans = boneTranslations.find(bone);
                auto rot = boneRotations.find(bone);
                auto scale = boneScales.find(bone);

                // the difference between the bone's default transform and the animated transform:
                auto diff = bone->getBoneTransform(
                        trans != boneTranslations.end() ? &trans->second : NULL,
                        rot != boneRotations.end() ? &rot->second : NULL,
                        scale != boneScales.end() ? &scale->second : NULL
                ) * inverse(bone->getBoneTransform());

                if (rig.bonePoseTransform.find(bone) == rig.bonePoseTransform.end())
                {
                    rig.bonePoseTransform[bone] = mat4(1);
                }

                if (play.influence != 1.)
                {
                    mat4 diffInfluenced;
                    Interpolation::interpolate(mat4(1), diff, play.influence, diffInfluenced);
                    rig.bonePoseTransform[bone] *= diffInfluenced;
                }
                else
                {
                    rig.bonePoseTransform[bone] *= diff;
                }
            }

            bool endFinish = play.timer == anim.duration && play.timeMultiplier >= 0;
            bool beginFinish = play.timer == 0. && play.timeMultiplier < 0;

            if (beginFinish || endFinish)
            {
                finishedAnimations[e].push_back(play);
                if (play.loop)
                    play.timer = beginFinish ? anim.duration : 0.f;
                else
                {
                    it = rig.playingAnimations.erase(it);
                    continue;
                }
            }
            ++it;
        }

        std::function<void(SharedBone &, const mat4 &)> calcBone;
        calcBone = [&] (SharedBone &bone, const mat4 &parentDiff) {

            auto &diff = rig.bonePoseTransform[bone];
            diff *= bone->getBoneTransform();
            // diff is now the transform of the bone in it's parent local space.

            diff = parentDiff * diff;
            // diff is now the transform of the bone in global space.

            for (auto &child : bone->children)
                calcBone(child, diff);

            diff *= bone->inverseBindMatrix;
        };

        for (auto &bone : arm->bones)
        {
            if (bone->parent == NULL)
            {
                calcBone(bone, mat4(1));
            }
        }

    });
    for (auto &[e, l] : finishedAnimations)
        for (auto &a : l)
            room->emitEntityEvent(e, a, "AnimationFinished");

}
