persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_masks")

function create(e)
	
	--[[
    setName(e, "instanced sprinkle shower")

	
	setComponents(e, {
		
        RenderModel {
            modelName = "Sprinkle"
        },
		InstancedRendering {
		},
		Transform {
			position = vec3(0, 10, 0)
		}
	})
	
	local i = 0;
	local interval = 30 / 400

	setUpdateFunction(e, interval, function()
		
		i = i + 1

		local child = createChild(e, "instance "..i)

		setComponents(child, {

			Transform {
				position = component.Transform.getFor(e).position - vec3(0, 1, 0),
				scale = vec3(.05)
			},
			RigidBody {
				mass = 1,
				collider = Collider {
					bounciness = .5,
					frictionCoefficent = 1,
					collisionCategoryBits = collisionMasks.DYNAMIC_PROPS,
					collideWithMaskBits = collisionMasks.STATIC_TERRAIN | collisionMasks.DYNAMIC_PROPS,
				},
				gravity = vec3(0, -1, 0)
			},
			SphereColliderShape {
				radius = .02
			},
			DespawnAfter {
				time = 5
			}
		})
		
		component.InstancedRendering.getFor(e):dirty().transformEntities:add(child)

		setOnDestroyCallback(child, function()
			-- TODO: parent could be (about to be) destroyed, doing .getFor on a parent could cause a crash.

			component.InstancedRendering.getFor(e):dirty().transformEntities:erase(child)
		end)

	end)
	]]--

end