
collisionMasks = include("scripts/entities/level_room/_collision_masks")

loadRiggedModels("assets/models/cubeman.glb", false)
loadColliderMeshes("assets/models/test_convex_colliders.obj", true)
loadColliderMeshes("assets/models/test_concave_colliders.obj", false)

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
            gravity = vec3(0),
            collider = Collider {
                bounciness = .5,
                frictionCoefficent = 1,
                collisionCategoryBits = collisionMasks.DYNAMIC_CHARACTER,
                collideWithMaskBits = collisionMasks.STATIC_TERRAIN | collisionMasks.SENSOR,
            }
        },
        CapsuleColliderShape {
            sphereRadius = 2,
            sphereDistance = 2
        },
        GravityFieldAffected {
            defaultGravity = vec3(0, -10, 0)
        }
        --Inspecting()
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
        component.RigidBody.getFor(player):dirty().gravity = vec3(0, -10, 0)
    end)

end

