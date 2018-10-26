--Horizontal alignment
TEXT_ALIGN_LEFT 	= 1
TEXT_ALIGN_CENTER 	= 2
TEXT_ALIGN_RIGHT 	= 4
--Vertical alignment
TEXT_ALIGN_TOP 		= 8
TEXT_ALIGN_MIDDLE	= 16
TEXT_ALIGN_BOTTOM	= 32
TEXT_ALIGN_BASELINE	= 64

local jacket = nil;
local selectedIndex = 1
local selectedDiff = 1
local songCache = {}
local ioffset = 0
local doffset = 0
local diffColors = {{0,0,255}, {0,255,0}, {255,0,0}, {255, 0, 255}}
local timer = 0
local jacketFallback = gfx.CreateSkinImage("song_select/loading.png", 0)

game.LoadSkinSample("menu_click")
game.LoadSkinSample("click-02")

check_or_create_cache = function(song, loadJacket)
    if not songCache[song.id] then songCache[song.id] = {} end
    
    if not songCache[song.id]["title"] then
        songCache[song.id]["title"] = gfx.CreateLabel(song.title, 35, 0)
    end
    
    if not songCache[song.id]["artist"] then
        songCache[song.id]["artist"] = gfx.CreateLabel(song.artist, 25, 0)
    end
    
    if not songCache[song.id]["bpm"] then
        songCache[song.id]["bpm"] = gfx.CreateLabel(song.bpm, 30, 0)
    end
    
    if not songCache[song.id]["jacket"] and loadJacket then
        songCache[song.id]["jacket"] = gfx.CreateImage(song.difficulties[1].jacketPath, 0)
    end
end

draw_song = function(song, x, y, selected)
    check_or_create_cache(song)
    gfx.BeginPath()
    gfx.RoundedRectVarying(x,y, 700, 100,0,0,40,0)
    gfx.FillColor(30,30,30)
    gfx.StrokeColor(0,128,255)
    gfx.StrokeWidth(1)
    if selected then 
        gfx.StrokeColor(255,128,0)
        gfx.StrokeWidth(2)
    end
    gfx.Fill()
    gfx.Stroke()
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_TOP + gfx.TEXT_ALIGN_LEFT)
    gfx.DrawLabel(songCache[song.id]["title"], x+10, y + 10, 600)
    gfx.DrawLabel(songCache[song.id]["artist"], x+10, y + 50, 600)
    gfx.ForceRender()
end

draw_diff_icon = function(diff, x, y)
    gfx.BeginPath()
    gfx.RoundedRectVarying(x,y, 70, 70,0,0,20,0)
    gfx.FillColor(15,15,15)
    local r = diffColors[diff.difficulty + 1][1]
    local g = diffColors[diff.difficulty + 1][2]
    local b = diffColors[diff.difficulty + 1][3]
    gfx.StrokeColor(r,g,b)
    gfx.StrokeWidth(2)
    gfx.Fill()
    gfx.Stroke()
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_MIDDLE + gfx.TEXT_ALIGN_CENTER)
    gfx.FastText(tostring(diff.level), x+35,y+35, 20, 0)
end

draw_cursor = function(x,y,rotation,width)
    gfx.BeginPath();
    gfx.Translate(x,y)
    gfx.Rotate(rotation)
   -- gfx.MoveTo(-width/2 - 10, -20)
   -- gfx.LineTo(-width/2, 0)
   -- gfx.LineTo(-width/2 - 10, 20)
    gfx.StrokeColor(255,128,0)
    gfx.StrokeWidth(4)
   -- gfx.Stroke()
   -- gfx.BeginPath()
   -- gfx.MoveTo(width/2 + 10, -20)
   -- gfx.LineTo(width/2, 0)
   -- gfx.LineTo(width/2 + 10, 20)
   -- gfx.Stroke()
    gfx.Rect(-width/2, -width/2, width, width)
    gfx.Stroke()
    gfx.Rotate(-rotation)
    gfx.Translate(-x,-y)
end

draw_diffs = function(song, x, y)
    for i,d in ipairs(song.difficulties) do
        draw_diff_icon(d, 10 + x + 80 * (i - 1), y + 10)
    end
    draw_cursor(10 + x + 80 * (selectedDiff - 1 - doffset) + 35, y + 45, timer * math.pi, 70)
end

draw_selected = function(song, x, y)
    check_or_create_cache(song)
    local xpos = 20 - ((-ioffset) ^ 2) * 10
    local ypos = resy/2 - (ioffset) * 90
    draw_song(song, xpos, ypos, true)
    
    --song/diff details
    local diff = song.difficulties[selectedDiff]
    gfx.Translate(x,y)
    gfx.BeginPath()
    gfx.Rect(0,0,500,700)
    gfx.FillColor(30,30,30)
    gfx.Fill()
    if not songCache[song.id][selectedDiff] or songCache[song.id][selectedDiff] ==  jacketFallback then
        songCache[song.id][selectedDiff] = gfx.LoadImageJob(diff.jacketPath, jacketFallback, 200,200)
    end
    
    if songCache[song.id][selectedDiff] then
        gfx.BeginPath()
        gfx.ImageRect(100, 20, 300, 300, songCache[song.id][selectedDiff], 1, 0)
    end
    draw_diffs(song,10,340)
    gfx.TextAlign(gfx.TEXT_ALIGN_TOP + gfx.TEXT_ALIGN_LEFT)
    gfx.FillColor(255,255,255)
    gfx.FontSize(30)
    gfx.FastText("BPM:",10, 450)
    gfx.DrawLabel(songCache[song.id]["bpm"], 150, 450)
    gfx.FastText("Effector:",10, 490)
    gfx.FastText(diff.effector,150, 490)
end

render = function(deltaTime)
    timer = (timer + deltaTime)
    timer = timer % 2
    resx,resy = game.GetResolution();
    gfx.BeginPath();
    gfx.LoadSkinFont("segoeui.ttf");
    gfx.TextAlign(TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE);
    gfx.FontSize(40);
    gfx.FillColor(255,255,255);
    if songwheel.songs[1] ~= nil then
        --before selected
        for i = math.max(selectedIndex - 14, 1), math.max(selectedIndex - 1,1) do
            local song = songwheel.songs[i];
            local xpos = 20 - ((selectedIndex - i + ioffset) ^ 2) * 1
            local ypos = resy/2 - (selectedIndex - i + ioffset) * 40
            draw_song(song, xpos, ypos)
        end
        
        --after selected
        for i = math.min(selectedIndex + 14, #songwheel.songs), selectedIndex + 1,-1 do
            local song = songwheel.songs[i]
            local xpos = 20 - ((i - selectedIndex - ioffset) ^ 2) * 1
            local ypos = resy/2 - (selectedIndex - i + ioffset) * 40
            local alpha = 255 - (selectedIndex - i + ioffset) * 31
            draw_song(song, xpos, ypos)
        end
        
        --selected
        draw_selected(songwheel.songs[selectedIndex], 750, resy/2 - 350)
    end
    ioffset = ioffset * 0.8
    doffset = doffset * 0.8
    gfx.ResetTransform()
    gfx.ForceRender()
end

set_index = function(newIndex)
    if newIndex ~= selectedIndex then
        game.PlaySample("menu_click")
    end
    ioffset = ioffset + selectedIndex - newIndex
    selectedIndex = newIndex
end;

set_diff = function(newDiff)
    if newDiff ~= selectedDiff then
        game.PlaySample("click-02")
    end
    doffset = doffset + newDiff - selectedDiff
    selectedDiff = newDiff
end;