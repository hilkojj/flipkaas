
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_masks")

loadModels("assets/models/banana.glb", false)
loadColliderMeshes("assets/models/banana.obj", false)

function create(e, args)
	
    setName(e, "banana planet")
	
	setComponents(e, {
		RigidBody {
			mass = 0,
			collider = Collider {
				bounciness = 1,
                frictionCoefficent = 1,
				collisionCategoryBits = collisionMasks.STATIC_TERRAIN,
				collideWithMaskBits = collisionMasks.DYNAMIC_CHARACTER | collisionMasks.DYNAMIC_PROPS
			}
		},
		ConcaveColliderShape {
			meshName = "Banana"
		},
		RenderModel {
			modelName = "Banana"
		},
		ShadowCaster(),
		ShadowReceiver()
	})
	component.Transform.getFor(e)

	for i = 1, 19 do
			
		local grav = createChild(e, "gravfield"..i)

		local yOffsets = {
			9,
			2.3,
			-1,
			-3,
			-4.3,
			-5,
			-6,
			-7,
			-7.5,
			-8,
			-8,
			-7.5,
			-7,
			-6,
			-4,
			-2.3,
			1,
			4,
			6
		}

		print(i)
		print(yOffsets[i])

		setComponents(grav, {
		
			Transform(),
			TransformChild {
				parentEntity = e,
				offset = Transform {
					position = vec3(i * 3 - 30, yOffsets[i], 0)
				}
			},
			GhostBody {
				collider = Collider {
					collisionCategoryBits = collisionMasks.SENSOR,
					collideWithMaskBits = collisionMasks.DYNAMIC_CHARACTER | collisionMasks.DYNAMIC_PROPS,
					registerCollisions = true
					-- todo: emit = false
				}
			},
			SphereColliderShape {
				radius = 15
			},
			GravityField {
				priority = 0
			},
			SphereGravityFunction {
			}
		})


	end

end
