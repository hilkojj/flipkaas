
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_collision_masks")

function create(e)
	
    setName(e, "sensor test")
		
	setComponents(e, {
		
        Transform {
			position = vec3(0, 5, 0)
		},
		GhostBody {
			collider = Collider {
				collisionCategoryBits = collisionMasks.SENSOR,
				collideWithMaskBits = collisionMasks.DYNAMIC_CHARACTER, -- | collisionMasks.DYNAMIC_PROPS,
				registerCollisions = true
			}
		},
		BoxColliderShape {
			halfExtents = vec3(10, 10, 10);
		},
		GravityField {
			priority = 0
		},
		SphereGravityFunction {
		}
	})

	onEntityEvent(e, "Collision", function (col)
		print("enter: "..getName(col.otherEntity))
	end)

	onEntityEvent(e, "CollisionEnded", function (col)
		print("left: "..getName(col.otherEntity))
	end)

end
