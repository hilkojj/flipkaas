
persistenceMode(TEMPLATE | ARGS, {"Transform"})

masks = include("scripts/entities/level_room/_masks")

defaultArgs({
	
})

function create(e, args)
	
	args.radius = 18.8
	args.gravityRadius = 50

    setName(e, "apple planet")
	
	local grav = createChild(e, "gravity field")


	setComponents(e, {
		RigidBody {
			mass = 0,
			collider = Collider {
				bounciness = 1,
                frictionCoefficent = 1,
				collisionCategoryBits = masks.STATIC_TERRAIN,
				collideWithMaskBits = masks.DYNAMIC_CHARACTER | masks.DYNAMIC_PROPS
			}
		},
		SphereColliderShape {
			radius = args.radius
		},
		RenderModel {
			modelName = "Apple"
		},
		CustomShader {
            vertexShaderPath = "shaders/default.vert",
            fragmentShaderPath = "shaders/default.frag",
            defines = {BITE = "1", APPLE = "1"},
			uniformsFloat = { biteZ = 75, cartoonyFresnel = 1.,
			
				holeTime0 = 0,
				holeTime1 = 0,
				holeTime2 = 0,
				holeTime3 = 0,
				holeTime4 = 0,
				holeTime5 = 0,
				holeTime6 = 0,
				holeTime7 = 0,
			
			},
			uniformsVec3 = {

				holeDir0 = vec3(1, 0, 0),
				holeDir1 = vec3(0, 0, 0),
				holeDir2 = vec3(0, 0, 0),
				holeDir3 = vec3(0, 0, 0),
				holeDir4 = vec3(0, 0, 0),
				holeDir5 = vec3(0, 0, 0),
				holeDir6 = vec3(0, 0, 0),
				holeDir7 = vec3(0, 0, 0),

			}
        },
		ShadowCaster(),
		ShadowReceiver()
	})
	
	setUpdateFunction(e, .05, function()
	
		local uniforms = component.CustomShader.getFor(e):dirty().uniformsFloat
		uniforms["biteZ"] = _G.biteZ
		--print(uniforms["biteZ"])
	
	end)

	for holeI = 0, 7 do
		
		local updateHole = nil
		updateHole = function()

			local rot = quat:new()
			local holeDir = getAppleHoleDir(holeI, _G.player, e, rot)

			local shader = component.CustomShader.getFor(e):dirty()
			
			shader.uniformsVec3["holeDir"..holeI] = holeDir
			shader.uniformsFloat["holeTime"..holeI] = getTime()

			if holeDir ~= vec3(0) then
				
				setTimeout(e, 1.4, function()
					
					local appleTrans = component.Transform.getFor(e)

					local worm = createChild(e)
					applyTemplate(worm, "Worm")

					setComponents(worm, 
					{
						Transform {
							position = appleTrans.position,
							rotation = rot
						}
					})

					component.Transform.animate(worm, "position", appleTrans.position + vec3(100) * holeDir, 10, "pow3Out", function()
			
						component.DespawnAfter.getFor(worm).time = 0
					end)
					component.Transform.animate(worm, "scale", vec3(1, 1, 0), 10, "pow4In")
				
				end)

			end
			
			setTimeout(e, 8, updateHole)
		end

		setTimeout(e, 8 + holeI * 1., updateHole)

	end

	component.Transform.getFor(e).scale = vec3(1)

	setComponents(grav, {
		
        Transform(),
		TransformChild {
			parentEntity = e
		},
		GhostBody {
			collider = Collider {
				collisionCategoryBits = masks.SENSOR,
				collideWithMaskBits = masks.DYNAMIC_CHARACTER | masks.DYNAMIC_PROPS,
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

	setComponents(createChild(e, "stick collider"), {
		
        Transform(),
		TransformChild {
			parentEntity = e,
			offset = Transform {
				position = vec3(0, args.radius, 0)
			}
		},
		RigidBody {
			mass = 0,
			collider = Collider {
				collisionCategoryBits = masks.STATIC_TERRAIN,
				collideWithMaskBits = masks.DYNAMIC_CHARACTER | masks.DYNAMIC_PROPS,
				registerCollisions = true
				-- todo: emit = false
			}
		},
		CapsuleColliderShape {
			sphereRadius = 2,
			sphereDistance = 10
		}
	})

end
