
persistenceMode(TEMPLATE | ARGS)

collisionMasks = include("scripts/entities/level_room/_collision_masks")
loadModels("assets/models/suzanne.glb", false)

function create(e)
	
    setName(e, "instanced test")

	
	setComponents(e, {
		
        RenderModel {
            modelName = "Suzanne"
        },
		InstancedRendering {
		},
		Transform {
			position = vec3(0, 10, 0)
		},
		GhostBody {

		},
		SphereColliderShape {
			radius = .5;
		},
        ShadowReceiver(),
		ShadowCaster()
	})
	
	local i = 0;

	setUpdateFunction(e, 2, function()
		
		i = i + 1

		local child = createChild(e, "instance "..i)

		setComponents(child, {

			Transform {
				position = component.Transform.getFor(e).position - vec3(0, 1, 0)
			},
			RigidBody {
				mass = 10,
				collider = Collider {
					bounciness = .3,
					frictionCoefficent = 1,
					collisionCategoryBits = collisionMasks.DYNAMIC_PROPS,
					collideWithMaskBits = collisionMasks.STATIC_TERRAIN | collisionMasks.DYNAMIC_PROPS | collisionMasks.SENSOR,
				}
			},
			ConvexColliderShape {
				meshName = "Suzanne"
			},
			DespawnAfter {
				time = 20
			},
			GravityFieldAffected {
				defaultGravity = vec3(0, -10, 0)
			}
		})
		
		component.InstancedRendering.getFor(e):dirty().transformEntities:add(child)

		setOnDestroyCallback(child, function()
			-- TODO: parent could be (about to be) destroyed, doing .getFor on a parent could cause a crash.

			component.InstancedRendering.getFor(e):dirty().transformEntities:erase(child)
		end)

	end)

end
