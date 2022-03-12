
loadRiggedModels("assets/models/cubeman.glb", false)

function create(player)

    setName(player, "player")

    setComponents(player, {
        Transform {
            position = vec3(0, 10, 0)
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
        ShadowCaster(),
        RigidBody {
            dynamic = true,
            gravity = false
        },
        Collider {
            rigidBodyEntity = player
        },
        BoxColliderShape {

        },
        Inspecting()
    })

    local testFloor = createEntity()
    setComponents(testFloor, {
        Transform {
            position = vec3(0, 0, 0)
        },
        RigidBody {
            dynamic = false
        },
        Collider {
            rigidBodyEntity = testFloor
        },
        BoxColliderShape {
            halfExtents = vec3(100, 1, 100)
        },
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
        component.RigidBody.getFor(player).gravity = true
    end)

end

