
Song Wheel
============
Contains a list of songs accessed by `songwheel.songs`

Example for loading the jacket of the first diff for every song:

.. code-block:: lua

    for i,song in ipairs(songwheel.songs) do
        jackets[song.id] = gfx.CreateImage(song.difficulties[1].jacketPath, 0)
    end

Song
***************
A song contains the following fields:


.. code-block:: c#

    string title
    string artist
    string bpm //ex. "170-200"
    int id //unique static identifier
    string path //folder the song is stored in
    difficulty[] difficulties //array of all difficulties for this song
    
Difficulty
**********
A difficulty contains the following fields:


.. code-block:: c#

    string jacketPath
    int level
    int difficulty // 0 = nov, 1 = adv, etc.
    int id //unique static identifier
    string effector
    difficulty[] scores //array of all scores on this diff
    
    
Score
*****
A score contains the following fields:


.. code-block:: c#

    float gauge //range 0.0 -> 1.0
    int flags //contains information about gaugeType/random/mirror
    int score
    int perfects
    int goods
    int misses

get_page_size
*************
Function called by the game to get how much to scroll when page up or page down are pressed.
Needs to be defined for the game to work properly.