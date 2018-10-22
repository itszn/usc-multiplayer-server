local jacket = nil
local resx,resy = game.GetResolution()
local scale = math.min(resx / 800, resy /800)
local gradeImg;
local desw = 800
local desh = 800
local moveX = 0
local moveY = 0
if resx / resy > 1 then
    moveX = resx / (2*scale) - 400
else
    moveY = resy / (2*scale) - 400
end

draw_line = function(x1,y1,x2,y2,w,r,g,b)
    gfx.BeginPath()
    gfx.MoveTo(x1,y1)
    gfx.LineTo(x2,y2)
    gfx.StrokeColor(r,g,b)
    gfx.StrokeWidth(w)
    gfx.Stroke()
end

draw_highscores = function()
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT)
    gfx.LoadSkinFont("segoeui.ttf")
    gfx.FontSize(30)
    gfx.Text("Highscores:",510,30)
    for i,s in ipairs(result.highScores) do
        gfx.TextAlign(gfx.TEXT_ALIGN_LEFT)
        gfx.BeginPath()
        local ypos =  60 + (i - 1) * 80
        gfx.RoundedRectVarying(510,ypos, 280, 70,0,0,35,0)
        gfx.FillColor(15,15,15)
        gfx.StrokeColor(0,128,255)
        gfx.StrokeWidth(2)
        gfx.Fill()
        gfx.Stroke()
        gfx.BeginPath()
        gfx.FillColor(255,255,255)
        gfx.FontSize(25)
        gfx.Text(string.format("#%d",i), 515, ypos + 25)
        gfx.LoadSkinFont("NovaMono.ttf")
        gfx.FontSize(60)
        gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_TOP)
        gfx.Text(string.format("%08d", s.score), 650, ypos + 3)
    end
end

draw_graph = function(x,y,w,h)
    gfx.BeginPath()
    gfx.Rect(x,y,w,h)
    gfx.FillColor(0,0,0,210)
    gfx.Fill()    
    gfx.BeginPath()
    gfx.MoveTo(x,y + h - h * result.gaugeSamples[1])
    for i = 2, #result.gaugeSamples do
        gfx.LineTo(x + i * w / #result.gaugeSamples,y + h - h * result.gaugeSamples[i])
    end
    gfx.StrokeWidth(2.0)
    gfx.StrokeColor(0,180,255)
    gfx.Stroke()
end

render = function(deltaTime, showStats)
    gfx.Scale(scale,scale)
    gfx.Translate(moveX,moveY)
    if jacket == nil then
        jacket = gfx.CreateImage(result.jacketPath, 0)
    end
    if not gradeImg then
        gradeImg = gfx.CreateSkinImage(string.format("score/%s.png", result.grade),0)
    end
    gfx.BeginPath()
    gfx.Rect(0,0,500,800)
    gfx.FillColor(30,30,30)
    gfx.Fill()
    
    --Title and jacket
    gfx.LoadSkinFont("segoeui.ttf")
    gfx.BeginPath()
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER)
    gfx.FontSize(30)
    gfx.Text(result.title, 250, 35)
    gfx.FontSize(20)
    gfx.Text(result.artist, 250, 60)
    if jacket then
        gfx.ImageRect(100,90,300,300,jacket,1,0)
    end
    draw_graph(100,300,300,90)
    gfx.BeginPath()
    gfx.ImageRect(340,330,60,60,gradeImg,1,0)
    gfx.BeginPath()
    gfx.FontSize(20)
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT + gfx.TEXT_ALIGN_MIDDLE)
    gfx.Text(string.format("%d%%", math.floor(result.gauge * 100)),410,390 - 90 * result.gauge)
    --Score data
    gfx.BeginPath()
    gfx.RoundedRect(120,400,500 - 240,60,30);
    gfx.FillColor(15,15,15)
    gfx.StrokeColor(0,128,255)
    gfx.StrokeWidth(2)
    gfx.Fill()
    gfx.Stroke()
    gfx.BeginPath()
    gfx.FillColor(255,255,255)
    gfx.LoadSkinFont("NovaMono.ttf")
    gfx.FontSize(60)
    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER + gfx.TEXT_ALIGN_TOP)
    gfx.Text(string.format("%08d", result.score), 250, 400)
    --Left Column
    gfx.TextAlign(gfx.TEXT_ALIGN_LEFT)
    gfx.FontSize(30)
    gfx.Text("CRIT:",10, 500);
    gfx.Text("NEAR:",10, 540);
    gfx.Text("ERROR:",10, 580);
    --Right Column
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT)
    gfx.FontSize(30)
    gfx.Text(string.format("%d", result.perfects),480, 500);
    gfx.Text(string.format("%d", result.goods),480, 540);
    gfx.Text(string.format("%d", result.misses),480, 580);
    --Separator Lines
    draw_line(10,505,480,505, 1.5, 255,150,0)
    draw_line(10,545,480,545, 1.5, 255,0,200)
    draw_line(10,585,480,585, 1.5, 255,0,0)
    
    gfx.LoadSkinFont("segoeui.ttf")

    gfx.TextAlign(gfx.TEXT_ALIGN_CENTER)
    gfx.FontSize(80)
    gfx.Text("Extra Stats",250,700)

    draw_highscores()
    
    
end