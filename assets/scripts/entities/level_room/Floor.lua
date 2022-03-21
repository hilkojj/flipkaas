
persistenceMode(TEMPLATE | ARGS, {"Transform"})

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
                bounciness = 1
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

