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
local songCache = {};


render = function(deltaTime)
    resx,resy = game.GetResolution();
    gfx.BeginPath();
    gfx.LoadSkinFont("segoeui.ttf");
    gfx.TextAlign(TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    gfx.FillColor(255,255,255);
    if songwheel.songs[1] ~= nil then
        for i,song in ipairs(songwheel.songs) do
            if not songCache[song.id] then songCache[song.id] = {} end
            if not songCache[song.id]["label"] then
                songCache[song.id]["label"] = gfx.CreateLabel(song.title, 40, 0)
            end
            if not songCache[song.id]["jacket"] then
                songCache[song.id]["jacket"] = gfx.CreateImage(song.difficulties[1].jacketPath, 0)
            end
            
            offset = i - selectedIndex
            offset = offset * 50
            gfx.BeginPath();
            gfx.TextAlign(TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE);
            gfx.DrawLabel(songCache[song.id]["label"], resx/2, resy/2 + offset)
            if i == selectedIndex and songCache[song.id]["jacket"] then
                gfx.BeginPath();
                gfx.ImageRect(0,0,100,100,songCache[song.id]["jacket"], 1, 0)
            end
        end
    end
    gfx.ResetTransform();
end

set_index = function(newIndex)
    game.Log( songwheel.songs[newIndex].difficulties[1].jacketPath, 3);
    selectedIndex = newIndex;
end;