
persistenceMode(TEMPLATE | ARGS, {"Transform", "PointLight"})

function create(light)

    local i = 0
    while not setName(light, "light_"..i) do
        i = i + 1
    end

    component.Transform.getFor(light) -- might be loaded already.
    component.PointLight.getFor(light) -- might be loaded already.

end
