
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_collision_masks")

defaultArgs({
	radius = 10,
	gravityRadius = 10
})

function create(e, args)
	
    setName(e, "disc planet")
	
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
		ConvexColliderShape {
			meshName = "Disc"
		},
		RenderModel {
			modelName = "Disc"
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
		BoxColliderShape {
			halfExtents = vec3(args.radius + args.gravityRadius, args.gravityRadius, args.radius + args.gravityRadius)
		},
		GravityField {
			priority = 0
		},
		DiscGravityFunction {
			radius = args.radius,
			gravityRadius = args.gravityRadius
		}
	})

end
