local mposx = 0;
local mposy = 0;
local hovered = 0;
local buttonWidth = 250;
local buttonHeight = 75;
local buttonBorder = 2;

--Horizontal alignment
TEXT_ALIGN_LEFT 	= 1;
TEXT_ALIGN_CENTER 	= 2;
TEXT_ALIGN_RIGHT 	= 4;
--Vertical alignment
TEXT_ALIGN_TOP 		= 8;
TEXT_ALIGN_MIDDLE	= 16;
TEXT_ALIGN_BOTTOM	= 32;
TEXT_ALIGN_BASELINE	= 64;

mouse_clipped = function(x,y,w,h)
    return mposx > x and mposy > y and mposx < x+w and mposy < y+h;
end;

draw_button = function(name, x, y, hoverindex)
    local rx = x - (buttonWidth / 2);
    local ty = y - (buttonHeight / 2);
    BeginPath();
    FillColor(0,128,255);
    if mouse_clipped(rx,ty, buttonWidth, buttonHeight) then
       hovered = hoverindex; 
       FillColor(255,128,0);
    end
    Rect(rx - buttonBorder,
        ty - buttonBorder, 
        buttonWidth + (buttonBorder * 2), 
        buttonHeight + (buttonBorder * 2));
    Fill();
    BeginPath();
    FillColor(40,40,40);
    Rect(rx, ty, buttonWidth, buttonHeight);
    Fill();
    BeginPath();
    FillColor(255,255,255);
    TextAlign(TEXT_ALIGN_CENTER + TEXT_ALIGN_MIDDLE);
    FontSize(40);
    Text(name, x, y);
end;

render = function(deltaTime)
    resx,resy = GetResolution();
    mposx,mposy = GetMousePos();
    buttonY = resy / 2;
    hovered = 0;
    draw_button("Start", resx / 2, buttonY, 1);
    buttonY = buttonY + 100;
    draw_button("Settings", resx / 2, buttonY, 2);
    buttonY = buttonY + 100;
    draw_button("Exit", resx / 2, buttonY, 3);
    
    BeginPath();
    FillColor(255,255,255);
    FontSize(120);
    Text("unnamed_sdvx_clone", resx / 2, resy / 2 - 150);
end;

mouse_pressed = function(button)
    return hovered;
end;