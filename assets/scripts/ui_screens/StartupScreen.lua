
_G.titleScreen = true

_G.joinedSession = false

startupArgs = getGameStartupArgs()
if not _G.joinedSession then

    saveGamePath = startupArgs["--single-player"] or "saves/default_save.dibdab"
    startSinglePlayerSession(saveGamePath)

    username = startupArgs["--username"] or "poopoo"
    joinSession(username, function(declineReason)

        tryCloseGame()
        error("couldn't join session: "..declineReason)
    end)
    _G.joinedSession = true
end

onEvent("BeforeDelete", function()
    print("startup screen done..")
end)

function startLevel()
    if screenTransitionStarted then
        return
    end

    startScreenTransition("transitions/screen_transition1", "shaders/ui/transition_cutoff")
    onEvent("ScreenTransitionStartFinished", function()

        closeActiveScreen()
        openScreen("scripts/ui_screens/LevelScreen")
    end)
end

setComponents(createEntity(), {
    UIElement {
        absolutePositioning = true
    },
    UIContainer {
        --nineSliceSprite = "sprites/ui/hud_border",
        fillRemainingParentHeight = true,
        fillRemainingParentWidth = true,

        zIndexOffset = -10
    }
})

local popup = createEntity()
setName(popup, "popup")
setComponents(popup, {
    UIElement {
        absolutePositioning = true,
        absoluteHorizontalAlign = 1,
        absoluteVerticalAlign = 2,
        renderOffset = ivec2(0, 10)
    },
    UIContainer {
        nineSliceSprite = "sprites/ui/9slice",

        fixedWidth = 200,
        fixedHeight = 100,
        centerAlign = true
    }
})

applyTemplate(createChild(popup, "gameovertext"), "Text", {
    text = "HapMan - The Inevitable.\n",
    waving = true,
    wavingFrequency = .04,
    wavingSpeed = 10,
    wavingAmplitude = 3,
    lineSpacing = 4
})

applyTemplate(createChild(popup, "cptext"), "Text", {
    text = "(C) 2022\n\n",
    color = 2
})


function difficultyBtn(name, difficulty)
    local btn = createChild(popup)
    applyTemplate(btn, "Button", {
        text = name,
        action = function()
            _G.difficulty = difficulty
            startLevel()
        end
    })
    component.UIElement.getFor(btn).margin = ivec2(0, 0)
end
--difficultyBtn("Easy", 6)
difficultyBtn("Play", 9)
--difficultyBtn("Hell", 12)


endScreenTransition("transitions/screen_transition0", "shaders/ui/transition_cutoff")
loadOrCreateLevel("assets/levels/title_screen.lvl")

--applyTemplate(createEntity(), "AwfulMusic")