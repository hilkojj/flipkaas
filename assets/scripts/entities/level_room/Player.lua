
loadRiggedModels("assets/models/cubeman.ubj", false)

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

                }
            }
        }
    })

end

