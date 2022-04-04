
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

    local hearts = {
        createChild(hud, "heart 0"),
        createChild(hud, "heart 1"),
        createChild(hud, "heart 2"),
        createChild(hud, "heart 3"),
    }
    for i, h in pairs(hearts) do

        setComponents(h, {
            UIElement {
                
                absolutePositioning = true,
                absoluteHorizontalAlign = 1,
                absoluteVerticalAlign = 0,
                renderOffset = ivec2(i * 16 - 40, -52)
            },
            AsepriteView {
                sprite = "sprites/ui/heart",
                loop = false
            }
        
        })

    end

    _G.updateHealthBar = function(left)
        
        for i = 4, left + 1, -1 do
            component.AsepriteView.getFor(hearts[i]).playingTag = 0
        end
        
    end

    function getTimeString()
	    local minutesStr = math.floor(getTime() / 60)..""
        local secsStr = math.floor(math.fmod(getTime(), 60))..""
        return (#minutesStr == 1 and "0"..minutesStr or minutesStr)..":"..(#secsStr == 1 and "0"..secsStr or secsStr)
    end


    local timeText = createChild(hud, "time text")
    applyTemplate(timeText, "Text", {
        text = "00:00",
        color = 2
    })
    setUpdateFunction(timeText, .5, function()
    
        component.TextView.getFor(timeText).text = getTimeString()
    
    end, false)


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
        6,
        5,
        15,
        100 -- :(
    }

    local totalEnergyEver = 0

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
        totalEnergyEver = totalEnergyEver + 1
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

    _G.showGameOverPopup = function(won)
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
        
        
        if not won then

        
            applyTemplate(createChild(popup, "gameovertext"), "Text", {
                text = "GAME OVER!\nGotta delay your death!",
                waving = true,
                wavingFrequency = .2,
                wavingSpeed = 20,
                wavingAmplitude = 2,
                lineSpacing = 0
            })

            
            applyTemplate(createChild(popup, "scoretext"), "Text", {
                text = "Your score: "..totalEnergyEver.."\n",
                lineSpacing = 10,
                color = colors.brick--5,
            })
        

            applyTemplate(createChild(popup, "retrybutton"), "Button", {
                text = "Retry",
                action = _G.retryLevel,
                center = true,
                fixedWidth = 80
            })
        else

            applyTemplate(createChild(popup, "gameovertext"), "Text", {
                text = "You won the game...\nIt was inevitable...",
                waving = true,
                wavingFrequency = .04,
                wavingSpeed = 10,
                wavingAmplitude = 3,
                lineSpacing = 0
            })

            applyTemplate(createChild(popup, "scoretext"), "Text", {
                text = "Your time: "..getTimeString().."\n",
                lineSpacing = 10,
                color = 2,
            })
            destroyEntity(timeText)
            
            applyTemplate(createChild(popup, "scoretext"), "Text", {
                text = "Your score: "..totalEnergyEver.."\n",
                lineSpacing = 10,
                color = colors.brick--5,
            })
        

        end
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