local jacket = nil

render = function(deltaTime, showStats)
    if jacket == nil then
        jacket = gfx.CreateImage(result.jacketPath, 0)
    end
    gfx.BeginPath()
    gfx.TextAlign(0)
    gfx.FontSize(20)
    gfx.Text(result.title, 5, 20);
    gfx.Text(result.artist, 5, 40);
    gfx.Text("MAX COMBO: " .. tostring(result.maxCombo), 5, 60);
    gfx.Text("SCORE: " .. tostring(result.score), 5, 80);
    gfx.Text("GAUGE: " .. string.format("%1.f", result.gauge * 100) .. "%", 5, 100);
    gfx.Text("BPM: " .. result.bpm, 5, 120);
    local x = 0;
    for i = 1, #result.gaugeSamples do
       local sample = result.gaugeSamples[i];
       gfx.BeginPath()
       gfx.Rect(x,240,2,20);
       x = x + 2;
       gfx.FillColor(0, math.floor(sample * 255), 0);
       gfx.Fill()
    end
    gfx.ImageRect(5,140,100,100,jacket,1,0)
end