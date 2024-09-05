/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 ****/

#ifndef __TF_DEFS_H__
#define __TF_DEFS_H__

#include <cstdint>

 // instant damage
constexpr int DMG_GENERIC = 0;          // generic damage was done
constexpr int DMG_CRUSH = 1 << 0;       // crushed by falling or moving object
constexpr int DMG_BULLET = 1 << 1;      // shot
constexpr int DMG_SLASH = 1 << 2;       // cut, clawed, stabbed
constexpr int DMG_BURN = 1 << 3;        // heat burned
constexpr int DMG_FREEZE = 1 << 4;      // frozen
constexpr int DMG_FALL = 1 << 5;        // fell too far
constexpr int DMG_BLAST = 1 << 6;       // explosive blast damage
constexpr int DMG_CLUB = 1 << 7;        // crowbar, punch, headbutt
constexpr int DMG_SHOCK = 1 << 8;       // electric shock
constexpr int DMG_SONIC = 1 << 9;       // sound pulse shockwave
constexpr int DMG_ENERGYBEAM = 1 << 10; // laser or other high energy beam
constexpr int DMG_NEVERGIB = 1 << 12;   // with this bit OR'd in, no damage type will be able to gib victims upon death
constexpr int DMG_ALWAYSGIB = 1 << 13;  // with this bit OR'd in, any damage type can be made to gib victims upon death.
constexpr int DMG_DROWN = 1 << 14;      // Drowning
// time-based damage
constexpr int DMG_TIMEBASED = ~0x3fff; // mask for time-based damage

constexpr int DMG_PARALYZE = 1 << 15;     // slows affected creature down
constexpr int DMG_NERVEGAS = 1 << 16;     // nerve toxins, very bad
constexpr int DMG_POISON = 1 << 17;       // blood poisoning
constexpr int DMG_RADIATION = 1 << 18;    // radiation exposure
constexpr int DMG_DROWNRECOVER = 1 << 19; // drowning recovery
constexpr int DMG_ACID = 1 << 20;         // toxic chemicals or acid burns
constexpr int DMG_SLOWBURN = 1 << 21;     // in an oven
constexpr int DMG_SLOWFREEZE = 1 << 22;   // in a subzero freezer
constexpr int DMG_MORTAR = 1 << 23;       // Hit by air raid (done to distinguish grenade from mortar)

// these are the damage types that are allowed to gib corpses
#define DMG_GIB_CORPSE (DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB)

// these are the damage types that have client hud art
#define DMG_SHOWNHUD (DMG_POISON | DMG_ACID | DMG_FREEZE | DMG_SLOWFREEZE | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK)

//===========================================================================
// OLD OPTIONS.QC
//===========================================================================
#define DEFAULT_AUTOZOOM false
#define WEINER_SNIPER        // autoaiming for sniper rifle

constexpr unsigned char FLAME_MAXWORLDNUM = 20; // maximum number of flames in the world. DO NOT PUT BELOW 20.

//#define MAX_WORLD_PIPEBOMBS      15	// This is divided between teams - this is the most you should have on a net server
constexpr unsigned char MAX_PLAYER_PIPEBOMBS = 8; // maximum number of pipebombs any 1 player can have active
constexpr unsigned char MAX_PLAYER_AMMOBOXES = 3; // maximum number of ammoboxes any 1 player can have active

//#define MAX_WORLD_FLARES         9	// This is the total number of flares allowed in the world at one time
//#define MAX_WORLD_AMMOBOXES      20	// This is divided between teams - this is the most you should have on a net server
constexpr unsigned char GR_TYPE_MIRV_NO = 4;   // Number of Mirvs a Mirv Grenade breaks into
constexpr unsigned char GR_TYPE_NAPALM_NO = 8; // Number of flames napalm grenade breaks into (unused if net server)

#define MEDIKIT_IS_BIOWEAPON // Medikit acts as a bioweapon against enemies

constexpr unsigned char TEAM_HELP_RATE = 60; // used only if teamplay bit 64 (help team with lower score) is set.

// 60 is a mild setting, and won't make too much difference
	  // increasing it _decreases_ the amount of help the losing team gets
	  // Minimum setting is 1, which would really help the losing team

#define DISPLAY_CLASS_HELP true			// Change this to #OFF if you don't want the class help to
													// appear whenever a player connects
#define NEVER_TEAMFRAGS false				// teamfrags options always off
#define ALWAYS_TEAMFRAGS false			// teamfrags options always on
#define CHECK_SPEEDS true					// makes sure players aren't moving too fast
#define SNIPER_RIFLE_RELOAD_TIME 1.5f	// seconds

constexpr unsigned short MAPBRIEFING_MAXTEXTLENGTH = 512;
constexpr unsigned short PLAYER_PUSH_VELOCITY = 50; // Players push teammates if they're moving under this speed

// Debug Options
//#define MAP_DEBUG                     // Debug for Map code. I suggest running in a hi-res
// mode and/or piping the output from the server to a file.
#ifdef MAP_DEBUG
#define MDEBUG(x) x
#else
#define MDEBUG(x)
#endif
//#define VERBOSE                       // Verbose Debugging on/off

//===========================================================================
// OLD QUAKE Defs
//===========================================================================
// items
constexpr unsigned int IT_AXE = 4096;
constexpr unsigned int IT_SHOTGUN = 1;
constexpr unsigned int IT_SUPER_SHOTGUN = 2;
constexpr unsigned int IT_NAILGUN = 4;
constexpr unsigned int IT_SUPER_NAILGUN = 8;
constexpr unsigned int IT_GRENADE_LAUNCHER = 16;
constexpr unsigned int IT_ROCKET_LAUNCHER = 32;
constexpr unsigned int IT_LIGHTNING = 64;
constexpr unsigned int IT_EXTRA_WEAPON = 128;

constexpr unsigned int IT_SHELLS = 256;
constexpr unsigned int IT_NAILS = 512;
constexpr unsigned int IT_ROCKETS = 1024;
constexpr unsigned int IT_CELLS = 2048;

constexpr unsigned int IT_ARMOR1 = 8192;
constexpr unsigned int IT_ARMOR2 = 16384;
constexpr unsigned int IT_ARMOR3 = 32768;
constexpr unsigned int IT_SUPERHEALTH = 65536;

constexpr unsigned int IT_KEY1 = 131072;
constexpr unsigned int IT_KEY2 = 262144;

constexpr unsigned int IT_INVISIBILITY = 524288;
constexpr unsigned int IT_INVULNERABILITY = 1048576;
constexpr unsigned int IT_SUIT = 2097152;
constexpr unsigned int IT_QUAD = 4194304;
constexpr unsigned int IT_HOOK = 8388608;

constexpr unsigned int IT_KEY3 = 16777216; // Stomp invisibility
constexpr unsigned int IT_KEY4 = 33554432; // Stomp invulnerability

//===========================================================================
// TEAMFORTRESS Defs
//===========================================================================
// TeamFortress State Flags
constexpr unsigned int TFSTATE_GRENPRIMED = 1;           // Whether the player has a primed grenade
constexpr unsigned int TFSTATE_RELOADING = 2;            // Whether the player is reloading
constexpr unsigned int TFSTATE_ALTKILL = 4;              // #true if killed with a weapon not in self.weapon: NOT USED ANYMORE
constexpr unsigned int TFSTATE_RANDOMPC = 8;             // Whether Playerclass is random, new one each respawn
constexpr unsigned int TFSTATE_INFECTED = 16;            // set when player is infected by the bioweapon
constexpr unsigned int TFSTATE_INVINCIBLE = 32;          // Player has permanent Invincibility (Usually by GoalItem)
constexpr unsigned int TFSTATE_INVISIBLE = 64;           // Player has permanent Invisibility (Usually by GoalItem)
constexpr unsigned int TFSTATE_QUAD = 128;               // Player has permanent Quad Damage (Usually by GoalItem)
constexpr unsigned int TFSTATE_RADSUIT = 256;            // Player has permanent Radsuit (Usually by GoalItem)
constexpr unsigned int TFSTATE_BURNING = 512;            // Is on fire
constexpr unsigned int TFSTATE_GRENTHROWING = 1024;      // is throwing a grenade
constexpr unsigned int TFSTATE_AIMING = 2048;            // is using the laser sight
constexpr unsigned int TFSTATE_ZOOMOFF = 4096;           // doesn't want the FOV changed when zooming
constexpr unsigned int TFSTATE_RESPAWN_READY = 8192;     // is waiting for respawn, and has pressed fire
constexpr unsigned int TFSTATE_HALLUCINATING = 16384;    // set when player is hallucinating
constexpr unsigned int TFSTATE_TRANQUILISED = 32768;     // set when player is tranquilised
constexpr unsigned int TFSTATE_CANT_MOVE = 65536;        // set when player is setting a detpack
constexpr unsigned int TFSTATE_RESET_FLAMETIME = 131072; // set when the player has to have his flames increased in health

// Defines used by TF_T_Damage (see combat.qc)
constexpr unsigned char TF_TD_IGNOREARMOUR = 1; // Bypasses the armour of the target
constexpr unsigned char TF_TD_NOTTEAM = 2;      // Doesn't damage a team member (indicates direct fire weapon)
constexpr unsigned char TF_TD_NOTSELF = 4;      // Doesn't damage self

constexpr unsigned short TF_TD_OTHER = 0;       // Ignore armorclass
constexpr unsigned short TF_TD_SHOT = 1;        // Bullet damage
constexpr unsigned short TF_TD_NAIL = 2;        // Nail damage
constexpr unsigned short TF_TD_EXPLOSION = 4;   // Explosion damage
constexpr unsigned short TF_TD_ELECTRICITY = 8; // Electric damage
constexpr unsigned short TF_TD_FIRE = 16;       // Fire damage
constexpr unsigned short TF_TD_NOSOUND = 256;   // Special damage. Makes no sound/painframe, etc

/*==================================================*/
/* Toggleable Game Settings							*/
/*==================================================*/
constexpr unsigned char TF_RESPAWNDELAY1 = 5;  // seconds of waiting before player can respawn
constexpr unsigned char TF_RESPAWNDELAY2 = 10; // seconds of waiting before player can respawn
constexpr unsigned char TF_RESPAWNDELAY3 = 20; // seconds of waiting before player can respawn

constexpr unsigned short TEAMPLAY_NORMAL = 1;
constexpr unsigned short TEAMPLAY_HALFDIRECT = 2;
constexpr unsigned short TEAMPLAY_NODIRECT = 4;
constexpr unsigned short TEAMPLAY_HALFEXPLOSIVE = 8;
constexpr unsigned short TEAMPLAY_NOEXPLOSIVE = 16;
constexpr unsigned short TEAMPLAY_LESSPLAYERSHELP = 32;
constexpr unsigned short TEAMPLAY_LESSSCOREHELP = 64;
constexpr unsigned short TEAMPLAY_HALFDIRECTARMOR = 128;
constexpr unsigned short TEAMPLAY_NODIRECTARMOR = 256;
constexpr unsigned short TEAMPLAY_HALFEXPARMOR = 512;
constexpr unsigned short TEAMPLAY_NOEXPARMOR = 1024;
constexpr unsigned short TEAMPLAY_HALFDIRMIRROR = 2048;
constexpr unsigned short TEAMPLAY_FULLDIRMIRROR = 4096;
constexpr unsigned short TEAMPLAY_HALFEXPMIRROR = 8192;
constexpr unsigned short TEAMPLAY_FULLEXPMIRROR = 16384;

#define TEAMPLAY_TEAMDAMAGE (TEAMPLAY_NODIRECT | TEAMPLAY_HALFDIRECT | TEAMPLAY_HALFEXPLOSIVE | TEAMPLAY_NOEXPLOSIVE)
// FortressMap stuff
constexpr unsigned char TEAM1_CIVILIANS = 1;
constexpr unsigned char TEAM2_CIVILIANS = 2;
constexpr unsigned char TEAM3_CIVILIANS = 4;
constexpr unsigned char TEAM4_CIVILIANS = 8;

// Defines for the playerclass
constexpr unsigned char PC_UNDEFINED = 0;

constexpr unsigned char PC_SCOUT = 1;
constexpr unsigned char PC_SNIPER = 2;
constexpr unsigned char PC_SOLDIER = 3;
constexpr unsigned char PC_DEMOMAN = 4;
constexpr unsigned char PC_MEDIC = 5;
constexpr unsigned char PC_HVYWEAP = 6;
constexpr unsigned char PC_PYRO = 7;
constexpr unsigned char PC_SPY = 8;
constexpr unsigned char PC_ENGINEER = 9;

// Insert new class definitions here

// PC_RANDOM _MUST_ be the third last class
constexpr unsigned char PC_RANDOM = 10;    // Random playerclass
constexpr unsigned char PC_CIVILIAN = 11;  // Civilians are a special class. They cannot be chosen by players, only enforced by maps
constexpr unsigned char PC_LASTCLASS = 12; // Use this as the high-boundary for any loops

// through the playerclass.

constexpr unsigned char SENTRY_COLOR = 10; // will be in the PC_RANDOM slot for team colors

// These are just for the scanner
constexpr unsigned char SCAN_SENTRY = 13;
constexpr unsigned char SCAN_GOALITEM = 14;

// Values returned by CheckArea
constexpr unsigned char CAREA_CLEAR = 0;
constexpr unsigned char CAREA_BLOCKED = 1;
constexpr unsigned char CAREA_NOBUILD = 2;

/*==================================================*/
/* Impulse Defines		                        	*/
/*==================================================*/
// Alias check to see whether they already have the aliases
constexpr unsigned char TF_ALIAS_CHECK = 13;

// CTF Support Impulses
constexpr unsigned char HOOK_IMP1 = 22;
constexpr unsigned char FLAG_INFO = 23;
constexpr unsigned char HOOK_IMP2 = 39;

// Axe
constexpr unsigned char AXE_IMP = 40;

// Camera Impulse
constexpr unsigned char TF_CAM_TARGET = 50;
constexpr unsigned char TF_CAM_ZOOM = 51;
constexpr unsigned char TF_CAM_ANGLE = 52;
constexpr unsigned char TF_CAM_VEC = 53;
constexpr unsigned char TF_CAM_PROJECTILE = 54;
constexpr unsigned char TF_CAM_PROJECTILE_Z = 55;
constexpr unsigned char TF_CAM_REVANGLE = 56;
constexpr unsigned char TF_CAM_OFFSET = 57;
constexpr unsigned char TF_CAM_DROP = 58;
constexpr unsigned char TF_CAM_FADETOBLACK = 59;
constexpr unsigned char TF_CAM_FADEFROMBLACK = 60;
constexpr unsigned char TF_CAM_FADETOWHITE = 61;
constexpr unsigned char TF_CAM_FADEFROMWHITE = 62;

// Last Weapon impulse
constexpr unsigned char TF_LAST_WEAPON = 69;

// Status Bar Resolution Settings.  Same as CTF to maintain ease of use.
constexpr unsigned char TF_STATUSBAR_RES_START = 71;
constexpr unsigned char TF_STATUSBAR_RES_END = 81;

// Clan Messages
constexpr unsigned char TF_MESSAGE_1 = 82;
constexpr unsigned char TF_MESSAGE_2 = 83;
constexpr unsigned char TF_MESSAGE_3 = 84;
constexpr unsigned char TF_MESSAGE_4 = 85;
constexpr unsigned char TF_MESSAGE_5 = 86;

constexpr unsigned char TF_CHANGE_CLASS = 99; // Bring up the Class Change menu

// Added to PC_??? to get impulse to use if this clashes with your
// own impulses, just change this value, not the PC_??
constexpr unsigned char TF_CHANGEPC = 100;

// The next few impulses are all the class selections
// PC_SCOUT		101
// PC_SNIPER		102
// PC_SOLDIER	103
// PC_DEMOMAN	104
// PC_MEDIC		105
// PC_HVYWEAP	106
// PC_PYRO		107
// PC_RANDOM		108
// PC_CIVILIAN	109  // Cannot be used
// PC_SPY		110
// PC_ENGINEER	111

// Help impulses
constexpr unsigned char TF_DISPLAYLOCATION = 118;
constexpr unsigned char TF_STATUS_QUERY = 119;

constexpr unsigned char TF_HELP_MAP = 131;

// Information impulses
constexpr unsigned char TF_INVENTORY = 135;
constexpr unsigned char TF_SHOWTF = 136;
constexpr unsigned char TF_SHOWLEGALCLASSES = 137;

// Team Impulses
constexpr unsigned char TF_TEAM_1 = 140;       // Join Team 1
constexpr unsigned char TF_TEAM_2 = 141;       // Join Team 2
constexpr unsigned char TF_TEAM_3 = 142;       // Join Team 3
constexpr unsigned char TF_TEAM_4 = 143;       // Join Team 4
constexpr unsigned char TF_TEAM_CLASSES = 144; // Impulse to display team classes
constexpr unsigned char TF_TEAM_SCORES = 145;  // Impulse to display team scores
constexpr unsigned char TF_TEAM_LIST = 146;    // Impulse to display the players in each team.

// Grenade Impulses
constexpr unsigned char TF_GRENADE_1 = 150; // Prime grenade type 1
constexpr unsigned char TF_GRENADE_2 = 151; // Prime grenade type 2
constexpr unsigned char TF_GRENADE_T = 152; // Throw primed grenade

// Impulses for new items
// #define TF_SCAN				159		// Scanner Pre-Impulse
// #define TF_SCAN_10			162		// Scan using 10 enery (1 cell)
constexpr unsigned char TF_AUTO_SCAN = 159;     // Scanner On/Off
constexpr unsigned char TF_SCAN_ENEMY = 160;    // Impulses to toggle scanning of enemies
constexpr unsigned char TF_SCAN_FRIENDLY = 161; // Impulses to toggle scanning of friendlies
constexpr unsigned char TF_SCAN_SOUND = 162;    // Scanner sounds on/off
constexpr unsigned char TF_SCAN_30 = 163;       // Scan using 30 energy (2 cells)
constexpr unsigned char TF_SCAN_100 = 164;      // Scan using 100 energy (5 cells)
constexpr unsigned char TF_DETPACK_5 = 165;     // Detpack set to 5 seconds
constexpr unsigned char TF_DETPACK_20 = 166;    // Detpack set to 20 seconds
constexpr unsigned char TF_DETPACK_50 = 167;    // Detpack set to 50 seconds
constexpr unsigned char TF_DETPACK = 168;       // Detpack Pre-Impulse
constexpr unsigned char TF_DETPACK_STOP = 169;  // Impulse to stop setting detpack
constexpr unsigned char TF_PB_DETONATE = 170;   // Detonate Pipebombs

// Special skill
constexpr unsigned char TF_SPECIAL_SKILL = 171;

// Ammo Drop impulse
constexpr unsigned char TF_DROP_AMMO = 172;

// Reload impulse
constexpr unsigned char TF_RELOAD = 173;

// auto-zoom toggle
constexpr unsigned char TF_AUTOZOOM = 174;

// drop/pass commands
constexpr unsigned char TF_DROPKEY = 175;

// Select Medikit
constexpr unsigned char TF_MEDIKIT = 176;

// Spy Impulses
constexpr unsigned char TF_SPY_SPY = 177; // On net, go invisible, on LAN, change skin/color
constexpr unsigned char TF_SPY_DIE = 178; // Feign Death

// Engineer Impulses
constexpr unsigned char TF_ENGINEER_BUILD = 179;
constexpr unsigned char TF_ENGINEER_SANDBAG = 180;

// Medic
constexpr unsigned char TF_MEDIC_HELPME = 181;

// Status bar
constexpr unsigned char TF_STATUSBAR_ON = 182;
constexpr unsigned char TF_STATUSBAR_OFF = 183;

// Discard impulse
constexpr unsigned char TF_DISCARD = 184;

// ID Player impulse
constexpr unsigned char TF_ID = 185;

// Clan Battle impulses
constexpr unsigned char TF_SHOWIDS = 186;

// More Engineer Impulses
constexpr unsigned char TF_ENGINEER_DETDISP = 187;
constexpr unsigned char TF_ENGINEER_DETSENT = 188;

// Admin Commands
constexpr unsigned char TF_ADMIN_DEAL_CYCLE = 189;
constexpr unsigned char TF_ADMIN_KICK = 190;
constexpr unsigned char TF_ADMIN_BAN = 191;
constexpr unsigned char TF_ADMIN_COUNTPLAYERS = 192;
constexpr unsigned char TF_ADMIN_CEASEFIRE = 193;

// Drop Goal Items
constexpr unsigned char TF_DROPGOALITEMS = 194;

// More Admin Commands
constexpr unsigned char TF_ADMIN_NEXT = 195;

// More Engineer Impulses
constexpr unsigned char TF_ENGINEER_DETEXIT = 196;
constexpr unsigned char TF_ENGINEER_DETENTRANCE = 197;

// Yet MORE Admin Commands
constexpr unsigned char TF_ADMIN_LISTIPS = 198;

// Silent Spy Feign
constexpr unsigned char TF_SPY_SILENTDIE = 199;

/*==================================================*/
/* Defines for the ENGINEER's Building ability		*/
/*==================================================*/
// Ammo costs
constexpr unsigned char AMMO_COST_SHELLS = 2; // Metal needed to make 1 shell
constexpr unsigned char AMMO_COST_NAILS = 1;
constexpr unsigned char AMMO_COST_ROCKETS = 2;
constexpr unsigned char AMMO_COST_CELLS = 2;

// Building types
constexpr unsigned char BUILD_DISPENSER = 1;
constexpr unsigned char BUILD_SENTRYGUN = 2;
constexpr unsigned char BUILD_MORTAR = 3;
constexpr unsigned char BUILD_TELEPORTER_ENTRANCE = 4;
constexpr unsigned char BUILD_TELEPORTER_EXIT = 5;

// Building metal costs
constexpr unsigned char BUILD_COST_DISPENSER = 100; // Metal needed to build
constexpr unsigned char BUILD_COST_SENTRYGUN = 130;
constexpr unsigned char BUILD_COST_MORTAR = 150;
constexpr unsigned char BUILD_COST_TELEPORTER = 125;

constexpr unsigned char BUILD_COST_SANDBAG = 20; // Built with a separate alias

// Building times
constexpr unsigned char BUILD_TIME_DISPENSER = 2; // seconds to build
constexpr unsigned char BUILD_TIME_SENTRYGUN = 5;
constexpr unsigned char BUILD_TIME_MORTAR = 5;
constexpr unsigned char BUILD_TIME_TELEPORTER = 4;

// Building health levels
constexpr unsigned char BUILD_HEALTH_DISPENSER = 150; // Health of the building
constexpr unsigned char BUILD_HEALTH_SENTRYGUN = 150;
constexpr unsigned char BUILD_HEALTH_MORTAR = 200;
constexpr unsigned char BUILD_HEALTH_TELEPORTER = 80;

// Dispenser's maximum carrying capability
constexpr unsigned short BUILD_DISPENSER_MAX_SHELLS = 400;
constexpr unsigned short BUILD_DISPENSER_MAX_NAILS = 600;
constexpr unsigned short BUILD_DISPENSER_MAX_ROCKETS = 300;
constexpr unsigned short BUILD_DISPENSER_MAX_CELLS = 400;
constexpr unsigned short BUILD_DISPENSER_MAX_ARMOR = 500;

// Build state sent down to client
constexpr unsigned char BS_BUILDING = 1 << 0;
constexpr unsigned char BS_HAS_DISPENSER = 1 << 1;
constexpr unsigned char BS_HAS_SENTRYGUN = 1 << 2;
constexpr unsigned char BS_CANB_DISPENSER = 1 << 3;
constexpr unsigned char BS_CANB_SENTRYGUN = 1 << 4;
/*==================================================*/
/* Ammo quantities for dropping & dispenser use		*/
/*==================================================*/
constexpr unsigned char DROP_SHELLS = 20;
constexpr unsigned char DROP_NAILS = 20;
constexpr unsigned char DROP_ROCKETS = 10;
constexpr unsigned char DROP_CELLS = 10;
constexpr unsigned char DROP_ARMOR = 40;

/*==================================================*/
/* Team Defines				            			*/
/*==================================================*/
constexpr unsigned char TM_MAX_NO = 4; // Max number of teams. Simply changing this value isn't enough.

// A new global to hold new team colors is needed, and more flags
	 // in the spawnpoint spawnflags may need to be used.
	 // Basically, don't change this unless you know what you're doing :)

/*==================================================*/
/* New Weapon Defines		                        */
/*==================================================*/
constexpr unsigned int WEAP_HOOK = 1;
constexpr unsigned int WEAP_BIOWEAPON = 2;
constexpr unsigned int WEAP_MEDIKIT = 4;
constexpr unsigned int WEAP_SPANNER = 8;
constexpr unsigned int WEAP_AXE = 16;
constexpr unsigned int WEAP_SNIPER_RIFLE = 32;
constexpr unsigned int WEAP_AUTO_RIFLE = 64;
constexpr unsigned int WEAP_SHOTGUN = 128;
constexpr unsigned int WEAP_SUPER_SHOTGUN = 256;
constexpr unsigned int WEAP_NAILGUN = 512;
constexpr unsigned int WEAP_SUPER_NAILGUN = 1024;
constexpr unsigned int WEAP_GRENADE_LAUNCHER = 2048;
constexpr unsigned int WEAP_FLAMETHROWER = 4096;
constexpr unsigned int WEAP_ROCKET_LAUNCHER = 8192;
constexpr unsigned int WEAP_INCENDIARY = 16384;
constexpr unsigned int WEAP_ASSAULT_CANNON = 32768;
constexpr unsigned int WEAP_LIGHTNING = 65536;
constexpr unsigned int WEAP_DETPACK = 131072;
constexpr unsigned int WEAP_TRANQ = 262144;
constexpr unsigned int WEAP_LASER = 524288;

// still room for 12 more weapons
// but we can remove some by giving the weapons
// a weapon mode (like the rifle)

// HL-compatible weapon numbers
constexpr unsigned char WEAPON_HOOK = 1;

#define WEAPON_BIOWEAPON (WEAPON_HOOK + 1)
#define WEAPON_MEDIKIT (WEAPON_HOOK + 2)
#define WEAPON_SPANNER (WEAPON_HOOK + 3)
#define WEAPON_AXE (WEAPON_HOOK + 4)
#define WEAPON_SNIPER_RIFLE (WEAPON_HOOK + 5)
#define WEAPON_AUTO_RIFLE (WEAPON_HOOK + 6)
#define WEAPON_TF_SHOTGUN (WEAPON_HOOK + 7)
#define WEAPON_SUPER_SHOTGUN (WEAPON_HOOK + 8)
#define WEAPON_NAILGUN (WEAPON_HOOK + 9)
#define WEAPON_SUPER_NAILGUN (WEAPON_HOOK + 10)
#define WEAPON_GRENADE_LAUNCHER (WEAPON_HOOK + 11)
#define WEAPON_FLAMETHROWER (WEAPON_HOOK + 12)
#define WEAPON_ROCKET_LAUNCHER (WEAPON_HOOK + 13)
#define WEAPON_INCENDIARY (WEAPON_HOOK + 14)
#define WEAPON_ASSAULT_CANNON (WEAPON_HOOK + 16)
#define WEAPON_LIGHTNING (WEAPON_HOOK + 17)
#define WEAPON_DETPACK (WEAPON_HOOK + 18)
#define WEAPON_TRANQ (WEAPON_HOOK + 19)
#define WEAPON_LASER (WEAPON_HOOK + 20)
#define WEAPON_PIPEBOMB_LAUNCHER (WEAPON_HOOK + 21)
#define WEAPON_KNIFE (WEAPON_HOOK + 22)
#define WEAPON_BENCHMARK (WEAPON_HOOK + 23)

/*==================================================*/
/* New Weapon Related Defines		                */
/*==================================================*/
// shots per reload
constexpr unsigned char RE_SHOTGUN = 8;
constexpr unsigned char RE_SUPER_SHOTGUN = 16; // 8 shots
constexpr unsigned char RE_GRENADE_LAUNCHER = 6;
constexpr unsigned char RE_ROCKET_LAUNCHER = 4;

// reload times
constexpr unsigned char RE_SHOTGUN_TIME = 2;
constexpr unsigned char RE_SUPER_SHOTGUN_TIME = 3;
constexpr unsigned char RE_GRENADE_LAUNCHER_TIME = 4;
constexpr unsigned char RE_ROCKET_LAUNCHER_TIME = 5;

// Maximum velocity you can move and fire the Sniper Rifle
constexpr unsigned char WEAP_SNIPER_RIFLE_MAX_MOVE = 50;

// Medikit
constexpr unsigned char WEAP_MEDIKIT_HEAL = 200; // Amount medikit heals per hit
constexpr unsigned char WEAP_MEDIKIT_OVERHEAL = 50; // Amount of superhealth over max_health the medikit will dispense

// Spanner
constexpr unsigned char WEAP_SPANNER_REPAIR = 10;

// Detpack
constexpr unsigned short WEAP_DETPACK_DISARMTIME = 3;       // Time it takes to disarm a Detpack
constexpr unsigned short WEAP_DETPACK_SETTIME = 3;          // Time it takes to set a Detpack
constexpr unsigned short WEAP_DETPACK_SIZE = 700;           // Explosion Size
constexpr unsigned short WEAP_DETPACK_GOAL_SIZE = 1500;     // Explosion Size for goal triggering
constexpr unsigned short WEAP_DETPACK_BITS_NO = 12;			  // Bits that detpack explodes into

// Tranquiliser Gun
constexpr unsigned char TRANQ_TIME = 15;

// Grenades
constexpr unsigned char GR_PRIMETIME = 3;

#define GR_CALTROP_PRIME 0.5f

constexpr unsigned char GR_TYPE_NONE = 0;
constexpr unsigned char GR_TYPE_NORMAL = 1;
constexpr unsigned char GR_TYPE_CONCUSSION = 2;
constexpr unsigned char GR_TYPE_NAIL = 3;
constexpr unsigned char GR_TYPE_MIRV = 4;
constexpr unsigned char GR_TYPE_NAPALM = 5;
// #define GR_TYPE_FLARE 6
constexpr unsigned char GR_TYPE_GAS = 7;
constexpr unsigned char GR_TYPE_EMP = 8;
constexpr unsigned char GR_TYPE_CALTROP = 9;

//#define GR_TYPE_FLASH		10

// Defines for WeaponMode
constexpr unsigned char GL_NORMAL = 0;
constexpr unsigned char GL_PIPEBOMB = 1;

// Defines for OLD Concussion Grenade
constexpr unsigned char GR_OLD_CONCUSS_TIME = 5;
constexpr unsigned char GR_OLD_CONCUSS_DEC = 20;

// Defines for Concussion Grenade
constexpr float GR_CONCUSS_TIME = 0.25f;

constexpr unsigned char GR_CONCUSS_DEC = 10;
constexpr unsigned char MEDIUM_PING = 150;
constexpr unsigned char HIGH_PING = 200;

// Defines for the Gas Grenade
constexpr float GR_HALLU_TIME = 0.3f;
constexpr float GR_OLD_HALLU_TIME = 0.5f;
constexpr float GR_HALLU_DEC = 2.5f;

// Defines for the BioInfection
constexpr unsigned char BIO_JUMP_RADIUS = 128; // The distance the bioinfection can jump between players

/*==================================================*/
/* New Items			                        	*/
/*==================================================*/
constexpr unsigned char NIT_SCANNER = 1;

#define NIT_SILVER_DOOR_OPENED #IT_KEY1 // 131072
#define NIT_GOLD_DOOR_OPENED #IT_KEY2 // 262144

/*==================================================*/
/* New Item Flags		                        	*/
/*==================================================*/
constexpr unsigned char NIT_SCANNER_ENEMY = 1;    // Detect enemies
constexpr unsigned char NIT_SCANNER_FRIENDLY = 2; // Detect friendlies (team members)
constexpr unsigned char NIT_SCANNER_SOUND = 4;    // Motion detection. Only report moving entities.

/*==================================================*/
/* New Item Related Defines		                    */
/*==================================================*/
constexpr unsigned char NIT_SCANNER_POWER = 25;        // The amount of power spent on a scan with the scanner
																		// is multiplied by this to get the scan range.
constexpr unsigned char NIT_SCANNER_MAXCELL = 50;      // The maximum number of cells that can be used in one scan
constexpr unsigned char NIT_SCANNER_MIN_MOVEMENT = 50; // The minimum velocity an entity must have to be detected

// by scanners that only detect movement

/*==================================================*/
/* Variables used for New Weapons and Reloading     */
/*==================================================*/
// Armor Classes : Bitfields. Use the "armorclass" of armor for the Armor Type.
constexpr unsigned char AT_SAVESHOT = 1;        // Kevlar: Reduces bullet damage by 15%
constexpr unsigned char AT_SAVENAIL = 2;        // Wood: Reduces nail damage by 15%
constexpr unsigned char AT_SAVEEXPLOSION = 4;   // Blast: Reduces explosion damage by 15%
constexpr unsigned char AT_SAVEELECTRICITY = 8; // Shock: Reduces electricity damage by 15%
constexpr unsigned char AT_SAVEFIRE = 16;       // Asbestos: Reduces fire damage by 15%

/*==========================================================================*/
/* TEAMFORTRESS CLASS DETAILS												*/
/*==========================================================================*/
// Class Details for SCOUT
constexpr unsigned short PC_SCOUT_SKIN = 4;				   // Skin for this class when Classkin is on.
constexpr unsigned short PC_SCOUT_MAXHEALTH = 75;		   // Maximum Health Level
constexpr unsigned short PC_SCOUT_MAXSPEED = 400;		   // Maximum movement speed
constexpr unsigned short PC_SCOUT_MAXSTRAFESPEED = 400;	// Maximum strafing movement speed
constexpr unsigned short PC_SCOUT_MAXARMOR = 50;				// Maximum Armor Level, of any armor class
constexpr unsigned short PC_SCOUT_INITARMOR = 25;			// Armor level when respawned

constexpr float PC_SCOUT_MAXARMORTYPE = 0.3f;  // Maximum level of Armor absorption
constexpr float PC_SCOUT_INITARMORTYPE = 0.3f; // Absorption Level of armor when respawned

constexpr unsigned char PC_SCOUT_ARMORCLASSES = 3;   // #AT_SAVESHOT | #AT_SAVENAIL <- Armor Classes allowed for this class
constexpr unsigned char PC_SCOUT_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_SCOUT_WEAPONS (WEAP_AXE | WEAP_SHOTGUN | WEAP_NAILGUN)

constexpr unsigned char PC_SCOUT_MAXAMMO_SHOT = 50;   // Maximum amount of shot ammo this class can carry
constexpr unsigned char PC_SCOUT_MAXAMMO_NAIL = 200;  // Maximum amount of nail ammo this class can carry
constexpr unsigned char PC_SCOUT_MAXAMMO_CELL = 100;  // Maximum amount of cell ammo this class can carry
constexpr unsigned char PC_SCOUT_MAXAMMO_ROCKET = 25; // Maximum amount of rocket ammo this class can carry
constexpr unsigned char PC_SCOUT_INITAMMO_SHOT = 25;  // Amount of shot ammo this class has when respawned
constexpr unsigned char PC_SCOUT_INITAMMO_NAIL = 100; // Amount of nail ammo this class has when respawned
constexpr unsigned char PC_SCOUT_INITAMMO_CELL = 50;  // Amount of cell ammo this class has when respawned
constexpr unsigned char PC_SCOUT_INITAMMO_ROCKET = 0; // Amount of rocket ammo this class has when respawned

#define PC_SCOUT_GRENADE_TYPE_1 GR_TYPE_CALTROP //    <- 1st Type of Grenade this class has
#define PC_SCOUT_GRENADE_TYPE_2 GR_TYPE_CONCUSSION //    <- 2nd Type of Grenade this class has

constexpr unsigned char PC_SCOUT_GRENADE_INIT_1 = 2; // Number of grenades of Type 1 this class has when respawned
constexpr unsigned char PC_SCOUT_GRENADE_INIT_2 = 3; // Number of grenades of Type 2 this class has when respawned

#define PC_SCOUT_TF_ITEMS NIT_SCANNER // <- TeamFortress Items this class has

constexpr float PC_SCOUT_MOTION_MIN_I = 0.5f; // < Short range

constexpr unsigned char PC_SCOUT_MOTION_MIN_MOVE = 50; // Minimum vlen of player velocity to be picked up by motion detector
constexpr unsigned char PC_SCOUT_SCAN_TIME = 2;        // # of seconds between each scan pulse
constexpr unsigned char PC_SCOUT_SCAN_RANGE = 100;     // Default scanner range
constexpr unsigned char PC_SCOUT_SCAN_COST = 2;        // Default scanner cell usage per scan

// Class Details for SNIPER
constexpr unsigned short PC_SNIPER_SKIN = 5;
constexpr unsigned short PC_SNIPER_MAXHEALTH = 90;
constexpr unsigned short PC_SNIPER_MAXSPEED = 300;
constexpr unsigned short PC_SNIPER_MAXSTRAFESPEED = 300;
constexpr unsigned short PC_SNIPER_MAXARMOR = 50;
constexpr unsigned short PC_SNIPER_INITARMOR = 0;

constexpr float PC_SNIPER_MAXARMORTYPE = 0.3f;
constexpr float PC_SNIPER_INITARMORTYPE = 0.3f;

constexpr unsigned char PC_SNIPER_ARMORCLASSES = 3;   // #AT_SAVESHOT | #AT_SAVENAIL
constexpr unsigned char PC_SNIPER_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_SNIPER_WEAPONS (WEAP_SNIPER_RIFLE | WEAP_AUTO_RIFLE | WEAP_AXE | WEAP_NAILGUN)

constexpr unsigned char PC_SNIPER_MAXAMMO_SHOT = 75;
constexpr unsigned char PC_SNIPER_MAXAMMO_NAIL = 100;
constexpr unsigned char PC_SNIPER_MAXAMMO_CELL = 50;
constexpr unsigned char PC_SNIPER_MAXAMMO_ROCKET = 25;
constexpr unsigned char PC_SNIPER_INITAMMO_SHOT = 60;
constexpr unsigned char PC_SNIPER_INITAMMO_NAIL = 50;
constexpr unsigned char PC_SNIPER_INITAMMO_CELL = 0;
constexpr unsigned char PC_SNIPER_INITAMMO_ROCKET = 0;

#define PC_SNIPER_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_SNIPER_GRENADE_TYPE_2 GR_TYPE_NONE

constexpr unsigned char PC_SNIPER_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_SNIPER_GRENADE_INIT_2 = 0;
constexpr unsigned char PC_SNIPER_TF_ITEMS = 0;

// Class Details for SOLDIER
constexpr unsigned short PC_SOLDIER_SKIN = 6;
constexpr unsigned short PC_SOLDIER_MAXHEALTH = 100;
constexpr unsigned short PC_SOLDIER_MAXSPEED = 240;
constexpr unsigned short PC_SOLDIER_MAXSTRAFESPEED = 240;
constexpr unsigned short PC_SOLDIER_MAXARMOR = 200;
constexpr unsigned short PC_SOLDIER_INITARMOR = 100;

constexpr float PC_SOLDIER_MAXARMORTYPE = 0.8f;
constexpr float PC_SOLDIER_INITARMORTYPE = 0.8f;

constexpr unsigned char PC_SOLDIER_ARMORCLASSES = 31;  // ALL
constexpr unsigned char PC_SOLDIER_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_SOLDIER_WEAPONS (WEAP_AXE | WEAP_SHOTGUN | WEAP_SUPER_SHOTGUN | WEAP_ROCKET_LAUNCHER)

constexpr unsigned char PC_SOLDIER_MAXAMMO_SHOT = 100;
constexpr unsigned char PC_SOLDIER_MAXAMMO_NAIL = 100;
constexpr unsigned char PC_SOLDIER_MAXAMMO_CELL = 50;
constexpr unsigned char PC_SOLDIER_MAXAMMO_ROCKET = 50;
constexpr unsigned char PC_SOLDIER_INITAMMO_SHOT = 50;
constexpr unsigned char PC_SOLDIER_INITAMMO_NAIL = 0;
constexpr unsigned char PC_SOLDIER_INITAMMO_CELL = 0;
constexpr unsigned char PC_SOLDIER_INITAMMO_ROCKET = 10;

#define PC_SOLDIER_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_SOLDIER_GRENADE_TYPE_2 GR_TYPE_NAIL

constexpr unsigned char PC_SOLDIER_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_SOLDIER_GRENADE_INIT_2 = 1;
constexpr unsigned char PC_SOLDIER_TF_ITEMS = 0;

constexpr unsigned char MAX_NAIL_GRENS = 2;   // Can only have 2 Nail grens active
constexpr unsigned char MAX_NAPALM_GRENS = 2; // Can only have 2 Napalm grens active
constexpr unsigned char MAX_GAS_GRENS = 2;    // Can only have 2 Gas grenades active
constexpr unsigned char MAX_MIRV_GRENS = 2;   // Can only have 2 Mirv's
constexpr unsigned char MAX_CONCUSSION_GRENS = 3;
constexpr unsigned char MAX_CALTROP_CANS = 3;

// Class Details for DEMOLITION MAN
constexpr unsigned short PC_DEMOMAN_SKIN = 1;
constexpr unsigned short PC_DEMOMAN_MAXHEALTH = 90;
constexpr unsigned short PC_DEMOMAN_MAXSPEED = 280;
constexpr unsigned short PC_DEMOMAN_MAXSTRAFESPEED = 280;
constexpr unsigned short PC_DEMOMAN_MAXARMOR = 120;
constexpr unsigned short PC_DEMOMAN_INITARMOR = 50;

constexpr float PC_DEMOMAN_MAXARMORTYPE = 0.6f;
constexpr float PC_DEMOMAN_INITARMORTYPE = 0.6f;

constexpr unsigned short PC_DEMOMAN_ARMORCLASSES = 31;  // ALL
constexpr unsigned short PC_DEMOMAN_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_DEMOMAN_WEAPONS (WEAP_AXE | WEAP_SHOTGUN | WEAP_GRENADE_LAUNCHER | WEAP_DETPACK)

constexpr unsigned char PC_DEMOMAN_MAXAMMO_SHOT = 75;
constexpr unsigned char PC_DEMOMAN_MAXAMMO_NAIL = 50;
constexpr unsigned char PC_DEMOMAN_MAXAMMO_CELL = 50;
constexpr unsigned char PC_DEMOMAN_MAXAMMO_ROCKET = 50;
constexpr unsigned char PC_DEMOMAN_MAXAMMO_DETPACK = 1;
constexpr unsigned char PC_DEMOMAN_INITAMMO_SHOT = 30;
constexpr unsigned char PC_DEMOMAN_INITAMMO_NAIL = 0;
constexpr unsigned char PC_DEMOMAN_INITAMMO_CELL = 0;
constexpr unsigned char PC_DEMOMAN_INITAMMO_ROCKET = 20;
constexpr unsigned char PC_DEMOMAN_INITAMMO_DETPACK = 1;

#define PC_DEMOMAN_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_DEMOMAN_GRENADE_TYPE_2 GR_TYPE_MIRV

constexpr unsigned char PC_DEMOMAN_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_DEMOMAN_GRENADE_INIT_2 = 2;
constexpr unsigned char PC_DEMOMAN_TF_ITEMS = 0;

// Class Details for COMBAT MEDIC
constexpr unsigned short PC_MEDIC_SKIN = 3;
constexpr unsigned short PC_MEDIC_MAXHEALTH = 90;
constexpr unsigned short PC_MEDIC_MAXSPEED = 320;
constexpr unsigned short PC_MEDIC_MAXSTRAFESPEED = 320;
constexpr unsigned short PC_MEDIC_MAXARMOR = 100;
constexpr unsigned short PC_MEDIC_INITARMOR = 50;

constexpr float PC_MEDIC_MAXARMORTYPE = 0.6f;
constexpr float PC_MEDIC_INITARMORTYPE = 0.3f;

constexpr unsigned char PC_MEDIC_ARMORCLASSES = 11;  // ALL except EXPLOSION
constexpr unsigned char PC_MEDIC_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_MEDIC_WEAPONS (WEAP_BIOWEAPON | WEAP_MEDIKIT | WEAP_SHOTGUN | WEAP_SUPER_SHOTGUN | WEAP_SUPER_NAILGUN)

constexpr unsigned char PC_MEDIC_MAXAMMO_SHOT = 75;
constexpr unsigned char PC_MEDIC_MAXAMMO_NAIL = 150;
constexpr unsigned char PC_MEDIC_MAXAMMO_CELL = 50;
constexpr unsigned char PC_MEDIC_MAXAMMO_ROCKET = 25;
constexpr unsigned char PC_MEDIC_MAXAMMO_MEDIKIT = 100;
constexpr unsigned char PC_MEDIC_INITAMMO_SHOT = 50;
constexpr unsigned char PC_MEDIC_INITAMMO_NAIL = 50;
constexpr unsigned char PC_MEDIC_INITAMMO_CELL = 0;
constexpr unsigned char PC_MEDIC_INITAMMO_ROCKET = 0;
constexpr unsigned char PC_MEDIC_INITAMMO_MEDIKIT = 50;

#define PC_MEDIC_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_MEDIC_GRENADE_TYPE_2 GR_TYPE_CONCUSSION

constexpr unsigned char PC_MEDIC_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_MEDIC_GRENADE_INIT_2 = 2;
constexpr unsigned char PC_MEDIC_TF_ITEMS = 0;
constexpr unsigned char PC_MEDIC_REGEN_TIME = 3;   // Number of seconds between each regen.
constexpr unsigned char PC_MEDIC_REGEN_AMOUNT = 2; // Amount of health regenerated each regen.

// Class Details for HVYWEAP
constexpr unsigned short PC_HVYWEAP_SKIN = 2;
constexpr unsigned short PC_HVYWEAP_MAXHEALTH = 100;
constexpr unsigned short PC_HVYWEAP_MAXSPEED = 230;
constexpr unsigned short PC_HVYWEAP_MAXSTRAFESPEED = 230;
constexpr unsigned short PC_HVYWEAP_MAXARMOR = 300;
constexpr unsigned short PC_HVYWEAP_INITARMOR = 15;

constexpr float PC_HVYWEAP_MAXARMORTYPE = 0.8f;
constexpr float PC_HVYWEAP_INITARMORTYPE = 0.8f;

constexpr unsigned char PC_HVYWEAP_ARMORCLASSES = 31;  // ALL
constexpr unsigned char PC_HVYWEAP_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_HVYWEAP_WEAPONS (WEAP_ASSAULT_CANNON | WEAP_AXE | WEAP_SHOTGUN | WEAP_SUPER_SHOTGUN)

constexpr unsigned char PC_HVYWEAP_MAXAMMO_SHOT = 200;
constexpr unsigned char PC_HVYWEAP_MAXAMMO_NAIL = 200;
constexpr unsigned char PC_HVYWEAP_MAXAMMO_CELL = 50;
constexpr unsigned char PC_HVYWEAP_MAXAMMO_ROCKET = 25;
constexpr unsigned char PC_HVYWEAP_INITAMMO_SHOT = 200;
constexpr unsigned char PC_HVYWEAP_INITAMMO_NAIL = 0;
constexpr unsigned char PC_HVYWEAP_INITAMMO_CELL = 30;
constexpr unsigned char PC_HVYWEAP_INITAMMO_ROCKET = 0;

#define PC_HVYWEAP_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_HVYWEAP_GRENADE_TYPE_2 GR_TYPE_MIRV

constexpr unsigned char PC_HVYWEAP_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_HVYWEAP_GRENADE_INIT_2 = 1;
constexpr unsigned char PC_HVYWEAP_TF_ITEMS = 0;
constexpr unsigned char PC_HVYWEAP_CELL_USAGE = 7; // Amount of cells spent to power up assault cannon

// Class Details for PYRO
constexpr unsigned short PC_PYRO_SKIN = 21;
constexpr unsigned short PC_PYRO_MAXHEALTH = 100;
constexpr unsigned short PC_PYRO_MAXSPEED = 300;
constexpr unsigned short PC_PYRO_MAXSTRAFESPEED = 300;
constexpr unsigned short PC_PYRO_MAXARMOR = 150;
constexpr unsigned short PC_PYRO_INITARMOR = 5;

constexpr float PC_PYRO_MAXARMORTYPE = 0.6f;
constexpr float PC_PYRO_INITARMORTYPE = 0.6f;

constexpr unsigned char PC_PYRO_ARMORCLASSES = 27;   // ALL except EXPLOSION
constexpr unsigned char PC_PYRO_INITARMORCLASS = 16; // #AT_SAVEFIRE

#define PC_PYRO_WEAPONS (WEAP_INCENDIARY | WEAP_FLAMETHROWER | WEAP_AXE | WEAP_SHOTGUN)

constexpr unsigned char PC_PYRO_MAXAMMO_SHOT = 40;
constexpr unsigned char PC_PYRO_MAXAMMO_NAIL = 50;
constexpr unsigned char PC_PYRO_MAXAMMO_CELL = 200;
constexpr unsigned char PC_PYRO_MAXAMMO_ROCKET = 20;
constexpr unsigned char PC_PYRO_INITAMMO_SHOT = 20;
constexpr unsigned char PC_PYRO_INITAMMO_NAIL = 0;
constexpr unsigned char PC_PYRO_INITAMMO_CELL = 120;
constexpr unsigned char PC_PYRO_INITAMMO_ROCKET = 5;

#define PC_PYRO_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_PYRO_GRENADE_TYPE_2 GR_TYPE_NAPALM

constexpr unsigned char PC_PYRO_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_PYRO_GRENADE_INIT_2 = 4;
constexpr unsigned char PC_PYRO_TF_ITEMS = 0;
constexpr unsigned char PC_PYRO_ROCKET_USAGE = 3; // Number of rockets per incendiary cannon shot

// Class Details for SPY
constexpr unsigned short PC_SPY_SKIN = 22;
constexpr unsigned short PC_SPY_MAXHEALTH = 90;
constexpr unsigned short PC_SPY_MAXSPEED = 300;
constexpr unsigned short PC_SPY_MAXSTRAFESPEED = 300;
constexpr unsigned short PC_SPY_MAXARMOR = 100;
constexpr unsigned short PC_SPY_INITARMOR = 25;

constexpr float PC_SPY_MAXARMORTYPE = 0.6f;  // Was 0.3
constexpr float PC_SPY_INITARMORTYPE = 0.6f; // Was 0.3

constexpr unsigned char PC_SPY_ARMORCLASSES = 27;  // ALL except EXPLOSION
constexpr unsigned char PC_SPY_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_SPY_WEAPONS (WEAP_AXE | WEAP_TRANQ | WEAP_SUPER_SHOTGUN | WEAP_NAILGUN)

constexpr unsigned char PC_SPY_MAXAMMO_SHOT = 40;
constexpr unsigned char PC_SPY_MAXAMMO_NAIL = 100;
constexpr unsigned char PC_SPY_MAXAMMO_CELL = 30;
constexpr unsigned char PC_SPY_MAXAMMO_ROCKET = 15;
constexpr unsigned char PC_SPY_INITAMMO_SHOT = 40;
constexpr unsigned char PC_SPY_INITAMMO_NAIL = 50;
constexpr unsigned char PC_SPY_INITAMMO_CELL = 10;
constexpr unsigned char PC_SPY_INITAMMO_ROCKET = 0;

#define PC_SPY_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_SPY_GRENADE_TYPE_2 GR_TYPE_GAS

constexpr unsigned char PC_SPY_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_SPY_GRENADE_INIT_2 = 2;
constexpr unsigned char PC_SPY_TF_ITEMS = 0;
constexpr unsigned char PC_SPY_CELL_REGEN_TIME = 5;
constexpr unsigned char PC_SPY_CELL_REGEN_AMOUNT = 1;
constexpr unsigned char PC_SPY_CELL_USAGE = 3;         // Amount of cells spent while invisible
constexpr unsigned char PC_SPY_GO_UNDERCOVER_TIME = 4; // Time it takes to go undercover

// Class Details for ENGINEER
constexpr unsigned short PC_ENGINEER_SKIN = 22; // Not used anymore
constexpr unsigned short PC_ENGINEER_MAXHEALTH = 80;
constexpr unsigned short PC_ENGINEER_MAXSPEED = 300;
constexpr unsigned short PC_ENGINEER_MAXSTRAFESPEED = 300;
constexpr unsigned short PC_ENGINEER_MAXARMOR = 50;
constexpr unsigned short PC_ENGINEER_INITARMOR = 25;

constexpr float PC_ENGINEER_MAXARMORTYPE = 0.6f;
constexpr float PC_ENGINEER_INITARMORTYPE = 0.3f;

constexpr unsigned char PC_ENGINEER_ARMORCLASSES = 31;  // ALL
constexpr unsigned char PC_ENGINEER_INITARMORCLASS = 0; // Armor class worn when respawned

#define PC_ENGINEER_WEAPONS (WEAP_SPANNER | WEAP_LASER | WEAP_SUPER_SHOTGUN)

constexpr unsigned char PC_ENGINEER_MAXAMMO_SHOT = 50;
constexpr unsigned char PC_ENGINEER_MAXAMMO_NAIL = 50;
constexpr unsigned char PC_ENGINEER_MAXAMMO_CELL = 200; // synonymous with metal
constexpr unsigned char PC_ENGINEER_MAXAMMO_ROCKET = 30;
constexpr unsigned char PC_ENGINEER_INITAMMO_SHOT = 20;
constexpr unsigned char PC_ENGINEER_INITAMMO_NAIL = 25;
constexpr unsigned char PC_ENGINEER_INITAMMO_CELL = 100; // synonymous with metal
constexpr unsigned char PC_ENGINEER_INITAMMO_ROCKET = 0;

#define PC_ENGINEER_GRENADE_TYPE_1 GR_TYPE_NORMAL
#define PC_ENGINEER_GRENADE_TYPE_2 GR_TYPE_EMP

constexpr unsigned char PC_ENGINEER_GRENADE_INIT_1 = 2;
constexpr unsigned char PC_ENGINEER_GRENADE_INIT_2 = 2;
constexpr unsigned char PC_ENGINEER_TF_ITEMS = 0;

// Class Details for CIVILIAN
constexpr unsigned short PC_CIVILIAN_SKIN = 22;
constexpr unsigned short PC_CIVILIAN_MAXHEALTH = 50;
constexpr unsigned short PC_CIVILIAN_MAXSPEED = 240;
constexpr unsigned short PC_CIVILIAN_MAXSTRAFESPEED = 240;
constexpr unsigned short PC_CIVILIAN_MAXARMOR = 0;
constexpr unsigned short PC_CIVILIAN_INITARMOR = 0;
constexpr unsigned short PC_CIVILIAN_MAXARMORTYPE = 0;
constexpr unsigned short PC_CIVILIAN_INITARMORTYPE = 0;
constexpr unsigned short PC_CIVILIAN_ARMORCLASSES = 0;
constexpr unsigned short PC_CIVILIAN_INITARMORCLASS = 0;

#define PC_CIVILIAN_WEAPONS WEAP_AXE

constexpr unsigned char PC_CIVILIAN_MAXAMMO_SHOT = 0;
constexpr unsigned char PC_CIVILIAN_MAXAMMO_NAIL = 0;
constexpr unsigned char PC_CIVILIAN_MAXAMMO_CELL = 0;
constexpr unsigned char PC_CIVILIAN_MAXAMMO_ROCKET = 0;
constexpr unsigned char PC_CIVILIAN_INITAMMO_SHOT = 0;
constexpr unsigned char PC_CIVILIAN_INITAMMO_NAIL = 0;
constexpr unsigned char PC_CIVILIAN_INITAMMO_CELL = 0;
constexpr unsigned char PC_CIVILIAN_INITAMMO_ROCKET = 0;
constexpr unsigned char PC_CIVILIAN_GRENADE_TYPE_1 = 0;
constexpr unsigned char PC_CIVILIAN_GRENADE_TYPE_2 = 0;
constexpr unsigned char PC_CIVILIAN_GRENADE_INIT_1 = 0;
constexpr unsigned char PC_CIVILIAN_GRENADE_INIT_2 = 0;
constexpr unsigned char PC_CIVILIAN_TF_ITEMS = 0;

/*==========================================================================*/
/* TEAMFORTRESS GOALS														*/
/*==========================================================================*/
// For all these defines, see the tfortmap.txt that came with the zip
// for complete descriptions.
// Defines for Goal Activation types : goal_activation (in goals)
constexpr unsigned short TFGA_TOUCH = 1;              // Activated when touched
constexpr unsigned short TFGA_TOUCH_DETPACK = 2;      // Activated when touched by a detpack explosion
constexpr unsigned short TFGA_REVERSE_AP = 4;         // Activated when AP details are _not_ met
constexpr unsigned short TFGA_SPANNER = 8;            // Activated when hit by an engineer's spanner
constexpr unsigned short TFGA_DROPTOGROUND = 2048;    // Drop to Ground when spawning

// Defines for Goal Effects types : goal_effect
constexpr unsigned char TFGE_AP = 1;                // AP is affected. Default.
constexpr unsigned char TFGE_AP_TEAM = 2;           // All of the AP's team.
constexpr unsigned char TFGE_NOT_AP_TEAM = 4;       // All except AP's team.
constexpr unsigned char TFGE_NOT_AP = 8;            // All except AP.
constexpr unsigned char TFGE_WALL = 16;             // If set, walls stop the Radius effects
constexpr unsigned char TFGE_SAME_ENVIRONMENT = 32; // If set, players in a different environment to the Goal are not affected
constexpr unsigned char TFGE_TIMER_CHECK_AP = 64;   // If set, Timer Goals check their criteria for all players fitting their effects

// Defines for Goal Result types : goal_result
constexpr unsigned char TFGR_SINGLE = 1;             // Goal can only be activated once
constexpr unsigned char TFGR_ADD_BONUSES = 2;        // Any Goals activated by this one give their bonuses
constexpr unsigned char TFGR_ENDGAME = 4;            // Goal fires Intermission, displays scores, and ends level
constexpr unsigned char TFGR_NO_ITEM_RESULTS = 8;    // GoalItems given by this Goal don't do results
constexpr unsigned char TFGR_REMOVE_DISGUISE = 16;   // Prevent/Remove undercover from any Spy
constexpr unsigned char TFGR_FORCE_RESPAWN = 32;     // Forces the player to teleport to a respawn point
constexpr unsigned char TFGR_DESTROY_BUILDINGS = 64; // Destroys this player's buildings, if any

// Defines for Goal Group Result types : goal_group
// None!
// But I'm leaving this variable in there, since it's fairly likely
// that some will show up sometime.

// Defines for Goal Item types, : goal_activation (in items)
constexpr unsigned short TFGI_GLOW = 1;             // Players carrying this GoalItem will glow
constexpr unsigned short TFGI_SLOW = 2;             // Players carrying this GoalItem will move at half-speed
constexpr unsigned short TFGI_DROP = 4;             // Players dying with this item will drop it
constexpr unsigned short TFGI_RETURN_DROP = 8;      // Return if a player with it dies
constexpr unsigned short TFGI_RETURN_GOAL = 16;     // Return if a player with it has it removed by a goal's activation
constexpr unsigned short TFGI_RETURN_REMOVE = 32;   // Return if it is removed by TFGI_REMOVE
constexpr unsigned short TFGI_REVERSE_AP = 64;      // Only pickup if the player _doesn't_ match AP Details
constexpr unsigned short TFGI_REMOVE = 128;         // Remove if left untouched for 2 minutes after being dropped
constexpr unsigned short TFGI_KEEP = 256;           // Players keep this item even when they die
constexpr unsigned short TFGI_ITEMGLOWS = 512;      // Item glows when on the ground
constexpr unsigned short TFGI_DONTREMOVERES = 1024; // Don't remove results when the item is removed
constexpr unsigned short TFGI_DROPTOGROUND = 2048;  // Drop To Ground when spawning
constexpr unsigned short TFGI_CANBEDROPPED = 4096;  // Can be voluntarily dropped by players
constexpr unsigned short TFGI_SOLID = 8192;         // Is solid... blocks bullets, etc

// Defines for methods of GoalItem returning
constexpr unsigned char GI_RET_DROP_DEAD = 0;   // Dropped by a dead player
constexpr unsigned char GI_RET_DROP_LIVING = 1; // Dropped by a living player
constexpr unsigned char GI_RET_GOAL = 2;        // Returned by a Goal
constexpr unsigned char GI_RET_TIME = 3;        // Returned due to timeout

// Defines for TeamSpawnpoints : goal_activation (in teamspawns)
constexpr unsigned char TFSP_MULTIPLEITEMS = 1; // Give out the GoalItem multiple times
constexpr unsigned char TFSP_MULTIPLEMSGS = 2;  // Display the message multiple times

// Defines for TeamSpawnpoints : goal_effects (in teamspawns)
constexpr unsigned char TFSP_REMOVESELF = 1; // Remove itself after being spawned on

// Defines for Goal States
constexpr unsigned char TFGS_ACTIVE = 1;
constexpr unsigned char TFGS_INACTIVE = 2;
constexpr unsigned char TFGS_REMOVED = 3;
constexpr unsigned char TFGS_DELAYED = 4;

// Defines for GoalItem Removing from Player Methods
constexpr unsigned char GI_DROP_PLAYERDEATH = 0; // Dropped by a dying player
constexpr unsigned char GI_DROP_REMOVEGOAL = 1;  // Removed by a Goal
constexpr unsigned char GI_DROP_PLAYERDROP = 2;  // Dropped by a player

// Legal Playerclass Handling
constexpr unsigned short TF_ILL_SCOUT = 1;
constexpr unsigned short TF_ILL_SNIPER = 2;
constexpr unsigned short TF_ILL_SOLDIER = 4;
constexpr unsigned short TF_ILL_DEMOMAN = 8;
constexpr unsigned short TF_ILL_MEDIC = 16;
constexpr unsigned short TF_ILL_HVYWEP = 32;
constexpr unsigned short TF_ILL_PYRO = 64;
constexpr unsigned short TF_ILL_RANDOMPC = 128;
constexpr unsigned short TF_ILL_SPY = 256;
constexpr unsigned short TF_ILL_ENGINEER = 512;

// Addition classes
constexpr unsigned char CLASS_TFGOAL = 128;
constexpr unsigned char CLASS_TFGOAL_TIMER = 129;
constexpr unsigned char CLASS_TFGOAL_ITEM = 130;
constexpr unsigned char CLASS_TFSPAWN = 131;

/*==========================================================================*/
/* Flamethrower																				 */
/*==========================================================================*/
constexpr float FLAME_PLYRMAXTIME = 5.0f;        // lifetime in seconds of a flame on a player

constexpr unsigned char FLAME_MAXBURNTIME = 8;   // lifetime in seconds of a flame on the world (big ones)
constexpr unsigned char NAPALM_MAXBURNTIME = 20; // lifetime in seconds of flame from a napalm grenade
constexpr unsigned char FLAME_MAXPLYRFLAMES = 4; // maximum number of flames on a player
constexpr unsigned char FLAME_NUMLIGHTS = 1;     // maximum number of light flame

constexpr float FLAME_BURNRATIO = 0.3f;          // the chance of a flame not 'sticking'

constexpr unsigned char GR_TYPE_FLAMES_NO = 15;  // number of flames spawned when a grenade explode
constexpr unsigned char FLAME_DAMAGE_TIME = 1;   // Interval between damage burns from flames

constexpr float FLAME_EFFECT_TIME = 0.2f;        // frequency at which we display flame effects.
constexpr float FLAME_THINK_TIME = 0.1f;         // Seconds between times the flame checks burn

constexpr unsigned char PER_FLAME_DAMAGE = 2; // Damage taken per second per flame by burning players

/*==================================================*/
/* CTF Support defines 								*/
/*==================================================*/
constexpr unsigned char CTF_FLAG1 = 1;
constexpr unsigned char CTF_FLAG2 = 2;
constexpr unsigned char CTF_DROPOFF1 = 3;
constexpr unsigned char CTF_DROPOFF2 = 4;
constexpr unsigned char CTF_SCORE1 = 5;
constexpr unsigned char CTF_SCORE2 = 6;

//.float	hook_out;

/*==================================================*/
/* Camera defines	 								*/
/*==================================================*/
/*
float live_camera;
.float camdist;
.vector camangle;
.entity camera_list;
*/

/*==================================================*/
/* QuakeWorld defines 								*/
/*==================================================*/
/*
float already_chosen_map;
// grappling hook variables
.entity	hook;
.float	on_hook;
.float  fire_held_down;// flag - true if player is still holding down the
														  // fire button after throwing a hook.
*/
/*==================================================*/
/* Server Settings								    */
/*==================================================*/
// Admin modes
constexpr unsigned char ADMIN_MODE_NONE = 0;
constexpr unsigned char ADMIN_MODE_DEAL = 1;

/*==================================================*/
/* Death Message defines							*/
/*==================================================*/
constexpr unsigned char DMSG_SHOTGUN = 1;
constexpr unsigned char DMSG_SSHOTGUN = 2;
constexpr unsigned char DMSG_NAILGUN = 3;
constexpr unsigned char DMSG_SNAILGUN = 4;
constexpr unsigned char DMSG_GRENADEL = 5;
constexpr unsigned char DMSG_ROCKETL = 6;
constexpr unsigned char DMSG_LIGHTNING = 7;
constexpr unsigned char DMSG_GREN_HAND = 8;
constexpr unsigned char DMSG_GREN_NAIL = 9;
constexpr unsigned char DMSG_GREN_MIRV = 10;
constexpr unsigned char DMSG_GREN_PIPE = 11;
constexpr unsigned char DMSG_DETPACK = 12;
constexpr unsigned char DMSG_BIOWEAPON = 13;
constexpr unsigned char DMSG_BIOWEAPON_ATT = 14;
constexpr unsigned char DMSG_FLAME = 15;
constexpr unsigned char DMSG_DETPACK_DIS = 16;
constexpr unsigned char DMSG_AXE = 17;
constexpr unsigned char DMSG_SNIPERRIFLE = 18;
constexpr unsigned char DMSG_AUTORIFLE = 19;
constexpr unsigned char DMSG_ASSAULTCANNON = 20;
constexpr unsigned char DMSG_HOOK = 21;
constexpr unsigned char DMSG_BACKSTAB = 22;
constexpr unsigned char DMSG_MEDIKIT = 23;
constexpr unsigned char DMSG_GREN_GAS = 24;
constexpr unsigned char DMSG_TRANQ = 25;
constexpr unsigned char DMSG_LASERBOLT = 26;
constexpr unsigned char DMSG_SENTRYGUN_BULLET = 27;
constexpr unsigned char DMSG_SNIPERLEGSHOT = 28;
constexpr unsigned char DMSG_SNIPERHEADSHOT = 29;
constexpr unsigned char DMSG_GREN_EMP = 30;
constexpr unsigned char DMSG_GREN_EMP_AMMO = 31;
constexpr unsigned char DMSG_SPANNER = 32;
constexpr unsigned char DMSG_INCENDIARY = 33;
constexpr unsigned char DMSG_SENTRYGUN_ROCKET = 34;
constexpr unsigned char DMSG_GREN_FLASH = 35;
constexpr unsigned char DMSG_TRIGGER = 36;
constexpr unsigned char DMSG_MIRROR = 37;
constexpr unsigned char DMSG_SENTRYDEATH = 38;
constexpr unsigned char DMSG_DISPENSERDEATH = 39;
constexpr unsigned char DMSG_GREN_AIRPIPE = 40;
constexpr unsigned char DMSG_CALTROP = 41;

/*==================================================*/
// TOGGLEFLAGS
/*==================================================*/
// Some of the toggleflags aren't used anymore, but the bits are still
// there to provide compatability with old maps
constexpr unsigned short TFLAG_CLASS_PERSIST = 1 << 0; // Persistent Classes Bit
constexpr unsigned short TFLAG_CHEATCHECK = 1 << 1;    // Cheatchecking Bit
constexpr unsigned short TFLAG_RESPAWNDELAY = 1 << 2;  // RespawnDelay bit
// #define TFLAG_UN					(1 << 3)		// NOT USED ANYMORE
constexpr unsigned short TFLAG_OLD_GRENS = 1 << 3;  // Use old concussion grenade and flash grenade
constexpr unsigned short TFLAG_UN2 = 1 << 4;        // NOT USED ANYMORE
constexpr unsigned short TFLAG_UN3 = 1 << 5;        // NOT USED ANYMORE
constexpr unsigned short TFLAG_UN4 = 1 << 6;        // NOT USED ANYMORE: Was Autoteam. CVAR tfc_autoteam used now.
constexpr unsigned short TFLAG_TEAMFRAGS = 1 << 7;  // Individual Frags, or Frags = TeamScore
constexpr unsigned short TFLAG_FIRSTENTRY = 1 << 8; // Used to determine the first time toggleflags is set in a map. Cannot be toggled by players.
constexpr unsigned short TFLAG_SPYINVIS = 1 << 9;   // Spy invisible only
constexpr unsigned short TFLAG_GRAPPLE = 1 << 10;   // Grapple on/off
// #define TFLAG_FULLTEAMSCORE		(1 << 11)  	// Each Team's score is TeamScore + Frags
constexpr unsigned short TFLAG_FLAGEMULATION = 1 << 12; // Flag emulation on for old TF maps
constexpr unsigned short TFLAG_USE_STANDARD = 1 << 13;  // Use the TF War standard for Flag emulation
constexpr unsigned short TFLAG_FRAGSCORING = 1 << 14;   // Use frag scoring only

/*======================*/
//      Menu stuff      //
/*======================*/

constexpr unsigned char MENU_DEFAULT = 1;
constexpr unsigned char MENU_TEAM = 2;
constexpr unsigned char MENU_CLASS = 3;
constexpr unsigned char MENU_MAPBRIEFING = 4;
constexpr unsigned char MENU_INTRO = 5;
constexpr unsigned char MENU_CLASSHELP = 6;
constexpr unsigned char MENU_CLASSHELP2 = 7;
constexpr unsigned char MENU_REPEATHELP = 8;

constexpr unsigned char MENU_SPECHELP = 9;

constexpr unsigned char MENU_SPY = 12;
constexpr unsigned char MENU_SPY_SKIN = 13;
constexpr unsigned char MENU_SPY_COLOR = 14;
constexpr unsigned char MENU_ENGINEER = 15;
constexpr unsigned char MENU_ENGINEER_FIX_DISPENSER = 16;
constexpr unsigned char MENU_ENGINEER_FIX_SENTRYGUN = 17;
constexpr unsigned char MENU_ENGINEER_FIX_MORTAR = 18;
constexpr unsigned char MENU_DISPENSER = 19;
constexpr unsigned char MENU_CLASS_CHANGE = 20;
constexpr unsigned char MENU_TEAM_CHANGE = 21;

constexpr unsigned char MENU_REFRESH_RATE = 25;

constexpr unsigned char MENU_VOICETWEAK = 50;

//============================
// Timer Types
constexpr unsigned char TF_TIMER_ANY = 0;
constexpr unsigned char TF_TIMER_CONCUSSION = 1;
constexpr unsigned char TF_TIMER_INFECTION = 2;
constexpr unsigned char TF_TIMER_HALLUCINATION = 3;
constexpr unsigned char TF_TIMER_TRANQUILISATION = 4;
constexpr unsigned char TF_TIMER_ROTHEALTH = 5;
constexpr unsigned char TF_TIMER_REGENERATION = 6;
constexpr unsigned char TF_TIMER_GRENPRIME = 7;
constexpr unsigned char TF_TIMER_CELLREGENERATION = 8;
constexpr unsigned char TF_TIMER_DETPACKSET = 9;
constexpr unsigned char TF_TIMER_DETPACKDISARM = 10;
constexpr unsigned char TF_TIMER_BUILD = 11;
constexpr unsigned char TF_TIMER_CHECKBUILDDISTANCE = 12;
constexpr unsigned char TF_TIMER_DISGUISE = 13;
constexpr unsigned char TF_TIMER_DISPENSERREFILL = 14;

// Non Player timers
constexpr unsigned char TF_TIMER_RETURNITEM = 100;
constexpr unsigned char TF_TIMER_DELAYEDGOAL = 101;
constexpr unsigned char TF_TIMER_ENDROUND = 102;

//============================
// Teamscore printing
constexpr unsigned char TS_PRINT_SHORT = 1;
constexpr unsigned char TS_PRINT_LONG = 2;
constexpr unsigned char TS_PRINT_LONG_TO_ALL = 3;

#endif