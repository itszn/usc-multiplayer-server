Result Screen
=============
The following fields are available under the ``result`` table:

.. code-block:: c#

    int score
    float gauge //value of the gauge at the end of the song
    int misses
    int goods
    int perfects
    int maxCombo
    int level
    int difficulty
    string title
    string artist
    string effector
    string bpm
    string jacketPath
    int medianHitDelta
    float meanHitDelta
    int earlies
    int lates
    int badge //same as song wheel badge (except 0 which means the user manually exited)
    float gaugeSamples[256] //gauge values sampled throughout the song
    string grade // "S", "AAA+", "AAA", etc.
    score[] highScores // Same as song wheel scores 

Calls made to lua
*****************
Calls made from the game to the script, these need to be defined for the game
to function properly.

get_capture_rect()
^^^^^^^^^^^^^^^^^^
The region of the screen to be saved in score screenshots.

Has to return ``x,y,w,h`` in pixel coordinates to the game.