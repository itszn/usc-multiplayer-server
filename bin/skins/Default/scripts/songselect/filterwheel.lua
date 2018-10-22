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
local levelLabels = {}
local folderLabels = {}

render = function(deltaTime, shown)
    if not shown then
        return
    end
    resx,resy = game.GetResolution();
    gfx.FillColor(0,0,0,200)
    gfx.FastRect(0,0,resx,resy)
    gfx.BeginPath();
    gfx.LoadSkinFont("segoeui.ttf");
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    if selectingFolders then
        for i,f in ipairs(filters.folder) do
            if not folderLabels[i] then
               folderLabels[i] = gfx.CreateLabel(f, 40, 0) 
            end
            if i == selectedFolder then
                gfx.FillColor(255,255,255,255)
            else
                gfx.FillColor(255,255,255,128)
            end
            local xpos = 40 - (1 * math.abs(i - selectedFolder) ^ 2)
            local ypos = resy/2 + 40 * (i - selectedFolder)
            gfx.DrawLabel(folderLabels[i], xpos, ypos);
        end
    else
        for i,l in ipairs(filters.level) do
            if not levelLabels[i] then
               levelLabels[i] = gfx.CreateLabel(l, 40, 0) 
            end
            if i == selectedLevel then
                gfx.FillColor(255,255,255,255)
            else
                gfx.FillColor(255,255,255,128)
            end
            local xpos = 40 - (1 * math.abs(i - selectedLevel) ^ 2)
            local ypos = resy/2 + 40 * (i - selectedLevel)
            gfx.DrawLabel(levelLabels[i], xpos, ypos);
        end
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