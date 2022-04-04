
masks = include("scripts/entities/level_room/_masks")

loadRiggedModels("assets/models/flyman.glb", false)

function create(player)
    setName(player, "player")

    _G.player = player
    _G.getTime = getTime

    local defaultCollideMask = masks.STATIC_TERRAIN | masks.SENSOR | masks.DYNAMIC_PROPS
    local defaultMass = 1
    local defaultJumpForce = 1200
    local defaultWalkSpeed = 13
    local radius = 1

    setComponents(player, {
        Transform {
            position = vec3(0, 10, 0)---480)
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
            color = vec3(-.9)
        },
        ShadowRenderer {
            visibilityMask = masks.PLAYER,
            resolution = ivec2(200),
            frustrumSize = vec2(7, 7),
            farClipPlane = 16
        }
    })

    local cam = getByName("3rd_person_camera")
    local defaultBossInfluence = .73
    if valid(cam) then
        setComponents(cam, {
            ThirdPersonFollowing {
                target = player,
                visibilityRayMask = masks.STATIC_TERRAIN,
                backwardsDistance = 34,
                upwardsDistance = 20,
                boss = getByName("Hapman"),
                bossInfluence = 1.,
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
    local flying = false

    local arrivedStage = 0

    _G.playerHit = function(damage)
        
        local timeElapsed = getTime() - prevHitTime

        if timeElapsed < 1.5 or (flying and damage == 1) then
            return
        end

        _G.playerHealth = math.max(0, _G.playerHealth - damage)
        _G.updateHealthBar(_G.playerHealth)

        setComponents(player, {

            CustomShader {
                vertexShaderPath = "shaders/rigged.vert",
                fragmentShaderPath = "shaders/default.frag",
                defines = {DAMAGE = "1"}
            }
        })

        setComponents(createEntity(), {

            DespawnAfter {
                time = 3
            },
            Transform {
                position = component.Transform.getFor(player).position
            },
            SoundSpeaker {
                sound = "sounds/ouch",
                volume = .05,
                pitch = 1.1
            },
            --PositionedAudio()

        })
        setComponents(createEntity(), {

            DespawnAfter {
                time = 3
            },
            Transform {
                position = component.Transform.getFor(player).position
            },
            SoundSpeaker {
                sound = "sounds/blood/short0",
                volume = 3.
            },
            PositionedAudio()

        })

        setTimeout(player, 1., function()
        
            component.CustomShader.remove(player)
        
        end)

        if _G.playerHealth == 0 then

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
                        position = vec3(-30, 80, _G.biteZ - 100),
                        rotation = camRot
                    }
                })
                component.Transform.animate(gameOverCam, "position", vec3(60, 110, _G.biteZ - 200), 20, "pow2")
                component.Transform.animate(gameOverCam, "rotation", goalCamRot, 20, "pow2")
                component.CameraPerspective.animate(gameOverCam, "fieldOfView", 40, 20, "pow2")

                setTimeout(player, 4, function()
                    if getTime() - _G.lastBitingStarted > 9 then
                        _G.bite(12)
                    end
                end)
                            
                _G.showGameOverPopup(arrivedStage == 4)

            end)

        end

    end

    local enteringStage = false

    local timeSinceLanding = 0

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

            if getTime() - timeSinceLanding > .1 then

                setComponents(createEntity(), {

                    DespawnAfter {
                        time = 3
                    },
                    SoundSpeaker {
                        sound = "sounds/landing",
                        volume = .01,
                        pitch = 1.2
                    },

                })

                timeSinceLanding = getTime()

            end

            
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
        vec3(0, 27, -50),
        vec3(0, 35, -215),
        vec3(0, 46, -394),
        vec3(0, 46, -680),
    }
    local stageCamDists = {
        vec2(20, 11),
        vec2(35, 27),
        vec2(30, 16),
        vec2(5, 24),
        vec2(65, 26)
    }

    _G.goFly = function(to)

        print("fly to stage "..to)
        
        if _G.playerHealth <= 0 then
            print("nope youre dead")
            return
        end

        flying = true
        local transitionDuration = 2

        if to == 4 then
            component.DirectionalLight.animate(getByName("sun"), "color", vec3(4, 4, 4), 10)
            local ttttttdddddddd = 15
            component.Transform.animate(player, "position", flyStarts[to], ttttttdddddddd)

            component.GravityFieldAffected.remove(player)
            component.RigidBody.getFor(player):dirty().gravity = vec3(0, -1, 0)
            
            setTimeout(player, 30, function()
                _G.bite(140 + 178 + 460 + _G.biteZ)
            end)
            
            if valid(cam) then
                component.ThirdPersonFollowing.getFor(player).boss = getByName("plate planet")
                component.ThirdPersonFollowing.animate(cam, "bossInfluence", 0.8, transitionDuration, "pow2")
                component.ThirdPersonFollowing.animate(cam, "backwardsDistance", 22, transitionDuration, "pow2")
                component.ThirdPersonFollowing.animate(cam, "upwardsDistance", 13, transitionDuration, "pow2")
            end

            setTimeout(player, ttttttdddddddd, function()
                print("hello.. its me... again...")
                component.RigidBody.getFor(player):dirty().gravity = vec3(0, -30, 0)
                component.ThirdPersonFollowing.getFor(cam).boss = getByName("Hapman")
            end)

            local deadFlies = createEntity()
            applyTemplate(deadFlies, "DeadFlies")

        else

            component.Transform.animate(player, "position", flyStarts[to], transitionDuration, "pow2")
            
            if valid(cam) then
                component.ThirdPersonFollowing.animate(cam, "bossInfluence", 0, transitionDuration, "pow2")
                component.ThirdPersonFollowing.animate(cam, "backwardsDistance", 22, transitionDuration, "pow2")
                component.ThirdPersonFollowing.animate(cam, "upwardsDistance", 13, transitionDuration, "pow2")
            end


        end

        local body = component.RigidBody.getFor(player):dirty()
        body.collider:dirty().collideWithMaskBits = 0
        body.mass = 0

        
        component.Rigged.getFor(player).playingAnimations:clear()
        component.Rigged.getFor(player).playingAnimations:add(PlayAnimation {
            name = "Flying",
            loop = true,
            timeMultiplier = 1.5
        })
        setComponents(player, {

            SoundSpeaker {
                sound = "sounds/fly",
                volume = 0.2,
                pitch = 1.2,
                looping = true
            }

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

    local stage0Bites = 0
    local stage1Bites = 0
    local stage2Bites = 0
    local stage3Bites = 0

    local checkIfNeedToBite = nil
    checkIfNeedToBite = function()

        if _G.playerHealth <= 0 then
            return
        end

        local stage = _G.getStage()

        print("at stage "..stage.." arrived at stage "..arrivedStage.." health ".._G.playerHealth)

        if arrivedStage == 0 then
            
            -- bite takes roughly 8 secs, but more is needed for collision checks. so 9

            if stage0Bites == 0 then
                bite(12)
                setTimeout(player, 9, function()
                    if arrivedStage == 0 then
                        checkIfNeedToBite()
                    end
                end)
            elseif stage0Bites == 1 then
                bite(30)
                setTimeout(player, 9, function()
                    if arrivedStage == 0 then
                        checkIfNeedToBite()
                    end
                end)
            elseif stage0Bites == 2 then
                bite(12)
            end
            stage0Bites = stage0Bites + 1
        elseif arrivedStage == 1 then
        
            if stage1Bites == 0 then

                setTimeout(player, 4, function()
                
                    _G.bite(140 + _G.biteZ)
                    setTimeout(player, 11, function()
                        if arrivedStage == 1 then
                            checkIfNeedToBite()
                        end
                    end)
                end)


            elseif stage1Bites == 1 then

                _G.bite(10)
                setTimeout(player, 9, function()
                    if arrivedStage == 1 then
                        checkIfNeedToBite()
                    end
                end)

            elseif stage1Bites == 2 then

                _G.bite(21)

            end
            stage1Bites = stage1Bites + 1
        elseif arrivedStage == 2 then
        
            if stage2Bites == 0 then

                setTimeout(player, 2, function()
                
                    _G.bite(320 + _G.biteZ)
                    setTimeout(player, 14, function()
                        if arrivedStage == 2 then
                            checkIfNeedToBite()
                        end
                    end)
                end)


            elseif stage2Bites == 1 then

                
                _G.bite(31)

            elseif stage2Bites == 2 then


            end
            stage2Bites = stage2Bites + 1
        elseif arrivedStage == 3 then
        
            if stage3Bites == 0 then

                setTimeout(player, 16, function()
                
                    _G.bite(460 + _G.biteZ)
                    setTimeout(player, 14, function()
                        if arrivedStage == 3 then
                            checkIfNeedToBite()
                        end
                    end)
                end)


            elseif stage3Bites == 1 then

                _G.bite(60)
                setTimeout(player, 12, function()
                    if arrivedStage == 3 then
                        checkIfNeedToBite()
                    end
                end)

            elseif stage3Bites == 2 then

                _G.bite(30)

            end
            stage3Bites = stage3Bites + 1
        end
        
    end

    setTimeout(player, 7, checkIfNeedToBite)

    _G.arrivedAtStage = function()
        arrivedStage = _G.getStage()

        flying = false

        if valid(cam) then
            component.ThirdPersonFollowing.animate(cam, "bossInfluence", arrivedStage ~= 3 and defaultBossInfluence or 0., 2.5, "pow2")
            component.ThirdPersonFollowing.animate(cam, "backwardsDistance", stageCamDists[arrivedStage + 1].x, 5, "pow2")
            component.ThirdPersonFollowing.animate(cam, "upwardsDistance", stageCamDists[arrivedStage + 1].y, 5, "pow2")
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
        setComponents(createEntity(), {

            DespawnAfter {
                time = 3
            },
            SoundSpeaker {
                sound = "sounds/helicopter_landing",
                volume = 1.
            },

        })
        component.SoundSpeaker.remove(player)

        local body = component.RigidBody.getFor(player):dirty()
        body.collider:dirty().collideWithMaskBits = defaultCollideMask

        if arrivedStage > 0 then

            setTimeout(player, 9 - math.min(9, getTime() - _G.lastBitingStarted), checkIfNeedToBite)
        end
    end

    setTimeout(player, .1, _G.arrivedAtStage) -- banana


    --[[
    -- TODO: remove this:
    listenToGamepadButton(player, 0, gameSettings.gamepadInput.test, "test")
    onEntityEvent(player, "test_pressed", function()
        local energyyyy = createEntity()
        applyTemplate(energyyyy, "Energy", {}, true)
        component.Transform.getFor(energyyyy).position = component.Transform.getFor(player).position
        component.Transform.getFor(energyyyy).rotation = component.Transform.getFor(player).rotation
        component.Transform.getFor(energyyyy).scale = component.Transform.getFor(player).scale
    end)
    ]]--
end

