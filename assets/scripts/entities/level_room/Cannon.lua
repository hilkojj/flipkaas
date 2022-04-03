
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
            radius = 4
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

    setUpdateFunction(e, .5, function()
    
        local cannonTrans = component.Transform.getFor(e)
        local dirEntityPos = component.Transform.getFor(sprinkleDirEntity).position
        local dir = dirEntityPos - cannonTrans.position


        local sprinkle = createChild(e)
        applyTemplate(sprinkle, "Sprinkle")

        local trans = component.Transform.getFor(sprinkle)
        trans.position = cannonTrans.position + dir * vec3(2)
        trans.rotation = cannonTrans.rotation
        local duration = 3
        component.Transform.animate(sprinkle, "position", cannonTrans.position + dir * vec3(120), duration, "linear")
        component.DespawnAfter.getFor(sprinkle).time = duration
        

    end)

end

