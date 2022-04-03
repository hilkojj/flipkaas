
masks = include("scripts/entities/level_room/_masks")

local instancer = nil
local i = 0

function create(e)

    i = i + 1
    --setName(e, "sprinkle"..i)

    --[[
    if instancer == nil then

        instancer = createEntity()
        setName(instancer, "sprinkle instancer")

        setComponents(instancer, {
		    InstancedRendering {
		    },
		    RenderModel {
                modelName = "Sprinkle"
            },
            ShadowCaster(),
            Transform()
        })

    end

    print("instancer sprinkle"..instancer)
    ]]--

	setComponents(e, {
        GhostBody {
            collider = Collider {
                bounciness = 1,
                frictionCoefficent = 1,
                collisionCategoryBits = masks.SENSOR,
                collideWithMaskBits = masks.DYNAMIC_CHARACTER,
                registerCollisions = true
            }
        },
        BoxColliderShape {
            halfExtents = vec3(2.5, 2, .7)
        },
        Transform {
            position = vec3(0, 10, 0)
        },
        RenderModel {
            modelName = "Sprinkle"
        },
        ShadowCaster(),
	})
    component.Transform.getFor(e)
    --component.InstancedRendering.getFor(instancer):dirty().transformEntities:add(e)

    local hit = false

    onEntityEvent(e, "Collision", function (col)
		if col.otherEntity == _G.player and not hit and #col.contactPoints > 0 then
            hit = true    

            component.DespawnAfter.getFor(e).time = 0
            --component.InstancedRendering.getFor(instancer):dirty().transformEntities:erase(e)

            _G.playerHit(1)

        end
	end)

end

