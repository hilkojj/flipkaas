
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_collision_masks")

function create(floor)

    setName(floor, "floor")
	
	setComponents(floor, {
		Transform {
            position = vec3(0, 0, 0),
            scale = vec3(100)
        },
        RigidBody {
            mass = 0,
            collider = Collider {
                bounciness = 1,
                frictionCoefficent = 1,
                collisionCategoryBits = collisionMasks.STATIC_TERRAIN,
                collideWithMaskBits = collisionMasks.DYNAMIC_PROPS | collisionMasks.DYNAMIC_CHARACTER,
            }
        },
        BoxColliderShape {
            halfExtents = vec3(100, 1, 100)
        },
        RenderModel {
            modelName = "Floor"
        },
        ShadowReceiver()
	})

end

