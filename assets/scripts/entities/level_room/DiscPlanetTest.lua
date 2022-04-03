
persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_masks")

loadColliderMeshes("assets/models/cookie.obj", false)

defaultArgs({
})

function create(e, args)

	args.radius = 32
	args.gravityRadius = 24
	
    setName(e, "cookie planet")
	
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
			meshName = "Cookie"
		},
		RenderModel {
			modelName = "Cookie"
		},
		CustomShader {
            vertexShaderPath = "shaders/default.vert",
            fragmentShaderPath = "shaders/default.frag",
            defines = {BITE = "1"},
			uniformsFloat = { biteZ = 75, cartoonyFresnel = .2 },
        },
		ShadowCaster(),
		ShadowReceiver()
	})
	component.Transform.getFor(e).scale = vec3(1)

	
	setUpdateFunction(e, .1, function()
	
		local uniforms = component.CustomShader.getFor(e):dirty().uniformsFloat
		uniforms["biteZ"] = _G.biteZ
	
	end)

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
			halfExtents = vec3(args.radius + args.gravityRadius, args.radius + args.gravityRadius, args.radius + args.gravityRadius)
		},
		GravityField {
			priority = 0
		},
		DiscGravityFunction {
			radius = args.radius,
			gravityRadius = args.gravityRadius,
			spherish = .18
		}
	})

	local playerEntered = false
	onEntityEvent(grav, "Collision", function (col)

		if col.otherEntity == _G.player and not playerEntered and getTime() > .1 then

			setUpdateFunction(grav, 0, function()
		
				local affected = component.GravityFieldAffected.getFor(_G.player)

				if affected.fields[1] == grav then

					print("arrived at cookie")
	
					playerEntered = true
					_G.arrivedAtStage()
					setUpdateFunction(grav, 0, nil)
				end
		
			end)
		end		


	end)
	onEntityEvent(grav, "CollisionEnded", function (col)

		if col.otherEntity == _G.player then

			setUpdateFunction(grav, 0, nil)
		end		


	end)



end
