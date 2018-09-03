render = function(deltaTime, shown)
    resx,resy = game.GetResolution();
    gfx.BeginPath();
    gfx.LoadSkinFont("segoeui.ttf");
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    if shown then
        gfx.FastText("Settings wheel", resx/2, resy/2, 40, TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE);
    end
end