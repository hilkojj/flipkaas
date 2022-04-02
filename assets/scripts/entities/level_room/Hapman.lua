
masks = include("scripts/entities/level_room/_masks")

persistenceMode(TEMPLATE | ARGS)

loadRiggedModels("assets/models/hapman.glb", false)

function create(hapman)
    
    setName(hapman, "Hapman")

    local startZ = 75

	setComponents(hapman, {

        Transform {
            position = vec3(0, -144, startZ),
            scale = vec3(40)
        },
        RenderModel {
            modelName = "Hapman",
            visibilityMask = masks.NON_PLAYER
        },
        CustomShader {
            vertexShaderPath = "shaders/rigged.vert",
            fragmentShaderPath = "shaders/default.frag",
            defines = {FOG_BASED_ON_HEIGHT = "1"}
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
        --Inspecting()

	})

    local biteI = 0
    local atePlayer = false;
    _G.biteZ = startZ
    function bite(travel)

        biteI = biteI + 1

        local trans = component.Transform.getFor(hapman)
        local travelDuration = 3
        local biteTimeout = 147/24

        local goalPos = trans.position + vec3(0, 0, -travel)
	    
        component.Transform.animate(hapman, "position", goalPos, travelDuration, "pow3")
        component.Rigged.getFor(hapman).playingAnimations:add(PlayAnimation {
            name = "Bite",
            loop = false
        })

        setTimeout(hapman, biteTimeout, function ()
        
            _G.biteZ = goalPos.z - 1.75 * trans.scale.z

            if not atePlayer then

                local biteSensor = createChild(hapman, "bite sensor")
                setComponents(biteSensor, {
                    Transform {
                        position = vec3(0, 0, _G.biteZ + 100)
                    },
                    GhostBody {
			            collider = Collider {
				            collisionCategoryBits = masks.SENSOR,
				            collideWithMaskBits = masks.DYNAMIC_CHARACTER,
				            registerCollisions = true
			            }
		            },
		            SphereColliderShape {
			            radius = 102
		            }
                })

                onEntityEvent(biteSensor, "Collision", function (col)
                    if col.otherEntity == _G.player then
                        print("game over!")
                        component.Transform.remove(_G.player)
                        component.RigidBody.remove(_G.player)
                        atePlayer = true

                        setTimeout(biteSensor, 2, function()
                            local gameOverCam = createChild(hapman, "game over cam")

                            applyTemplate(gameOverCam, "Camera", {
                                setAsMain = true,
                                name = "game over cam"
                            })

                            local camRot = quat:new()
                            camRot.x = 142
                            camRot.y = -25
                            camRot.z = 180
                            local goalCamRot = quat:new()
                            goalCamRot.x = 142
                            goalCamRot.y = 50
                            goalCamRot.z = 180


                            setComponents(gameOverCam, {
                                Transform {
                                    position = vec3(-30, 40, _G.biteZ - 50),
                                    rotation = camRot
                                }
                            })
                            component.Transform.animate(gameOverCam, "position", vec3(60, 80, _G.biteZ - 90), 20, "pow2")
                            component.Transform.animate(gameOverCam, "rotation", goalCamRot, 20, "pow2")
                            component.CameraPerspective.animate(gameOverCam, "fieldOfView", 40, 20, "pow2")

                            bite(18)
                            
                            _G.showGameOverPopup(0)

                        end)
                    end
	            end)

                setTimeout(biteSensor, .1, function()

                    if not atePlayer then

                        print("pfiew!")
                        destroyEntity(biteSensor)

                        --[[
                        setName(biteSensor, "bite wall "..biteI)

                        setComponents(biteSensor, {
                            RigidBody {
                                mass = 0,
                                collider = Collider {
                                    collisionCategoryBits = masks.STATIC_TERRAIN,
				                    collideWithMaskBits = masks.DYNAMIC_CHARACTER | masks.DYNAMIC_PROPS,
                                    registerCollisions = true
                                }
                            }
                    
                        })
                        ]]--
                    end
                end)
            end -- atePlayer
        end)

    end

    listenToGamepadButton(hapman, 0, gameSettings.gamepadInput.test, "test")
    onEntityEvent(hapman, "test_pressed", function()
        bite(40)
    end)

end

