DoorController
==============

Hack Factory Open Access Controller Software

--------

Libraries are included for convince, unless otherwise noted, I did not write them.  

The Weigand and Weigand 2 libraries were modified to fix some issues, I would like to eventually have these 2 libraries combined into a single class that can be used for both instances.

This is the Arduino portion of the code.  This is designed to work with a Serial controller(RPi or other system) that takes the entries from the door readers, weather a fob read or keyboard entry, and perform the backend processing.  The backend system can then unlock/lock the doors by activating relays on the board.

