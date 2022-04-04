
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

    _G.onBiteZChanged = {}
    _G.lastBitingStarted = 0

    _G.bite = function (travel)

        --travel = 0

        print("initiating bite with travel = "..travel)

        biteI = biteI + 1
        _G.lastBitingStarted = getTime()

        local trans = component.Transform.getFor(hapman)
        local travelDuration = 3
        local biteTimeout = 147/24

        local goalPos = trans.position + vec3(0, 0, -travel)
	    
        component.Transform.animate(hapman, "position", goalPos, travelDuration, "pow3")
        component.Rigged.getFor(hapman).playingAnimations:add(PlayAnimation {
            name = "Bite",
            loop = false
        })
        setTimeout(hapman, 0, function()
            setComponents(createEntity(), {

                DespawnAfter {
                    time = 10
                },
                Transform {
                    position = component.Transform.getFor(hapman).position
                },
                SoundSpeaker {
                    sound = "sounds/growl2",
                    volume = .1,
                },
                PositionedAudio()

                })
        end)

        setTimeout(hapman, biteTimeout, function ()
        
            _G.biteZ = goalPos.z - 1.75 * trans.scale.z

            for _, cb in pairs(_G.onBiteZChanged) do
                
                cb()
            end

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
                setComponents(createEntity(), {

                    DespawnAfter {
                        time = 3
                    },
                    Transform {
                        position = component.Transform.getFor(hapman).position
                    },
                    SoundSpeaker {
                        sound = "sounds/blood/long",
                        volume = .6,
                    },
                    --PositionedAudio()

                })

                onEntityEvent(biteSensor, "Collision", function (col)
                    if col.otherEntity == _G.player then
                        _G.playerHit(100)
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

    --[[
    listenToGamepadButton(hapman, 0, gameSettings.gamepadInput.test, "test")
    onEntityEvent(hapman, "test_pressed", function()
        bite(40)
    end)
    ]]--

end

