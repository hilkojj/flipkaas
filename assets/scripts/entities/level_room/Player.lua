
masks = include("scripts/entities/level_room/_masks")

loadRiggedModels("assets/models/flyman.glb", false)

function create(player)
    setName(player, "player")

    _G.player = player

    local defaultCollideMask = masks.STATIC_TERRAIN | masks.SENSOR
    local defaultMass = 1
    local defaultJumpForce = 1200
    local defaultWalkSpeed = 13
    local radius = 1

    setComponents(player, {
        Transform {
            position = vec3(0, 10, 0)
        },
        RenderModel {
            modelName = "Flyman",
            visibilityMask = masks.PLAYER
        },
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
            walkSpeed = defaultWalkSpeed,
            jumpForce = defaultJumpForce,
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
    local defaultBossInfluence = .66
    if valid(cam) then
        setComponents(cam, {
            ThirdPersonFollowing {
                target = player,
                visibilityRayMask = masks.STATIC_TERRAIN,
                backwardsDistance = 34,
                upwardsDistance = 20,
                boss = getByName("Hapman"),
                bossInfluence = 0,
                bossOffset = vec3(0, 160, 0)
            }
        })
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

        if component.CharacterMovement.getFor(player).jumpForce == 0 then
            return
        end
        
        component.Rigged.getFor(player).playingAnimations:add(PlayAnimation {
            name = "Jump",
            loop = false,
        })
	end)

    _G.playerHealth = 4
    local prevHitTime = getTime()

    _G.playerHit = function(damage)
        
        local timeElapsed = getTime() - prevHitTime

        if timeElapsed < 1.5 then
            return
        end

        playerHealth = math.max(0, playerHealth - damage)

        setComponents(player, {

            CustomShader {
                vertexShaderPath = "shaders/rigged.vert",
                fragmentShaderPath = "shaders/default.frag",
                defines = {DAMAGE = "1"}
            }
        })

        setTimeout(player, 1., function()
        
            component.CustomShader.remove(player)
        
        end)

        if playerHealth == 0 then

            print("game over!")
            component.Transform.remove(player)
            component.RigidBody.remove(player)
            atePlayer = true

            setTimeout(player, 2, function()
                local gameOverCam = createChild(player, "game over cam")

                applyTemplate(gameOverCam, "Camera", {
                    setAsMain = true,
                    name = "game over cam"
                })

                local camRot = quat:new()
                camRot.x = 142
                camRot.y = -25
                camRot.z = 180
                local goalCamRot = quat:new()
                goalCamRot.x = 156
                goalCamRot.y = 37
                goalCamRot.z = 180


                setComponents(gameOverCam, {
                    Transform {
                        position = vec3(-30, 40, _G.biteZ - 100),
                        rotation = camRot
                    }
                })
                component.Transform.animate(gameOverCam, "position", vec3(60, 80, _G.biteZ - 200), 20, "pow2")
                component.Transform.animate(gameOverCam, "rotation", goalCamRot, 20, "pow2")
                component.CameraPerspective.animate(gameOverCam, "fieldOfView", 40, 20, "pow2")

                _G.bite(12)
                            
                _G.showGameOverPopup(0)

            end)

        end

    end

    local enteringStage = false

    onEntityEvent(player, "Collision", function (col)
        
        if col.otherCategoryBits & masks.STATIC_TERRAIN ~= 0 and (col.impact > 20 or enteringStage) then
            -- hit floor
            enteringStage = false
            local rigged = component.Rigged.getFor(player)
            rigged.playingAnimations:clear()
            rigged.playingAnimations:add(PlayAnimation {
                name = "Idle",
                loop = true,
            })
            rigged.playingAnimations:add(PlayAnimation {
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
        vec3(0, 27, -50)
    }
    local stageCamDists = {
        vec2(20, 11),
        vec2(50, 37)
    }

    _G.goFly = function(to)

        print("fly to stage "..to)

        local transitionDuration = 2

        component.Transform.animate(player, "position", flyStarts[to], transitionDuration, "pow2")
        local body = component.RigidBody.getFor(player):dirty()
        body.collider:dirty().collideWithMaskBits = 0
        body.mass = 0

        if valid(cam) then
            component.ThirdPersonFollowing.animate(cam, "bossInfluence", 0, transitionDuration, "pow2")
        end

        
        component.Rigged.getFor(player).playingAnimations:clear()
        component.Rigged.getFor(player).playingAnimations:add(PlayAnimation {
            name = "Flying",
            loop = true,
            timeMultiplier = 1.5
        })

        component.SphereColliderShape.remove(player)
        component.CharacterMovement.getFor(player).jumpForce = 0
        component.CharacterMovement.getFor(player).walkSpeed = 22

        setTimeout(player, transitionDuration, function()
        
            local body = component.RigidBody.getFor(player):dirty()
            body.collider:dirty().collideWithMaskBits = masks.FLY_WALLS | masks.SENSOR
            body.mass = defaultMass
            component.SphereColliderShape.getFor(player):dirty().radius = radius
        
            

        end)
    end

    _G.arrivedAtStage = function()
        if valid(cam) then
            component.ThirdPersonFollowing.animate(cam, "bossInfluence", defaultBossInfluence, 5, "pow2")
            component.ThirdPersonFollowing.animate(cam, "backwardsDistance", stageCamDists[_G.getStage() + 1].x, 5, "pow2")
            component.ThirdPersonFollowing.animate(cam, "upwardsDistance", stageCamDists[_G.getStage() + 1].y, 5, "pow2")
        end

        component.CharacterMovement.getFor(player).jumpForce = defaultJumpForce
        component.CharacterMovement.getFor(player).walkSpeed = defaultWalkSpeed

        component.Rigged.getFor(player).playingAnimations:clear()
        component.Rigged.getFor(player).playingAnimations:add(PlayAnimation {
            name = "EnteringStage",
            loop = true,
            timeMultiplier = 2
        })
        enteringStage = true

        local body = component.RigidBody.getFor(player):dirty()
        body.collider:dirty().collideWithMaskBits = defaultCollideMask
    end

    setTimeout(player, .1, _G.arrivedAtStage) -- banana

    

    --setTimeout(player, 7, _G.bite)
end

