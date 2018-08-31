--Horizontal alignment
TEXT_ALIGN_LEFT 	= 1;
TEXT_ALIGN_CENTER 	= 2;
TEXT_ALIGN_RIGHT 	= 4;
--Vertical alignment
TEXT_ALIGN_TOP 		= 8;
TEXT_ALIGN_MIDDLE	= 16;
TEXT_ALIGN_BOTTOM	= 32;
TEXT_ALIGN_BASELINE	= 64;

local jacket = nil;
local selectedIndex = 1;

render = function(deltaTime)
    resx,resy = game.GetResolution();
    gfx.BeginPath();
    gfx.LoadSkinFont("segoeui.ttf");
    gfx.TextAlign(TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    gfx.FillColor(255,255,255);
    if songwheel.songs[1] ~= nil then
        for i,song in ipairs(songwheel.songs) do
            offset = i - selectedIndex
            offset = offset * 50
            gfx.BeginPath();
            if i == selectedIndex then
                gfx.FastText("--> " .. song.title .. " <--", resx/2, resy/2 + offset, 40, TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE)
            else
                gfx.FastText(song.title, resx/2, resy/2 + offset, 40, TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE)
            end
        end
    end
    gfx.ResetTransform();
end

set_index = function(newIndex)
    game.Log(songwheel.songs[newIndex].title, 3);
    selectedIndex = newIndex;
end;