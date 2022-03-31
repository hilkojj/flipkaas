
masks = include("scripts/entities/level_room/_masks")

loadRiggedModels("assets/models/cubeman.glb", false)
loadColliderMeshes("assets/models/test_convex_colliders.obj", true)
loadColliderMeshes("assets/models/test_concave_colliders.obj", false)

function create(player)
    setName(player, "player")

    setComponents(player, {
        Transform {
            position = vec3(0, 1, 0)
        },
        RenderModel {
            modelName = "SmallMan",
            visibilityMask = masks.PLAYER
        },
        --[[
        Rigged {
            playingAnimations = {
                PlayAnimation {
                    name = "testanim",
                    influence = 1,
                }
            }
        },
        ]]--
        ShadowCaster(),
        RigidBody {
            gravity = vec3(0),
            mass = 1,
            linearDamping = .1,
            angularAxisFactor = vec3(0),
            collider = Collider {
                bounciness = 0,
                frictionCoefficent = .1,
                collisionCategoryBits = masks.DYNAMIC_CHARACTER,
                collideWithMaskBits = masks.STATIC_TERRAIN | masks.SENSOR,
            }
        },
        SphereColliderShape {
            radius = 1
        },
        GravityFieldAffected {
            gravityScale = 30,
            defaultGravity = vec3(0, -30, 0)
        },
        CharacterMovement {
        }
        --Inspecting()
    })

    local dropShadowSun = createChild(player, "drop shadow sun")
    setComponents(dropShadowSun, {
        Transform(),
		TransformChild {
			parentEntity = player,
            offset = Transform {
                position = vec3(0, 2.5, 0)
            }
		},
        DirectionalLight {
            color = vec3(-.7)
        },
        ShadowRenderer {
            visibilityMask = masks.PLAYER,
            resolution = ivec2(64),
            frustrumSize = vec2(2),
            farClipPlane = 16
        }
    })

    local cam = getByName("3rd_person_camera")
    if valid(cam) then
        setComponents(cam, {
            ThirdPersonFollowing {
                target = player,
                visibilityRayMask = masks.STATIC_TERRAIN
            }
        })
    end

    local sun = getByName("sun")
    if valid(sun) then
        local sunRot = quat:new()
        sunRot.x = 21
        sunRot.y = -21
        sunRot.z = 0

        setComponents(sun, {
            TransformChild {
                parentEntity = player,
                offsetInWorldSpace = true,
                position = true,
                rotation = false,
                scale = false,
                offset = Transform {
                    position = vec3(-67, 500, 179),
                    rotation = sunRot
                }
            }
        })
    end

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

