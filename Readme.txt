===================================
   Foxbot Source code Readme
===================================

Foxbot has been developed by:

Tom, Aka RedFox
Jordan, Aka FURY
Jeremy, Aka DrEvil
Paul, Aka GoaT_RopeR
Grubber
Yuraj
Victor
JASMINE
Richard, Aka Zybby

Special thanks to:
RoboCop
Koala
Safety1st
Arkshine
Globoss
pizzahut
jeefo

   Introduction
   ------------

Foxbot is primarily a bot for the Half-Life mod Team Fortress Classic(AKA TFC).
This guide should provide you with some basic information on how to
compile and/or modify the Foxbot source code.
Most of the guide is Operating System independant but there is also
a section specifically about compiling the Linux version of Foxbot.

--------------------------------------------------------------------------------
Version 0.800
September-10-2021
Updated by RoboCop
--------------------------------------------------------------------------------

- Added new waypoints:-
	+ axlfly
	+ botspree
	+ dustbowl_old
	+ dustbowl2_v2
	+ gen_complex
	+ insideout
	+ lbdustbowl
	+ madcanyon
	+ mulch_dm2b1
	+ murderball1_3
	+ osaka_l
	+ osaka_r2
	+ rats2v2
	+ rock2_2way_r
	+ rock2_open_r
	+ sandbowl_r
	+ turbine
	+ warpath_r2

- Reinstated the 'foxbot_commander' feature

- Reduced fire delay for Sniper's Auto-Rifle

- Reduced further more redundant codes and added more C++14 support
	
- Reduced overflow issues and instability crashes

- Reduced Reachable Range to prevent waypoints creation from adding too many junctions

- Fixed Listenserver foxbot.cfg and other .cfg load failures

- Added some support for murderball, murderball1_3 and murderball-2002

- Fixed the unreachable waypoint in warpath as well as enhanced its other waypoints

- Reduced StartFrame stack by reducing some unwanted features and lines that exceed it
	
- Added more reaction time delay for bots to attack enemies as they were too quick to respond

- Improved dustbowl waypoint by removing rogue pathways in the Jump Point for Capture Point 1, 2 and 3
	
- bot_job_think.cpp adjusted for the Medic and Engineer bots to heal their team mates more often

- botdontmove, bot_chat, min_bots, max_bots, bot_bot_balance and bot_team_balance cvars should now operate properly

Known bugs:-

>> Soldier bots tend to struggle for rocket jumping in awkward areas that can block its way

>> Sniper bots tend to TK when Friendly Fire is on, when interacting with their team's sentries and teleports

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
