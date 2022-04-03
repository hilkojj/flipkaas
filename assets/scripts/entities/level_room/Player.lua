
masks = include("scripts/entities/level_room/_masks")

loadRiggedModels("assets/models/flyman.glb", false)

function create(player)
    setName(player, "player")

    _G.player = player

    local defaultCollideMask = masks.STATIC_TERRAIN | masks.SENSOR
    local defaultMass = 1
    local radius = 1

    setComponents(player, {
        Transform {
            position = vec3(0, 10, 0)
        },
        RenderModel {
            modelName = "Flyman",
            visibilityMask = masks.PLAYER
        },
        --[[
        CustomShader {
            vertexShaderPath = "shaders/default.vert",
            fragmentShaderPath = "shaders/default.frag",
            defines = {TEST = "1"}
        },
        ]]--

        Rigged {
            playingAnimations = {
                PlayAnimation {
                    name = "Idle",
                    influence = 1,
                    loop = true
                }
            }
        },
        ShadowCaster(),
        RigidBody {
            gravity = vec3(0),
            mass = defaultMass,
            linearDamping = .1,
            angularAxisFactor = vec3(0),
            allowSleep = false,
            collider = Collider {
                bounciness = 0,
                frictionCoefficent = .1,
                collisionCategoryBits = masks.DYNAMIC_CHARACTER,
                collideWithMaskBits = defaultCollideMask,
                registerCollisions = true
            }
        },
        SphereColliderShape {
            radius = radius
        },
        GravityFieldAffected {
            gravityScale = 30,
            defaultGravity = vec3(0, -30, 0)
        },
        CharacterMovement {
            inputInCameraSpace = true,
            walkSpeed = 13,
            jumpForce = 1200,
            fallingForce = 2000
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
            resolution = ivec2(200),
            frustrumSize = vec2(7, 7),
            farClipPlane = 16
        }
    })

    local cam = getByName("3rd_person_camera")
    if valid(cam) then
        setComponents(cam, {
            ThirdPersonFollowing {
                target = player,
                visibilityRayMask = masks.STATIC_TERRAIN,
                backwardsDistance = 20,
                upwardsDistance = 11,
                boss = getByName("Hapman"),
                bossInfluence = 0,
                bossOffset = vec3(0, 160, 0)
            }
        })
        component.ThirdPersonFollowing.animate(cam, "bossInfluence", .8, 1, "pow2Out")
        setMainCamera(cam)
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

    local energyLamp = createChild(player, "energy lamp")
    setComponents(energyLamp, {
        Transform(),
        TransformChild {
            parentEntity = player,
            offset = Transform {
                position = vec3(0, 4, -3)
            }
        },
        PointLight {
            color = vec3(0)
        }
    })

    --[[
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
    ]]--

    listenToGamepadButton(player, 0, gameSettings.gamepadInput.test, "test")
    onEntityEvent(player, "test_pressed", function()
        print("epic")
    end)

    onEntityEvent(player, "Jump", function (col)
        
        component.Rigged.getFor(player).playingAnimations:add(PlayAnimation {
            name = "Jump",
            loop = false,
        })
	end)

    onEntityEvent(player, "Collision", function (col)
        
        if col.otherCategoryBits & masks.STATIC_TERRAIN ~= 0 and col.impact > 20 then
            -- hit floor
            component.Rigged.getFor(player).playingAnimations:add(PlayAnimation {
                name = "Land",
                --influence = col.impact / 24,
                loop = false,
                timer = 3/24
            })
            print(col.impact)
        end
	end)

    setUpdateFunction(player, 0, function()

        if not component.Transform.has(player) then
            return
        end
    
        local trans = component.Transform.getFor(player)
        trans.position.z = math.min(_G.biteZ - 1, trans.position.z)
        
    end)

    local flyStarts = {
        vec3(0, 30, -50)
    }

    _G.goFly = function(i)

        print("fly "..i)

        local transitionDuration = 2

        component.Transform.animate(player, "position", flyStarts[i], transitionDuration, "pow2")
        local body = component.RigidBody.getFor(player):dirty()
        body.collider:dirty().collideWithMaskBits = 0
        body.mass = 0

        if valid(cam) then
            component.ThirdPersonFollowing.animate(cam, "bossInfluence", 0, transitionDuration, "pow2")
        end

        component.SphereColliderShape.remove(player)

        setTimeout(player, 4, function()
        
            local body = component.RigidBody.getFor(player):dirty()
            body.collider:dirty().collideWithMaskBits = defaultCollideMask
            body.mass = defaultMass
            component.SphereColliderShape.getFor(player):dirty().radius = radius
        
        end)
    end
end

