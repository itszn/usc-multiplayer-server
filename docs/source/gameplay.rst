Gameplay
========
The following fields are available under the ``gameplay`` table:

.. code-block:: c#

    string title
    string artist
    string jacketPath
    int difficulty
    int level
    float progress // 0.0 at the start of a song, 1.0 at the end
    float hispeed
    float bpm
    float gauge
    int comboState // 2 = puc, 1 = uc, 0 = normal
    
Example:    

.. code-block:: lua

    --Draw combo
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

    
Calls made to lua
*****************
These are functions the game calls in the gameplay lua script so they need to be defined in there. The reason for having these is mostly for updating and starting animations.

update_score(newScore)
^^^^^^^^^^^^^^^^^^^^^^
For updating the score in lua.

update_combo(newCombo)
^^^^^^^^^^^^^^^^^^^^^^
For updating the combo in lua.

near_hit(wasLate)
^^^^^^^^^^^^^^^^^
For updating early/late display.

laser_alert(isRight)
^^^^^^^^^^^^^^^^^^^^
For starting laser alert animations::

    if isRight == true then restart right alert animation
    else restart left alert animation
    
render_intro(deltaTime)
^^^^^^^^^^^^^^^^^^^^^^^
Function for rendering an intro or keeping an intro timer. This function will be
called every frame untill it returns ``true`` and never again after it has.

Example:

.. code-block:: lua

    render_intro = function(deltaTime)
        if not game.GetButton(game.BUTTON_STA) then
            introTimer = introTimer - deltaTime
        end
        introTimer = math.max(introTimer, 0)
        return introTimer <= 0
    end

render_outro(deltaTime, clearState)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Function for rendering an outro or keeping an outro timer.


This function gets called when the game has ended till the game has transitioned into
the result screen, the game starts transitioning when this function returns ``true``
for the first time.

``clearState`` tells this function if the player failed or cleared the game for example.
These are all the possible states::

    0 = Player manually exited the game
    1 = Failed
    2 = Cleared
    3 = Hard Cleared
    4 = Full Combo
    5 = Perfect