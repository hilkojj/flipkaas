
function create(hud, args)

    setName(hud, "hud")

    setComponents(hud, {
        UIElement {
            absolutePositioning = true
        },
        UIContainer {
        --[[
            nineSliceSprite = "sprites/ui/hud_border",
        ]]--

            fillRemainingParentHeight = true,
            fillRemainingParentWidth = true,

            zIndexOffset = -10
        }
    })

    _G.showGameOverPopup = function(score)
        local popup = createEntity()
        setName(popup, "gameover popup")
        setComponents(popup, {
            UIElement {
                absolutePositioning = true,
                absoluteHorizontalAlign = 1,
                absoluteVerticalAlign = 2,
                renderOffset = ivec2(0, 0)
            },
            UIContainer {
                nineSliceSprite = "sprites/ui/9slice",

                fixedWidth = 200,
                fixedHeight = 130,
                centerAlign = true
            }
        })
        applyTemplate(createChild(popup, "gameovertext"), "Text", {
            text = "GAME OVER!\n",
            waving = true,
            wavingFrequency = .2,
            wavingSpeed = 20,
            wavingAmplitude = 2,
            lineSpacing = 10
        })
        applyTemplate(createChild(popup, "scoretext"), "Text", {
            text = "Your score: "..score.."\n",
            lineSpacing = 10,
            color = colors.brick--5,
        })
        applyTemplate(createChild(popup, "retrybutton"), "Button", {
            text = "Retry",
            action = _G.retryLevel,
            center = true,
            fixedWidth = 80
        })
        applyTemplate(createChild(popup, "menubutton"), "Button", {
            text = "Main menu",
            action = _G.goToMainMenu,
            newLine = true,
            center = true,
            fixedWidth = 80
        })
    end


    --applyTemplate(createEntity(), "AwfulMusic")
end