
--loadRiggedModels("assets/models/cubeman.ubj", false)

function create(player)

    setName(player, "player")

    setComponents(player, {
        Transform {
            position = vec3(0, -1, 3)
        },
        RenderModel {
            modelName = "Cubeman"
        },
        Rigged {
            playingAnimations = {
                PlayAnimation {
                    name = "testanim",
                    influence = 1,
                }
            }
        },
        ShadowCaster()
    })

    onEntityEvent(player, "AnimationFinished", function(anim, unsub)
        print(anim.name.." has finished playing! Play it one more time but with less influence..")
        local anim = component.Rigged.getFor(player).playingAnimations[1]
        anim.loop = false
        anim.influence = .5
        unsub()

        component.Rigged.getFor(player).playingAnimations:add(PlayAnimation {
            name = "headanim",
            influence = 1.
        })
    end)

end

