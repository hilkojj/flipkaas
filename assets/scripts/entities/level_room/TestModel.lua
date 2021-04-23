
loadModels("assets/models/test.ubj", false)

function create(test)

    setName(test, "test_model")

    setComponents(test, {
        Transform(),
        RenderModel {
            modelName = "Suzanne"
        },
        ShadowCaster(),
        ShadowReceiver()
    })

end

