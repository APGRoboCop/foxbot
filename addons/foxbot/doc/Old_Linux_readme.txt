=================================================
          FoxBot v0.697 For TFC -- Linux
               http://www.foxbot.net
=================================================

NOTE: This is an older readme for the Linux version of foxbot 0.697.
It's included here for posterity(and because you may find it useful).

====================================================================

Nothing in game has been changed, this was just a port (see change_log)
There are a few notes that you need to know about before installing FoxBot.

- FoxBot is a Metamod ONLY plug-in, no standalone support. 
  For some reason when trying to load just foxbot in the liblist, it gave out a
  precache error.  
  It's not worth the time to fix this problem as Metamod should be installed if
  your running Linux anyways.

- To get FoxBot to use Neo-TF features, you must load Neo-TF a special way.
  Info below on how to install.


========================== How to install ==========================
General
  To get the files out of the tgz use your basic Linux commands (tar xvzf filename)
  Folder structure should be.. (hlds_l being your server install folder)

hlds_l/
hlds_l/foxbot
hlds_l/foxbot/tfc
hlds_l/foxbot/tfc/areas
hlds_l/foxbot/tfc/configs
hlds_l/foxbot/tfc/scripts
hlds_l/foxbot/tfc/waypoints

Metamod (and or) Adminmod
  All this takes is adding one line to the BOTTOM of the "tfc/addons/Metamod/plugins.ini" file...

linux ../foxbot/foxbot_MM_i586.so

Metamod (and) Neo-TF (and or) Adminmod
  First you must remove whatever lines you added to install Neo-TF, you either used
  Metamod or the liblist.
  If you installed Neo-TF into the liblist, you will need to install Metamod
  (www.metamod.org).  After that your liblist.gam
  should be configured to Metamod now. If you are using Metamod and Neo-TF
  (and or adminmod), open up the plugins.ini 
  file and remove the Neo-TF line.  Now Neo-TF has been removed from tfc, let's add
  it our special way that enables FoxBot 
  to use it.  Open or create if necessary an autoexec.cfg in the tfc folder.
  Add this line anywhere in it...

localinfo mm_gamedll addons/NeoTF/dlls/NeoTF_i486.so

  Now that's all we have to do to Neo-TF.  All we gota do now is edit the Metamod
  plugin file and add FoxBot to it.
  Open "tfc/addons/Metamod/plugins.ini" file and add this to the BOTTOM

linux ../foxbot/foxbot_MM_i586.so

  FoxBot and Neo-TF should now be installed and bots should be using Neo-TF features.


Thanks to florian from adminmod for heading up the port

Linux specific questions/help should be directed to the forums, 
problems/bugs should be emailed/pmed to me (Furytux@_REMOVE_ME_foxbot.net)

FoxBot's website: http://www.foxbot.net/
FoxBot's Forums: http://forums.foxbot.net/

Related Links
Metamod: http://www.Metamod.org
Adminmod: http://www.adminmod.org
Neo-TF: http://planetneotf.com
Linux Newbie Forum: http://linuxnewbie.org/
