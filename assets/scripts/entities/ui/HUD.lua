
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

            zIndexOffset = -10,
            centerAlign = true
        }
    })
    local energyMeter = createChild(hud, "energy meter")
    setComponents(energyMeter, {
        UIElement {
            --[[
            absolutePositioning = true,
            absoluteHorizontalAlign = 1,
            absoluteVerticalAlign = 0,
            renderOffset = ivec2(0, 0)
            ]]--
        },
        AsepriteView {
            sprite = "sprites/ui/energy_meter",
            loop = false
        }
        
    })

    local stage = 0

    function canFly()

        local flyInstructions = createChild(hud, "fly instruction")
        applyTemplate(flyInstructions, "Text", {
            text = "Enter fly mode: ["..gameSettings.keyInput.fly:getName().."] or ",
            waving = true,
            wavingFrequency = 10,
            wavingSpeed = 10,
            wavingAmplitude = 2,
            lineSpacing = 10
        })
        local flycontrollerButtonIcon = createChild(hud, "controller fly button")
        setComponents(flycontrollerButtonIcon, {
            UIElement {
                --[[
                absolutePositioning = true,
                absoluteHorizontalAlign = 1,
                absoluteVerticalAlign = 0,
                ]]--
                renderOffset = ivec2(69, 15)

            },
            AsepriteView {
                sprite = "sprites/ui/xbox_buttons"
            }
        
        })
        playAsepriteTag(component.AsepriteView.getFor(flycontrollerButtonIcon), gameSettings.gamepadInput.fly:getName(), true)

        function pressed()
            --[[
            ]]--

            setTimeout(hud, 0, function()
                destroyEntity(flyInstructions)
                destroyEntity(flycontrollerButtonIcon)
            end)

            _G.goFly(stage)
            setStage(stage)
        end

        
        listenToGamepadButton(flyInstructions, 0, gameSettings.gamepadInput.fly, "flyController")
        listenToKey(flyInstructions, gameSettings.keyInput.fly, "flyKey")
        onEntityEvent(flyInstructions, "flyController_pressed", pressed)
        onEntityEvent(flyInstructions, "flyKey_pressed", pressed)

    end

    --stage = 1
    --canFly()


    local stageEnergies = {
        5,
        6
    }

    local energy = 0
    local energyNeeded = 0
    function setStage(stage)
        energy = 0
        energyNeeded = stageEnergies[stage + 1]
        playAsepriteTag(component.AsepriteView.getFor(energyMeter), "Reset", true)
    end
    _G.getStage = function()
        return stage
    end
    
    setStage(0)

    _G.increaseEnergyMeter = function()
        
        energy = energy + 1
        local frame = math.floor(energy / energyNeeded * 16)
        if frame == 0 and energy > 0 then
            frame = 1
        end
        frame = math.min(16, frame)

        local view = component.AsepriteView.getFor(energyMeter)
        view.playingTag = -1
        view.frame = frame

        if energy == energyNeeded then

            stage = stage + 1
            
            setTimeout(hud, .1, canFly)
        end
    end

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