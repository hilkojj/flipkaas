
persistenceMode(TEMPLATE | ARGS, {"Transform"})

masks = include("scripts/entities/level_room/_masks")

local instancer = nil
local i = 0

function create(e)

    i = i + 1
    setName(e, "energy"..i)

    if instancer == nil then

        instancer = createEntity()
        setName(instancer, "energy instancer")

        setComponents(instancer, {
		    InstancedRendering {
		    },
		    RenderModel {
                modelName = "Energy"
            },
            CustomShader {
                vertexShaderPath = "shaders/default.vert",
                fragmentShaderPath = "shaders/default.frag",
                defines = {SHINY = "1", INSTANCED = "1"}
            },
            ShadowCaster(),
            Transform()
        })

    end

	setComponents(e, {
        --RigidBody { mass = 0,
		GhostBody {
            collider = Collider {
                bounciness = 1,
                frictionCoefficent = 1,
                collisionCategoryBits = masks.SENSOR,
                collideWithMaskBits = masks.DYNAMIC_CHARACTER,
                registerCollisions = true
            }
        },
        SphereColliderShape {
            radius = 4
        },
        
	})
    component.Transform.getFor(e)
    component.InstancedRendering.getFor(instancer):dirty().transformEntities:add(e)

    local hit = false

    onEntityEvent(e, "Collision", function (col)
		if col.otherEntity == _G.player and not hit and #col.contactPoints > 0 then
            hit = true    

            local trans = component.Transform.getFor(e)
            local originalPos = vec3(trans.position.x, trans.position.y, trans.position.z) -- TODO: jam hack, to prevent saving changed position
            local playerTrans = component.Transform.getFor(player)
            local dir = playerTrans.position - trans.position
            
            component.Transform.animate(e, "scale", vec3(1.5), .2, "pow2In")
            component.Transform.animate(e, "position", trans.position + dir * vec3(.5), .1, "pow2Out")

            local energyLamp = getChild(_G.player, "energy lamp")
            component.PointLight.animate(energyLamp, "color", vec3(50, 100, 600), .1, "pow2In")
            component.Transform.animate(_G.player, "scale", vec3(1, 1.2, 1), .1, "pow2In")

            _G.increaseEnergyMeter()

            setTimeout(e, .1, function()
                component.Transform.animate(e, "position", trans.position + dir * vec3(-2), .3, "pow2")
                component.PointLight.animate(energyLamp, "color", vec3(0, 0, 0), .2, "pow2In")
                component.Transform.animate(_G.player, "scale", vec3(1), .1, "pow2In")

                setTimeout(e, .21, function()
            
            
                    component.Transform.animate(e, "scale", vec3(0), .1, "pow2Out", function()

                        component.InstancedRendering.getFor(instancer):dirty().transformEntities:erase(e)
                        component.GhostBody.remove(e) -- dont destroy, because that will be saved in the level
                        component.Transform.getFor(e).position = originalPos
                        component.Transform.getFor(e).scale = vec3(1)

                    end)

                end)
            end)

        end
	end)

end

