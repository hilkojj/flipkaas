
persistenceMode(TEMPLATE | ARGS, {"Transform"})

masks = include("scripts/entities/level_room/_masks")
loadModels("assets/models/cannon.glb", false)

local instancer = nil
local i = 0

function create(e)

    i = i + 1
    setName(e, "cannon"..i)

    if instancer == nil then

        instancer = createEntity()
        setName(instancer, "cannon instancer")

        setComponents(instancer, {
		    InstancedRendering {
		    },
		    RenderModel {
                modelName = "Cannon"
            },
            ShadowCaster(),
            ShadowReceiver(),
            Transform()
        })

    end

	setComponents(e, {
        RigidBody {
            mass = 0,
            collider = Collider {
                bounciness = 1,
                frictionCoefficent = 1,
                collisionCategoryBits = masks.STATIC_TERRAIN,
                collideWithMaskBits = masks.DYNAMIC_CHARACTER
            }
        },
        SphereColliderShape {
            radius = 5.5
        },
        
	})
    component.Transform.getFor(e)
    component.InstancedRendering.getFor(instancer):dirty().transformEntities:add(e)

    local sprinkleDirEntity = createChild(e)
    setComponents(sprinkleDirEntity, {
        Transform(),
        TransformChild {
            parentEntity = e,
            offset = Transform {
                position = vec3(1, 0, 0)
            }
        } 
    })
    
    function hide(e)

        setUpdateFunction(e, 0, nil)
        
        component.InstancedRendering.getFor(instancer):dirty().transformEntities:erase(e)
        component.RigidBody.remove(e) -- dont destroy, because that will be saved in the level

    end

    setUpdateFunction(e, .95, function()
    
        local cannonTrans = component.Transform.getFor(e)

        if cannonTrans.position.z < _G.biteZ - 200 then
            return
        end

        local dirEntityPos = component.Transform.getFor(sprinkleDirEntity).position
        local dir = dirEntityPos - cannonTrans.position


        local sprinkle = createChild(e)
        applyTemplate(sprinkle, "Sprinkle")

        local trans = component.Transform.getFor(sprinkle)
        trans.position = cannonTrans.position + dir * vec3(2)
        trans.rotation = cannonTrans.rotation
        local duration = 3
        component.Transform.animate(sprinkle, "position", cannonTrans.position + dir * vec3(70), duration, "linear")
        component.DespawnAfter.getFor(sprinkle).time = duration
        

    end)

    setTimeout(e, .1, function()
        _G.onBiteZChanged[#_G.onBiteZChanged + 1] = function()

            if _G.biteZ < component.Transform.getFor(e).position.z then
                hide(e)
            end

        end
    end)

end

