persistenceMode(TEMPLATE | ARGS, {"Transform"})

collisionMasks = include("scripts/entities/level_room/_masks")

function create(e)
	
    setName(e, "instanced dead flies")

	
	setComponents(e, {
		
        RenderModel {
            modelName = "DeadFly"
        },
		InstancedRendering {
		},
		CustomShader {
            vertexShaderPath = "shaders/default.vert",
            fragmentShaderPath = "shaders/default.frag",
            defines = {INSTANCED = "1"},
        },
		Transform {
			position = vec3(0, 80, component.Transform.getFor(getByName("plate planet")).position.z)
		},
		ShadowReceiver(),
		ShadowCaster()
	})
	
	local i = 0;
	local interval = 30 / 400

	setUpdateFunction(e, interval, function()
		
		i = i + 1

		local child = createChild(e, "instance "..i)

		setComponents(child, {

			Transform {
				position = component.Transform.getFor(e).position - vec3(0, 1, 0)
			},
			RigidBody {
				mass = 1,
				collider = Collider {
					bounciness = .5,
					frictionCoefficent = 1,
					collisionCategoryBits = collisionMasks.DYNAMIC_PROPS,
					collideWithMaskBits = collisionMasks.STATIC_TERRAIN | collisionMasks.DYNAMIC_PROPS | collisionMasks.DYNAMIC_CHARACTER,
				}
			},
			SphereColliderShape {
				radius = 2
			},
			DespawnAfter {
				time = 36.2 - i * interval
			}
		})
		
		component.InstancedRendering.getFor(e):dirty().transformEntities:add(child)

		if i == 400 then

			setUpdateFunction(e, 0, nil)

		end

		--[[
		setOnDestroyCallback(child, function()
			-- TODO: parent could be (about to be) destroyed, doing .getFor on a parent could cause a crash.

			component.InstancedRendering.getFor(e):dirty().transformEntities:erase(child)
		end)
		]]--

	end)

end