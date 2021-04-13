
defaultArgs({
    setAsMain = false,
    name = ""
})

function create(cam, args)

    setComponents(cam, {
        Transform {
            position = vec3(10, 1, 10)
        },
        CameraPerspective {
            fieldOfView = 75,
            nearClipPlane = .1,
            farClipPlane = 1000
        }
    })

    if args.name ~= "" then
        setName(cam, args.name)
    end
    if args.setAsMain then
        setMainCamera(cam)
    end

end
