
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_masks")

defaultArgs({
	radius = 10,
	gravityRadius = 18
})

function create(e, args)
	
    setName(e, "sphere planet")
	
	local grav = createChild(e, "gravity field")


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
		SphereColliderShape {
			radius = args.radius
		},
		RenderModel {
			modelName = "Sphere"
		},
		ShadowCaster(),
		ShadowReceiver()
	})

	component.Transform.getFor(e).scale = vec3(args.radius)

	setComponents(grav, {
		
        Transform(),
		TransformChild {
			parentEntity = e
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
			radius = args.gravityRadius
		},
		GravityField {
			priority = 0
		},
		SphereGravityFunction {
		}
	})

end
