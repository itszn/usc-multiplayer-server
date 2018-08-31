--Horizontal alignment
TEXT_ALIGN_LEFT 	= 1;
TEXT_ALIGN_CENTER 	= 2;
TEXT_ALIGN_RIGHT 	= 4;
--Vertical alignment
TEXT_ALIGN_TOP 		= 8;
TEXT_ALIGN_MIDDLE	= 16;
TEXT_ALIGN_BOTTOM	= 32;
TEXT_ALIGN_BASELINE	= 64;

local selectingFolders = true;
local selectedLevel = 1;
local selectedFolder = 1;

render = function(deltaTime, shown)
    if not shown then
        return
    end
    resx,resy = game.GetResolution();
    gfx.BeginPath();
    gfx.LoadSkinFont("segoeui.ttf");
    gfx.TextAlign(TEXT_ALIGN_LEFT + TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    if selectingFolders then
        gfx.FastText(filters.folder[selectedFolder], 0, resy/2, 40, TEXT_ALIGN_LEFT + TEXT_ALIGN_MIDDLE);
    else
        gfx.FastText(filters.level[selectedLevel], 0, resy/2, 40, TEXT_ALIGN_LEFT + TEXT_ALIGN_MIDDLE);
    end
end

set_selection = function(newIndex, isFolder)
    if isFolder then
        selectedFolder = newIndex
    else
        selectedLevel = newIndex
    end
end

set_mode = function(isFolder)
    selectingFolders = isFolder
end