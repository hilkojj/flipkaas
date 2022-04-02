
masks = include("scripts/entities/level_room/_masks")

persistenceMode(TEMPLATE | ARGS, {"Transform"})

loadRiggedModels("assets/models/hapman.glb", false)

function create(hapman)
    
    setName(hapman, "Hapman")

	setComponents(hapman, {

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
    component.Transform.getFor(hapman)


end

