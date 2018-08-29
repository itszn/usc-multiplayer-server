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
    if songwheel.songs ~= nil then
        gfx.Text(songwheel.songs[selectedIndex].title, resx/2, resy/2);
        if jacket == nil then
            jacket = gfx.CreateImage(songwheel.songs[1].path .. "/" .. songwheel.songs[1].difficulties[1].jacketPath, 0);
        end;
    end;
    if jacket ~= nil then
        gfx.ImageRect(0,0,100,100,jacket,1,0);
    end
end

set_index = function(newIndex)
    game.Log(songwheel.songs[newIndex].title, 3);
    selectedIndex = newIndex;
end;