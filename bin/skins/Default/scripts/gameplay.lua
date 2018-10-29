local score = 0
local combo = 0
local jacket = nil
local resx,resy = game.GetResolution()
local portrait = resy > resx
local desw = 1280 --design width
if portrait then desw = 720 end
local critLinePos = { 0.95, 0.73 };
local desh = desw * (resy / resx) --design height
local scale = resx / desw
local songInfoWidth = 400
local jacketWidth = 100
local comboScale = 1.0
local late = false
local earlateTimer = 0;
local earlateColors = { {255,255,0}, {0,255,255} }
local alertTimers = {0,0}
local title = nil
local artist = nil
local jacketFallback = gfx.CreateSkinImage("song_select/loading.png", 0)
local bottomFill = gfx.CreateSkinImage("fill_bottom.png",0)
local topFill = gfx.CreateSkinImage("fill_top.png",0)

drawSongInfo = function(deltaTime)
    if jacket == nil or jacket == jacketFallback then
        jacket = gfx.LoadImageJob(gameplay.jacketPath, jacketFallback)
    end
    gfx.BeginPath()
    gfx.LoadSkinFont("segoeui.ttf")
    gfx.Translate(5,5) --upper left margin
    gfx.FillColor(20,20,20,200);
    gfx.Rect(0,0,songInfoWidth,100)
    gfx.Fill()
    gfx.BeginPath()
    gfx.ImageRect(0,0,jacketWidth,jacketWidth,jacket,1,0)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT)
    gfx.FontSize(30)
    local textX = jacketWidth + 10
    titleWidth = songInfoWidth - jacketWidth - 20
    gfx.Save()
    x1,y1,x2,y2 = gfx.TextBounds(0,0,gameplay.title)
    textscale = math.min(titleWidth / x2, 1)
    gfx.Translate(textX, 30)
    gfx.Scale(textscale, textscale)
    gfx.Text(gameplay.title, 0, 0)
    gfx.Restore()
    x1,y1,x2,y2 = gfx.TextBounds(0,0,gameplay.artist)
    textscale = math.min(titleWidth / x2, 1)
    gfx.Save()
    gfx.Translate(textX, 60)
    gfx.Scale(textscale, textscale)
    gfx.Text(gameplay.artist, 0, 0)
    gfx.Restore()
    gfx.FillColor(255,255,255)
    gfx.FontSize(20)
    gfx.Text(string.format("BPM: %.1f | HiSpeed: %.0f x %.1f = %.0f", 
        gameplay.bpm, gameplay.bpm, gameplay.hispeed, gameplay.bpm * gameplay.hispeed), 
        textX, 85)
    gfx.BeginPath()
    gfx.FillColor(0,150,255)
    gfx.Rect(jacketWidth,jacketWidth-10,(songInfoWidth - jacketWidth) * gameplay.progress,10)
    gfx.Fill()
    gfx.Translate(-5,-5) --undo margin
end

drawScore = function(deltaTime)
    gfx.BeginPath()
    gfx.LoadSkinFont("NovaMono.ttf")
    gfx.BeginPath()
    gfx.RoundedRectVarying(desw - 210,5,220,62,0,0,0,20)
    gfx.FillColor(20,20,20)
    gfx.StrokeColor(0,128,255)
    gfx.StrokeWidth(2)
    gfx.Fill()
    gfx.Stroke()
    gfx.Translate(-5,5) -- upper right margin
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_TOP)
    gfx.FontSize(60)
    gfx.Text(string.format("%08d", score),desw,0)
    
    gfx.Translate(5,-5) -- undo margin
end

drawGauge = function(deltaTime)
    local height = 1024 * scale * 0.35
    local width = 512 * scale * 0.35
    local posy = resy / 2 - height / 2
    local posx = resx - width
    if portrait then
        width = width * 0.8
        height = height * 0.8
        posy = posy - 30
        posx = resx - width
    end
    gfx.DrawGauge(gameplay.gauge, posx, posy, width, height, deltaTime);
end

drawCombo = function(deltaTime)
    if combo == 0 then return end
    local posx = desw / 2
    local posy = desh * critLinePos[1] - 100
    if portrait then posy = desh * critLinePos[2] - 100 end
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

drawEarlate = function(deltaTime)
    earlateTimer = math.max(earlateTimer - deltaTime,0)
    if earlateTimer == 0 then return nil end
    local alpha = math.floor(earlateTimer * 20) % 2
    alpha = alpha * 200 + 55
    gfx.BeginPath()
    gfx.FontSize(35)
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER, gfx.TEXT_ALIGN_MIDDLE)
    local ypos = desh * critLinePos[1] - 150
    if portrait then ypos = desh * critLinePos[2] - 150 end
    if late then
        gfx.FillColor(0,255,255, alpha)
        gfx.Text("LATE", desw / 2, ypos)
    else
        gfx.FillColor(255,0,255, alpha)
        gfx.Text("EARLY", desw / 2, ypos)
    end
end

drawFill = function(deltaTime)
    bw,bh = gfx.ImageSize(bottomFill)
    bottomAspect = bh/bw
    bottomHeight = desw * bottomAspect
    gfx.Rect(0, desh * critLinePos[2] + bottomHeight - 20, desw, 100)
    gfx.ImageRect(0, desh * critLinePos[2],  desw, bottomHeight, bottomFill, 1,0)


    gfx.BeginPath()
    tw,th = gfx.ImageSize(topFill)
    topAspect = th/tw
    topHeight = desw * topAspect
    local ar = desh/desw
    ar = ar - 1
    ar = ar / (16/9 - 1)
    ar = 1 - ar
    local yoff = ar * topHeight
    local retoff = ar * 20
    if ar < 0 then yoff = 0 end
    gfx.ImageRect(0,-yoff,desw, topHeight, topFill, 1, 0)
    gfx.BeginPath()
    return topHeight - yoff - retoff
end

drawAlerts = function(deltaTime)
    alertTimers[1] = math.max(alertTimers[1] - deltaTime,0)
    alertTimers[2] = math.max(alertTimers[2] - deltaTime,0)
    if alertTimers[1] > 0 then --draw left alert
        gfx.Save()
        local posx = desw / 2 - 350
        local posy = desh * critLinePos[1] - 135
        if portrait then 
            posy = desh * critLinePos[2] - 135 
            posx = 65
        end
        gfx.Translate(posx,posy)
        r,g,b = game.GetLaserColor(0)
        local alertScale = (-(alertTimers[1] ^ 2.0) + (1.5 * alertTimers[1])) * 5.0
        alertScale = math.min(alertScale, 1)
        gfx.Scale(1, alertScale)
        gfx.BeginPath()
        gfx.RoundedRectVarying(-50,-50,100,100,20,0,20,0)
        gfx.StrokeColor(r,g,b)
        gfx.FillColor(20,20,20)
        gfx.StrokeWidth(2)
        gfx.Fill()
        gfx.Stroke()
        gfx.BeginPath()
        gfx.FillColor(r,g,b)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE)
        gfx.FontSize(90)
        gfx.Text("L",0,0)
        gfx.Restore()
    end
    if alertTimers[2] > 0 then --draw right alert
        gfx.Save()
        local posx = desw / 2 + 350
        local posy = desh * critLinePos[1] - 135
        if portrait then 
            posy = desh * critLinePos[2] - 135 
            posx = desw - 65
        end
        gfx.Translate(posx,posy)
        r,g,b = game.GetLaserColor(1)
        local alertScale = (-(alertTimers[2] ^ 2.0) + (1.5 * alertTimers[2])) * 5.0
        alertScale = math.min(alertScale, 1)
        gfx.Scale(1, alertScale)
        gfx.BeginPath()
        gfx.RoundedRectVarying(-50,-50,100,100,0,20,0,20)
        gfx.StrokeColor(r,g,b)
        gfx.FillColor(20,20,20)
        gfx.StrokeWidth(2)
        gfx.Fill()
        gfx.Stroke()
        gfx.BeginPath()
        gfx.FillColor(r,g,b)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_MIDDLE)
        gfx.FontSize(90)
        gfx.Text("R",0,0)
        gfx.Restore()
    end
end

render = function(deltaTime)
    gfx.Scale(scale,scale)
    local yshift = 0
    if portrait then yshift = drawFill(deltaTime) end
    gfx.Translate(0, yshift)
    drawSongInfo(deltaTime)
    drawScore(deltaTime)
    gfx.Translate(0, -yshift)
    drawGauge(deltaTime)
    drawEarlate(deltaTime)
    drawCombo(deltaTime)
    drawAlerts(deltaTime)
end

update_score = function(newScore)
    score = newScore
end

update_combo = function(newCombo)
    combo = newCombo
    comboScale = 1.5
end

near_hit = function(wasLate) --for updating early/late display
    late = wasLate
    earlateTimer = 0.75
end

laser_alert = function(isRight) --for starting laser alert animations
    if isRight then alertTimers[2] = 1.5
    else alertTimers[1] = 1.5
    end
end