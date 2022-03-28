
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_collision_masks")

defaultArgs({
	donutRadius = 20,
	gravityRadius = 20
})

function create(e, args)
	
    setName(e, "donut planet")
	
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
		ConcaveColliderShape {
			meshName = "Donut"
		},
		RenderModel {
			modelName = "Donut"
		},
		ShadowCaster(),
		ShadowReceiver()
	})
	component.Transform.getFor(e).scale = vec3(args.donutRadius)

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
			halfExtents = vec3(args.donutRadius + args.gravityRadius, args.gravityRadius, args.donutRadius + args.gravityRadius)
		},
		GravityField {
			priority = 0
		},
		DonutGravityFunction {
			donutRadius = args.donutRadius,
			gravityRadius = args.gravityRadius
		}
	})

end
