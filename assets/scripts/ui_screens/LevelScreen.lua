
_G.hudScreen = currentEngine

onEvent("BeforeDelete", function()
    loadOrCreateLevel(nil)
    if _G.hudScreen == currentEngine then
        _G.hudScreen = nil
    end
end)

if _G.levelToLoad == nil then
    error("_G.levelToLoad is nil")
end

local levelRestarter = createEntity()
listenToKey(levelRestarter, gameSettings.keyInput.retryLevel, "retry_key")
onEntityEvent(levelRestarter, "retry_key_pressed", function()
    closeActiveScreen()
    openScreen("scripts/ui_screens/LevelScreen")
end)

loadOrCreateLevel(_G.levelToLoad)

setComponents(createEntity(), {
    UIElement(),
    TextView {
        text = "Deeptris v0.1",
        fontSprite = "sprites/ui/default_font"
    }
})
