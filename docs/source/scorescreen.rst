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
    float gaugeSamples[256] //gauge values sampled throughout the song
    string grade // "S", "AAA+", "AAA", etc.
    score[] highScores // Same as song wheel scores
