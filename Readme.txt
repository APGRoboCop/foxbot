===================================
   Foxbot Source code Readme
===================================

   Introduction
   ------------

Foxbot is primarily a bot for the Half-Life mod Team Fortress Classic(AKA TFC).
This guide should provide you with some basic information on how to
compile and/or modify the Foxbot source code.
Most of the guide is Operating System independant but there is also
a section specifically about compiling the Linux version of Foxbot.

==============================
   FoxBot Version History
==============================

--------------------------------------------------------------------------------
Version 0.791
August-07-2018
Updated by RoboCop
--------------------------------------------------------------------------------

- Removed unwanted tchar.h dependancies that are no longer required - Thanks to Globoss
- Linux build should now work for Assault and Capture Point maps due to syntax error fixed - Thanks to Globoss
- Fixed Write Strings and compile time for version.cpp that fails on GCC 6+
- Fixed extdll.h that appears to fail to compile on GCC 6+ for #ifndef min and max
- Added optimisation for faster float point for Win32 build
- Linux build optimised for floating-point arithmetic and tuned for newer arch
- bot_job_think.cpp should be fairly adjusted for the bots to choose their paths and tasks wisely
- h_export.cpp Linux 'h_Library' can now detect both old and new TFC server build (tfc.so or tfc_i386.so)
- Added teleport waypoints for baconbowl_r, chimkey_l and siege
- Added more map configs for rats maps, plus turkeyburgers2002, fishburgers and donutburgers to spawn 1 sniper per 4 teams
- Enhanced the map configs for the Assault & Defend maps, rock2 and flagrun to spawn the recommended bot classes for each team
- Enhanced the hunted waypoints to prevent the Red team and the Hunted wondering back and forth in entrance tunnel
- Added some more defensive waypoints for avanti, casbah, flagrun, chimkey_l and warpath
- Added some important conc/rocket jump waypoints for the stock maps, palermo, ksour, cornfield, baconbowl_r and sandbowl_r2
- Reduced the risk of bots invading enemy spawns for the stock maps, chimkey_l, tf2fort and ksour
- Reduced excessive waypoint and pathwaypoint quantity for rats, rats2 and rats3, rats_cmd, palermo and baconbowl_r
- Removed non recommended waypoints for dustbowl that includes sentry that are not suitable for defending CP3
- Fixed the waypoint problems for 55, openfire_lowgrens, xpress2k4, destroy_l, phantom, shutdown2, sandbowl_r2 and tf2fort
- Added new waypoints for bases2k3, momentum_l, mortality_l and hellion

Known bugs:-  
>> botdontmove, bot_chat, bot_bot_balance and bot_team_balance cvars may not be 100% operational
>> FoxBot appears to crash sometimes on Windows Listenservers

____________________________________________________________

   Compiling the source code
   -------------------------

Although there are a number of .cpp files in the Foxbot source code directory
you may not need to compile them all to make the Foxbot Metamod plugin.

Source files used when compiling:
   bot.cpp
   botcam.cpp
   bot_client.cpp
   bot_combat.cpp
   bot_job_assessors.cpp
   bot_job_functions.cpp
   bot_job_think.cpp
   bot_navigate.cpp
   bot_start.cpp
   dll.cpp
   engine.cpp
   h_export.cpp
   meta_api.cpp
   sdk_util.cpp
   util.cpp
   version.cpp
   waypoint.cpp

Directories to include when compiling:
   \hlsdk\metamod\metamod\
   \hlsdk\multiplayer\cl_dll\
   \hlsdk\multiplayer\common\
   \hlsdk\multiplayer\dlls\
   \hlsdk\multiplayer\engine\
   \hlsdk\multiplayer\pm_shared\
____________________________________________________________

   Compiling a Linux version of Foxbot
   -----------------------------------

The Linux version of Foxbot isn't hugely different from the Windows version.
But here are some important differences:

First of all, there is only a dedicated Half-Life server available for Linux.
As opposed to the choice of dedicated and listen servers you get with Windows.

Secondly, Foxbot uses a number of plain text configuration files and
scripts when running.
Windows text files use line endings of the type CRLF, whereas Linux text
files use line endings of the type LF.
So ideally you should make sure that all the text files that Foxbot uses
are in the right format for your Operating System.
There is a program available on Linux, called dos2unix, that can convert
several text files to the right format for you in one big batch.

You will also need a C++ compiler installed on your Linux machine.
The one most commonly used on Linux is part of GCC - the Gnu Compiler Collection.
You can test if you have it installed by typing this command at the console:
g++ --version
If you you are told that there is no such file or directory then you
will probably need to install it.

Finally, you will need a Linux Makefile in order to compile the Linux
version of the bots.
One is provided that should hopefully work on your machine without any
serious problems.
Just type the command 'make' from the main foxbot source code directory.

____________________________________________________________

   Notes on the Windows Version
   ----------------------------

With Windows there is a fairly wide variety of C++ compilers available.
A big problem is that the Half-Life SDK isn't compatible with a
number of them.

Compilers that have been known to work with the Half-Life SDK:
Bloodshed's Dev-cpp 4.9.9.2(which is a free compiler).
Microsofts Visual C++.
Mingw(I've heard this works but haven't tried it myself).

Compilers that complained about the Half-Life SDK:
Borlands free command line compiler(version 5.0).
Open Watcom C/C++ Version 1.8.

____________________________________________________________

   Useful Notes
   ------------

If you modify stuff in bot.h then you may want to re-compile _ALL_ the
source files.
Adding/modifying variables in the bot_t struct without a full re-compile
has been known to make the bots start acting really weird or for a crash
to occur.

If you are compiling both Windows and Linux versions of Foxbot then you
should be aware that the object files(files with an .o extension) from
each version are incompatible and may clash with each other.
For example the Linux GCC compiler will probably spit out errors if it
finds object files from the Windows version sitting around in the
source directory.

____________________________________________________________

   Other sources of information
   ----------------------------

Foxbot was built using Botmans HPB Bot source code as a basis.
So you might also find the HPB Bot source code and readme.txt file useful.

Also you might want to check out the Bots-United website at:
www.bots-united.com
It's a site geared towards bot development.
