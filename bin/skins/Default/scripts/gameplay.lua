local score = 0
local combo = 0
local jacket = nil

render = function(deltaTime)
    if jacket == nil then
        jacket = gfx.CreateImage(gameplay.JacketPath, 0)
    end
    gfx.BeginPath()
    gfx.TextAlign(0)
    gfx.FontSize(20)
    gfx.Text(gameplay.Title, 5, 20);
    gfx.Text(gameplay.Artist, 5, 40);
    gfx.Text("COMBO: " .. tostring(combo), 5, 60);
    gfx.Text("SCORE: " .. tostring(score), 5, 80);
    gfx.Text("PROGRESS: " .. string.format("%.f", gameplay.progress * 100) .. "%", 5, 100);
    gfx.Text("HISPEED: " .. string.format("%.1f",gameplay.hispeed * gameplay.bpm), 5, 120);
    gfx.Text("BPM: " .. string.format("%.1f",gameplay.bpm), 5, 140);
    gfx.ImageRect(5,160,100,100,jacket,1,0)
end

update_score = function(newScore)
    score = newScore
end

update_combo = function(newCombo)
    combo = newCombo
end

near_hit = function(wasLate) --for updating early/late display

end

laser_alert = function(isRight) --for starting laser alert animations
    --if isRight == true then restart right alert animation
    --else restart left alert animation
end