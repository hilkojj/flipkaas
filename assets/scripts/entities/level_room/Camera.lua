
persistenceMode(TEMPLATE | ARGS, {"Transform"})

defaultArgs({
    setAsMain = false,
    name = ""
})

function create(cam, args)

    setComponents(cam, {
        CameraPerspective {
            fieldOfView = 75,
            nearClipPlane = .1,
            farClipPlane = 1000
        }
    })

    component.Transform.getFor(cam)

    if args.name ~= "" then
        setName(cam, args.name)
    end
    if args.setAsMain then
        setMainCamera(cam)
    end

end
