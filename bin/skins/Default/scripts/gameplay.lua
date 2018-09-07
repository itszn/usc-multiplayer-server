local score = 0
local combo = 0
local jacket = nil
local resx,resy = game.GetResolution()
local scale = resx / 1280
local desw = 1280 --design width
local desh = desw * (resy / resx) --design height
local songInfoWidth = 400
local jacketWidth = 100
local comboScale = 1.0

drawSongInfo = function(deltaTime)
    if jacket == nil then
        jacket = gfx.CreateImage(gameplay.jacketPath, 0)
    end
    gfx.BeginPath()
    gfx.LoadSkinFont("segoeui.ttf")
    gfx.Translate(5,5) --upper left margin
    gfx.FillColor(100,100,100);
    gfx.Rect(0,0,songInfoWidth,100)
    gfx.Fill()
    gfx.BeginPath()
    gfx.ImageRect(0,0,jacketWidth,jacketWidth,jacket,1,0)
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER)
    gfx.FontSize(30)
    local textX = jacketWidth + (songInfoWidth - jacketWidth) / 2
    gfx.Text(gameplay.title, textX, 30);
    gfx.Text(gameplay.artist, textX, 60);
    gfx.BeginPath()
    gfx.FillColor(0,150,255)
    gfx.Rect(jacketWidth,jacketWidth-10,(songInfoWidth - jacketWidth) * gameplay.progress,10)
    gfx.Fill()
    gfx.Translate(-5,-5) --undo margin
end

drawScore = function(deltaTime)
    gfx.BeginPath()
    gfx.LoadSkinFont("NovaMono.ttf")
    gfx.Translate(-5,5) -- upper right margin
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_TOP)
    gfx.FontSize(40)
    gfx.Text(string.format("%08d", score),desw,0)
    
    gfx.Translate(5,-5) -- undo margin
end

drawGauge = function(deltaTime)
    local height = 1024 * scale * 0.35
    local width = 512 * scale * 0.35
    local posy = resy / 2 - height / 2
    local posx = resx - width
    gfx.DrawGauge(gameplay.gauge, posx, posy, width, height, deltaTime);
end

drawCombo = function(deltaTime)
    if combo == 0 then return end
    local posx = desw / 2
    local posy = desh * 0.8
    gfx.BeginPath()
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE)
    if gameplay.comboState == 2 then
        gfx.FillColor(100,255,0) --puc
    elseif gameplay.comboState == 1 then
        gfx.FillColor(255,200,0) --uc
    else
        gfx.FillColor(255,255,255) --regular
    end
    gfx.LoadSkinFont("NovaMono.ttf")
    gfx.FontSize(50 * math.max(comboScale, 1))
    comboScale = comboScale - deltaTime * 3
    gfx.Text(tostring(combo), posx, posy)
end

render = function(deltaTime)
    gfx.Scale(scale,scale)
    drawSongInfo(deltaTime)
    drawScore(deltaTime)
    drawGauge(deltaTime)
    drawCombo(deltaTime)
end

update_score = function(newScore)
    score = newScore
end

update_combo = function(newCombo)
    combo = newCombo
    comboScale = 1.5
end

near_hit = function(wasLate) --for updating early/late display

end

laser_alert = function(isRight) --for starting laser alert animations
    --if isRight == true then restart right alert animation
    --else restart left alert animation
end