render = function(deltaTime, shown)
    resx,resy = game.GetResolution();
    gfx.BeginPath();
    gfx.LoadSkinFont("segoeui.ttf");
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    if shown then
        gfx.FillColor(0,0,0,200)
        gfx.Rect(0,0,resx,resy)
        gfx.Fill()
        gfx.BeginPath()
        for i,setting in ipairs(settings) do
            gfx.FastText(string.format("%s: %s", setting.name, setting.value), resx/2, resy/2 + 40 * (i - settings.currentSelection), 40, gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE);
        end
    end
end