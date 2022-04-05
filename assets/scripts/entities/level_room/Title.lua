

function create(title)

    setName(title, "title")

    setComponents(title, {
        Transform(),
        RenderModel {
            modelName = "Title"
        },
        CustomShader {
            vertexShaderPath = "shaders/default.vert",
            fragmentShaderPath = "shaders/default.frag",
            defines = {TITLE = "1"}
        },
        --[[
        SoundSpeaker {
            sound = "sounds/soundtrack",
            volume = .25
        }
        ]]--
    })

end