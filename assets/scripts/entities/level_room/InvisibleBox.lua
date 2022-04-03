
persistenceMode(TEMPLATE | ARGS, {"Transform", "BoxColliderShape"})

masks = include("scripts/entities/level_room/_masks")

defaultArgs({
    
    playerOnly = true

})

function create(box, args)

    setComponents(box, {
        RigidBody {
            mass = 0,
            collider = Collider {
                bounciness = 1,
                frictionCoefficent = 1,
                collisionCategoryBits = masks.FLY_WALLS,
                collideWithMaskBits = args.playerOnly and masks.DYNAMIC_CHARACTER or masks.DYNAMIC_PROPS | masks.DYNAMIC_CHARACTER,
            }
        },
    })

    component.Transform.getFor(box) -- might be loaded already.
    component.BoxColliderShape.getFor(box)

end
