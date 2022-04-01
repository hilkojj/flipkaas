
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_masks")

defaultArgs({
	gravityRadius = 6,
	height = 20
})

function create(e, args)
	
    setName(e, "cylinder planet")
	
	local grav = createChild(e, "gravity field")

	local prio = 0

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
			meshName = "Cylinder"
		},
		RenderModel {
			modelName = "Cylinder"
		},
		ShadowCaster(),
		ShadowReceiver()
	})
	component.Transform.getFor(e)

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
			halfExtents = vec3(args.gravityRadius, args.height * .5, args.gravityRadius)
		},
		GravityField {
			priority = prio
		},
		CylinderGravityFunction {
			gravityRadius = args.gravityRadius,
			height = args.height
		}
	})

	local cylToPlane = createChild(e, "cyl to plane")
	local cylToPlaneGrav = createChild(e, "cyl to plane gravity field")

	setComponents(cylToPlane, {
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
			meshName = "CylToPlane"
		},
		RenderModel {
			modelName = "CylToPlane"
		},
		ShadowCaster(),
		ShadowReceiver(),
		Transform(),
		TransformChild {
			parentEntity = e,
			offset = Transform {
				position = vec3(0, args.height * -.5 - 2, 0)
			}
		}
	})
	setComponents(cylToPlaneGrav, {
		
        Transform(),
		TransformChild {
			parentEntity = cylToPlane
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
			halfExtents = vec3(args.gravityRadius, 2, args.gravityRadius)
		},
		GravityField {
			priority = prio + 1
		},
		CylinderToPlaneGravityFunction {
			gravityRadius = args.gravityRadius,
			height = 4,
			cylinderRadius = 2
		}
	})

end
