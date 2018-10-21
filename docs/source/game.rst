Game
====
Functions available under the `game` table, these are available in every script.

Constants
*********

+--------------------+------------------------------+
|    Lua Name        |         Value                |
+====================+==============================+
|LOGGER_INFO         | Logger::Severity::Info       |
+--------------------+------------------------------+
|LOGGER_NORMAL       | Logger::Severity::Normal     |
+--------------------+------------------------------+
|LOGGER_WARNING      | Logger::Severity::Warning    |
+--------------------+------------------------------+
|LOGGER_ERROR        | Logger::Severity::Error      |
+--------------------+------------------------------+


GetMousePos()
*************
Returns the mouse's position on the game window in pixel coordinates.

Example::

    mposx,mposy = game.GetMousePos();


GetResolution()
***************
Returns the game window's resolution in pixels.

Example::

    resx,resy = game.GetResolution();


Log(char* message, int severity)
********************************
Logs a message to the game's log file.

Example::

    game.Log("Something went wrong!", game.LOGGER_ERROR)