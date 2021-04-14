
loadModels("assets/models/test.ubj", false)

function create(player)

    setName(player, "player")

    setComponents(player, {
        Transform(),
        RenderModel {
            modelName = "Suzanne"
        }
    })

end

