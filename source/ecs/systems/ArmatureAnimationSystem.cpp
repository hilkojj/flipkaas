
#include "ArmatureAnimationSystem.h"
#include "../../level/room/Room3D.h"
#include "../../generated/Model.hpp"

void ArmatureAnimationSystem::update(double deltaTime, EntityEngine *engine)
{
    auto room = dynamic_cast<Room3D *>(engine);
    if (!room) return;

    room->entities.view<Transform, RenderModel, Rigged>().each([&](Transform &t, RenderModel &rm, Rigged &rig) {

        rig.boneAnimTransform.clear();

        auto &model = room->models[rm.modelName];
        if (!model) return;

        for (auto &play : rig.playingAnimations)
        {
            play.timer += deltaTime * play.timeMultiplier;

            for (auto &part : model->parts)
            {
                if (!part.armature || part.armature->animations.find(play.name) == part.armature->animations.end())
                    return;

                auto &anim = part.armature->animations[play.name];

                for (auto &bone : part.bones)
                {
                    auto &keyframes = anim.keyFramesPerBone[bone];
                    if (keyframes.size() < 2)
                        continue;

                    int kfI;
                    for (kfI = 0; kfI < keyframes.size() - 2; kfI++)
                        if (keyframes[kfI + 1].keyTime >= play.timer)
                            break;

                    auto &kf0 = keyframes.at(kfI);
                    auto &kf1 = keyframes.at(kfI + 1);

                    float progress = (play.timer - kf0.keyTime) / (kf1.keyTime - kf0.keyTime);
                    progress = min(1.f, progress);

                    mat4 mat = glm::translate(mat4(1.f), bone->translation);
                    mat *= glm::toMat4(bone->rotation);
                    mat = glm::scale(mat, bone->scale);

                    auto &boneTrans = rig.boneAnimTransform[bone] = mat4(1);

                    boneTrans = glm::translate(mat4(1), mix(kf0.translation, kf1.translation, progress));
                    boneTrans *= glm::toMat4(slerp(kf0.rotation, kf1.rotation, progress));
                    boneTrans = glm::scale(boneTrans, mix(kf0.scale, kf1.scale, progress));

                    boneTrans = inverse(mat) * boneTrans;
                }
            }

        }

    });
}
