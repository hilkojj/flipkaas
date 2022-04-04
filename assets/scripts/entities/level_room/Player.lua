
function create(player)

    if not _G.titleScreen then
        applyTemplate(player, "Flyman")
    else
        applyTemplate(player, "Title")
    end

end
