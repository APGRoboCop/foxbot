//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot.h
//
// Copyright (C) 2003 - Tom "Redfox" Simpson
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details at:
// http://www.gnu.org/copyleft/gpl.html
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef BOT_H
#define BOT_H

#include "osdep.h"

// stuff for Win32 vs. Linux builds

#ifndef __linux__

typedef int(FAR* GETENTITYAPI)(DLL_FUNCTIONS*, int);
typedef int(FAR* GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS*, int*);
typedef void(DLLEXPORT* GIVEFNPTRSTODLL)(enginefuncs_t*, globalvars_t*);
typedef void(FAR* LINK_ENTITY_FUNC)(entvars_t*);

#else

#include <dlfcn.h>
#define GetProcAddress dlsym

typedef int BOOL;

typedef int (*GETENTITYAPI)(DLL_FUNCTIONS*, int);
typedef int (*GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS*, int*);
typedef void (*GIVEFNPTRSTODLL)(enginefuncs_t*, globalvars_t*);
typedef void (*LINK_ENTITY_FUNC)(entvars_t*);

#endif

// define constants used to identify the MOD we are playing...
#define VALVE_DLL 1
#define TFC_DLL 2
//#define CSTRIKE_DLL 3 // Not required for TFC - [APG]RoboCop[CL]
//#define GEARBOX_DLL 4
//#define FRONTLINE_DLL 5

// define some function prototypes...
BOOL ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]);
void ClientPutInServer(edict_t* pEntity);
void ClientCommand(edict_t* pEntity);
void ClientKill(edict_t* pEntity);

void FakeClientCommand(edict_t* pBot, char* arg1, char* arg2, char* arg3);
// void FakeClientCommand (edict_t *pFakeClient, const char *fmt, ...);

const char* Cmd_Args(void);
const char* Cmd_Argv(int argc);
int Cmd_Argc(void);

#define BOT_PITCH_SPEED 20
#define BOT_YAW_SPEED 20

#define RESPAWN_IDLE 1
#define RESPAWN_NEED_TO_RESPAWN 2
#define RESPAWN_IS_RESPAWNING 3

// game start messages for TFC...
#define MSG_TFC_IDLE 1
#define MSG_TFC_TEAM_SELECT 2
#define MSG_TFC_CLASS_SELECT 3

// game start messages for CS...
#define MSG_CS_IDLE 1
#define MSG_CS_TEAM_SELECT 2
#define MSG_CS_CT_SELECT 3
#define MSG_CS_T_SELECT 4

// game start messages for OpFor...
#define MSG_OPFOR_IDLE 1
#define MSG_OPFOR_TEAM_SELECT 2
#define MSG_OPFOR_CLASS_SELECT 3

// game start messages for FrontLineForce...
#define MSG_FLF_IDLE 1
#define MSG_FLF_TEAM_SELECT 2
#define MSG_FLF_CLASS_SELECT 3
#define MSG_FLF_PISTOL_SELECT 4
#define MSG_FLF_WEAPON_SELECT 5
#define MSG_FLF_RIFLE_SELECT 6
#define MSG_FLF_SHOTGUN_SELECT 7
#define MSG_FLF_SUBMACHINE_SELECT 8
#define MSG_FLF_HEAVYWEAPONS_SELECT 9

#define TFC_CLASS_CIVILIAN 11
#define TFC_CLASS_SCOUT 1
#define TFC_CLASS_SNIPER 2
#define TFC_CLASS_SOLDIER 3
#define TFC_CLASS_DEMOMAN 4
#define TFC_CLASS_MEDIC 5
#define TFC_CLASS_HWGUY 6
#define TFC_CLASS_PYRO 7
#define TFC_CLASS_SPY 8
#define TFC_CLASS_ENGINEER 9

#define BOT_SKIN_LEN 32
#define BOT_NAME_LEN 32

// This is the same spawn flag as SF_BUTTON_TOUCH_ONLY, i.e. used for buttons
// that activate when a player bumps into them(like the ones on well)
// Defined here so that there's no need to trawl the HL SDK to find SF_BUTTON_TOUCH_ONLY.
#define SFLAG_PROXIMITY_BUTTON 256

// bot chat stuff
#define TOTAL_CHAT_TYPES 6
#define MAX_CHAT_STRINGS 100 // max strings per type of chat
#define MAX_CHAT_LENGTH 80

// the indexes for each type of chat
#define CHAT_TYPE_GREETING 0
#define CHAT_TYPE_KILL_HI 1
#define CHAT_TYPE_KILL_LOW 2
#define CHAT_TYPE_KILLED_HI 3
#define CHAT_TYPE_KILLED_LOW 4
#define CHAT_TYPE_SUICIDE 5

#include <string>

using namespace std;

// a class for handling the bot chat messages
class chatClass
{
private:
    // section header names for each chat type, as used in the chat file
    string sectionNames[TOTAL_CHAT_TYPES];

    // chat strings, organised by groups of chat types
    string strings[TOTAL_CHAT_TYPES][MAX_CHAT_STRINGS];

    // counts the number of strings read from the chat file
    int stringCount[TOTAL_CHAT_TYPES];

    // used to avoid repeating the same message twice in a row
    int recentStrings[TOTAL_CHAT_TYPES][5];

public:
    chatClass(void); // constructor, sets up the names of the chat section headers

    void readChatFile(void); // this processes the bot chat file

    // this will fill msg with a randomly selected string from the
    // specified chat section
    void pickRandomChatString(char* msg, size_t maxLength, const int chatSection, const char* playerName);
};

// message script intercept stuff
#define MSG_MAX 64

struct msg_com_struct {
    char ifs[32];
    int blue_av[8];
    int red_av[8];
    int green_av[8];
    int yellow_av[8];
    struct msg_com_struct* next;
};

typedef struct {
    int iId;    // weapon ID
    int iClip;  // amount of ammo in the clip
    int iAmmo1; // amount of ammo in primary reserve
    int iAmmo2; // amount of ammo in secondary reserve
} bot_current_weapon_t;

// a structure for storing bot personality traits
typedef struct {
    short fairplay;      // (1 - 1000) the higher the number the fairer the bot is to other players
                         //	short teamFocus;  // how much the bot wants to help the team(or itself!) win
    short aggression;    // how dominant the bot tends to be
    int faveClass;       // which class the bot prefers
    unsigned camper : 1; // how likely a bot sniper is to stay at the same sniper waypoint after killing someone
    short health;        // minimum health percentage the bot will tolerate
    short humour;        // how keen the bot is on doing daft things
    //	int logoIndex; // choice of graffiti logo
} bot_trait_struct;

// maximum number of jobs storable in the bot's job buffer
#define JOB_BUFFER_MAX 5

// this structure is shared by all the job types in the bot's job buffer
typedef struct {
    float f_bufferedTime; // how long ago this job was put into the buffer
    int priority;         // how important this job is currently

    int phase;         // internal job state, should be zero when job is created
    float phase_timer; // used in various ways by each job type

    int waypoint;                  // waypoint this job concerns
    int waypointTwo;               // a few jobs involve two waypoints
    edict_t* object;               // object this job concerns
    edict_t* player;               // player this job concerns
    Vector origin;                 // map coordinates this job concerns
    char message[MAX_CHAT_LENGTH]; // used only by jobs that involve talking
    //	int generalData;  // general purpose, can be used any way the job sees fit
} job_struct;

// maximum number of jobs that can be blacklisted(kept out of a bot's buffer) simultaneously
#define JOB_BLACKLIST_MAX 5

// a blacklist structure for jobs that fail repetitively(often because of waypoint problems)
typedef struct {
    int type;        // type of job being blacklisted
    float f_timeOut; // time when job should be allowed back into the job buffer
} job_blacklist_struct;

// maximum number of teleport pairs each bot can remember
#define MAX_BOT_TELEPORTER_MEMORY 3

// structure for storing what each bot can know about just one Teleporter pair
typedef struct {
    edict_t* entrance;
    int entranceWP;
    //	edict_t *exit;  // not needed at all!
    int exitWP;
} teleporterPair_struct;

// a structure for data specific to the bots current OR last seen enemy
typedef struct {
    edict_t* ptr; // pointer to the enemy, NULL if no enemies
                  //	unsigned type:4;  // player, sentry gun etc.
    unsigned seenWithFlag : 1;
    //	unsigned isVisible:1;
    // unsigned wasEliminated:1;  // the enemy is not a threat(e.g. they died)
    float f_firstSeen;    // when the bot first encountered the enemy
    float f_lastSeen;     // when they were last seen
                          //	int seenClass;  // the class the bot last saw the enemy as
    float f_seenDistance; // how far away the bots current enemy is
    Vector lastLocation;  // where the bot last saw the enemy
} enemyStruct;

// this is the core data structure used for each bot
typedef struct {
    bool is_used; // set true to indicate an active bot
    edict_t* pEdict;
    int respawn_state;
    unsigned need_to_initialize : 1;
    unsigned not_started : 1;
    char name[BOT_NAME_LEN + 1];
    char skin[BOT_SKIN_LEN + 1];
    int bot_skill;
    int start_action;
    float f_kick_time;
    float create_time;
    int bot_start2;
    float bot_start3;

    // this is used as the bots version of gpGlobals->time
    // because gpGlobals->time appears to run in another thread
    float f_think_time; // set this at the start of botThink()

    bot_trait_struct trait; // bot personality stuff
    float f_humour_time;    // next time the bot may feel like doing something daft

    // buffer for remembering jobs that the bot is interested in doing
    int jobType[JOB_BUFFER_MAX];
    job_struct job[JOB_BUFFER_MAX];

    int currentJob; // which job in the job buffer the bot is currently trying to do

    // a list of job types that are temporarily blacklisted(kept out of the bot's buffer)
    job_blacklist_struct jobBlacklist[JOB_BLACKLIST_MAX];

    // TheFatal - START
    int msecnum;
    float msecdel;
    float msecval;
    // TheFatal - END

    // things from pev in CBasePlayer...
    int current_team; // TFC teams numbered 0 - 3
    int bot_team;     // in TFC teams from 1 - 5 (5 is auto-assign)
    int bot_class;
    int bot_real_health; // actual health value, not health percentage
    int bot_armor;
    int bot_weapons; // bit map of weapons the bot is carrying
    float idle_angle;
    float idle_angle_time;
    float f_blinded_time; // blinded might occur when either team wins all flags on Flagrun on TFC(screen goes black)

    float f_max_speed; // this seems to be the same for all classes
    float prev_speed;

    // variables to help with navigating to a map origin(such as a waypoint) ///////////
    int curr_wp_diff;            // used to spot when the bots waypoint has changed
    float f_progressToWaypoint;  // nearest bot has been to it's current waypoint so far
    float f_navProblemStartTime; // when the bot started to notice it was stuck
    int routeFailureTally;       // used to spot successive failures to get to a waypoint goal

    float f_find_item_time; // when to next check environment for interesting objects

    float f_pause_time;     // remembers when to unpause the bot
    float f_duck_time;      // remembers when the bot should stop crouching
    float f_move_speed;     // forwards or backwards
    float f_side_speed;     // strafing speed
    float f_vertical_speed; // used for swimming vertically
    bool side_direction;
    int strafe_mod; // this can tell bots to stab enemies or heal teammates

    unsigned bot_has_flag : 1;
    float f_dontEvadeTime;   // sets how long the bot should not deviate from it's route
    float f_item_check_time; // used to stop the bots checking for items too frequently.

    int flag_impulse; // used to identify which flag the bot carried when killed

    int current_wp;              // index of the waypoint the bot is at right now
    float f_current_wp_deadline; // the bot should reach it's current waypoint by this time
    int goto_wp;                 // indicates the waypoint the bot wants to get to
    int lastgoto_wp;
    float f_waypoint_drift; // allows bots to approach waypoints less precisely
                            //	Vector waypoint_origin;	// not used at the moment
                            //	float prev_waypoint_distance;	// not used at the moment
                            //	Vector lastLiftOrigin;  // not used at the moment

    // variables used in allowing bots to use alternative routes /////////////
    float f_side_route_time; // remembers when the bot will next consider branching its route
    int sideRouteTolerance;  // how far a bot is currently willing to branch it's route
    int branch_waypoint;     // a waypoint that will lead the bot along a branching route to it's goal

    // the bots memory of operational teleporter pairs that the bot has encountered
    teleporterPair_struct telePair[MAX_BOT_TELEPORTER_MEMORY];

    // is this stuff used in NeoTF or a custom TFC map?
    unsigned b_use_health_station : 1;
    float f_use_health_time;
    unsigned b_use_HEV_station : 1;
    float f_use_HEV_time;

    float f_use_button_time; // remembers when the bot activated a maps button

    // general purpose timer alerts /////////////
    float f_periodicAlertFifth; // goes off every fifth of a second
    float f_periodicAlert1;     // goes off roughly every second
    float f_periodicAlert3;     // goes off roughly every 3 seconds

    // enemy related variables /////////////
    enemyStruct enemy;           // structure for the bots current or last seen enemy
    edict_t* lastEnemySentryGun; // last enemy sentry gun targetted
    float f_enemy_check_time;    // used to stop the bots checking for enemies too frequently.

    short visEnemyCount;         // current number of enemies the bot can see nearby
    short visAllyCount;          // current number of friendly players the bot can see nearby(including itself)
    Vector lastAllyVector;       // where the last seen ally was seen last
    float f_lastAllySeenTime;    // when the bot last saw an ally
    float f_alliedMedicSeenTime; // when the bot last saw a friendly medic

    // variables related to tracking of other players /////////////
    //	short track_reason;  // why the bot is tracking who they are(not used yet)

    float f_bot_spawn_time;      // remembers when the bot last spawned
    float last_spawn_time;       // also remembers when the bot last spawned(dunno why)
    float f_killed_time;         // remembers when the bot last died
    edict_t* killer_edict;       // remembers who last killed this bot
    edict_t* killed_edict;       // remembers who the bot just killed
    bool lockClass;              // set true if you don't want the bots changing class when they feel like it
    short deathsTillClassChange; // remaining deaths till the bot should pick a new class
    int scoreAtSpawn;            // what their score was when they last spawned

    // weapon related variables /////////////////
    float f_shoot_time;         // when to next pull the trigger
    float f_primary_charging;   // Used when charging the primary fire button
    float f_secondary_charging; // Used when charging the secondary fire button
    int charging_weapon_id;
    bot_current_weapon_t current_weapon; // one current weapon for each bot
    int m_rgAmmo[MAX_AMMO_SLOTS];        // total ammo amounts (1 array for each bot)
    short ammoStatus;                    // tracks the bot's overrall ammo situation

    // remembers when a bot can swap back to a potentially suicidal weapon(e.g. RPG)
    // this is used to stop bots constantly swapping certain weapons based on range
    float f_safeWeaponTime;

    // this is used to keep the bots inaccuracy from making it's aim jump
    // from one extreme to another too quickly
    Vector aimDrift;

    float f_disturbedViewTime;  // when the bots aim should no longer be disturbed by concussion, burning etc.
    short disturbedViewAmount;  // if bot is concussed or burning etc. set this higher than 0
    float f_disturbedViewYaw;   // persistant disturbed aim tracker :-)
    float f_disturbedViewPitch; // persistant disturbed aim tracker :-)

    // remembers when the bot will next change it's view(for example, when camping)
    float f_view_change_time;

    // message related variables /////////////////
    unsigned greeting : 1;
    unsigned newmsg : 1; // set true if the bot should check a new message from another player
    char message[255];
    char msgstart[255];
    float f_roleSayDelay; // used to stop bots reporting stuff too often

    // Engineer variables /////////////
    unsigned has_sentry : 1;    // set true if the bot owns a sentry gun
    edict_t* sentry_edict;      // points to the bots sentry gun
    int sentry_ammo;            // ammo in the bots own sentry gun
    int sentryWaypoint;         // used to check if a sentry is in area the map script has closed off
    unsigned SGRotated : 1;     // set true if the engineer needed to rotate their SG and has done so
    unsigned has_dispenser : 1; // set true if the bot owns a dispensor
    edict_t* dispenser_edict;
    float f_dispenserDetTime; // when to blow dispenser if an enemy used it
    edict_t* tpEntrance;      // pointer to engineers own Teleporter
    edict_t* tpExit;          // pointer to engineers own Teleporter
    int tpEntranceWP;         // used to check if teleporter is in area the map script has closed off
    int tpExitWP;             // used to check if teleporter is in area the map script has closed off

    // Demoman variables ////////////////
    int detpack; // used to track if the bot has a detpack

    // Spy related variables ///////////////
    float f_suspectSpyTime;     // don't shoot yet at a suspected Spy during this time
    edict_t* suspectedSpy;      // most recent suspected Spy
    int disguise_state;         // 0 if undisguised, 1 if disguising, 2 if disguised
    float f_disguise_time;      // keeps track of how long it takes for a spy to disguise
    float f_feigningTime;       // when a Spy should stop feigning death
    float f_spyFeignAmbushTime; // Spy bots use this to set up ambushes

    // Sniper related variables ///////////////
    float f_snipe_time; // remembers when a sniper should release the rifle trigger

    // survival related variables ///////////////
    int lastFrameHealth;     // useful for checking how much a bot is getting hurt
    float f_injured_time;    // remembers when the bot was last hurt by someone
    float f_grenadeScanTime; // when the bot should next look for live grenades ahead of it
    float f_soundReactTime;  // remembers when the bot can next look at a sound source

    // Grenade throwing variables, by DrEvil /////////////
    unsigned nadePrimed : 1;
    int nadeType : 8; // remembers what type of grenade the bot has primed
    float primeTime;  // remembers when the bot last primed a grenade
    char grenades[2];
    int tossNade;
    int lastWPIndexChecked; // used for predictive grenade throwing
    float lastDistance;
    float lastDistanceCheck;

    float f_shortcutCheckTime; // when to next check if rocket/concussion jumping might help
    float FreezeDelay;         // for NeoTF Pyros I think

    float f_discard_time; // remembers when the bot may want to next discard unused ammo

    int mission : 8;          // Attacker, Defender, etc.
    unsigned lockMission : 1; // whether the bot should stick to it's current defense/offense role or not
} bot_t;

// roles to fill on the team
enum botRoles { ROLE_NONE, ROLE_ATTACKER, ROLE_DEFENDER };

enum side_direction_values { SIDE_DIRECTION_LEFT, SIDE_DIRECTION_RIGHT };

enum Misc { RJ_WP_INDEX = 0, RJ_WP_TEAM = 1, MAXRJWAYPOINTS = 20 };

enum SpyDisguiseStates { DISGUISE_NONE = 0, DISGUISE_UNDERWAY, DISGUISE_COMPLETE };

enum strafe_mod_values { STRAFE_MOD_NORMAL = 1, STRAFE_MOD_HEAL, STRAFE_MOD_STAB };

enum tracker_reasons {
    TRACK_REASON_ASSIST = 0, // the bot is helping a flag carrier
    TRACK_REASON_SUSPICIOUS, // the bot is following a suspected spy
    TRACK_REASON_TAG,        // the bot has decided to follow a fellow offensive bot
    TRACK_REASON_HEAL_ME,    // the bot wants a spotted medic to heal them
    TRACK_REASON_HUMAN,      // a human has told the bot to follow them
    TRACK_REASON_PURSUIT     // chase after an enemy
};

enum GrenadeSlots { PrimaryGrenade = 0, SecondaryGrenade = 1 };

// these grenade types are specific to TFC
enum GrenadeTypes {
    GRENADE_FRAGMENTATION = 1, // standard issue grenade most TFC classes carry
    GRENADE_CALTROP,
    GRENADE_CONCUSSION,
    GRENADE_EMP,
    GRENADE_FLAME, // Pyro special
    GRENADE_MIRV,
    GRENADE_NAIL,
    GRENADE_SPY_GAS,
    GRENADE_DAMAGE,     // choose a grenade type that is guaranteed to inflict damage
    GRENADE_STATIONARY, // choose grenade type based on a stationary target(e.g. sentry gun)
    GRENADE_RANDOM
}; // choose at random

// general bot ammo level indicators for all weapons any bot is carrying
// pBot->ammoStatus is set to one of these values
enum ammo_levels {
    AMMO_LOW,     // a weapon is running very low
    AMMO_WANTED,  // a weapon is below 50% max capacity
    AMMO_UNNEEDED // all neccessary ammo is above 50%
};

// pent->v.waterlevel constants
enum entity_waterlevels {
    WL_NOT_IN_WATER = 0, // waterlevel 0 - not in water
    WL_FEET_IN_WATER,    // waterlevel 1 - feet in water
    WL_WAIST_IN_WATER,   // waterlevel 2 - waist in water
    WL_HEAD_IN_WATER     // waterlevel 3 - head in water
};

#define MAX_BOTS 32

#define MAX_TEAMS 32
#define MAX_TEAMNAME_LENGTH 16

#define MAX_FLAGS 5

// structure for keeping track of what flags are in each map
typedef struct {
    bool mdl_match;
    int team_no;
    edict_t* edict;
} FLAG_S;

// structure for keeping track of each teams home base
typedef struct {
    int waypoint;             // last waypoint a bot of this team spawned near
    float f_last_update_time; // when a bot on this team last updated the info
} home_struct;

// new UTIL.CPP functions...
/*edict_t *UTIL_FindEntityInSphere( edict_t *pentStart,
        const Vector &vecCenter, float flRadius );
edict_t *UTIL_FindEntityByString( edict_t *pentStart,
        const char *szKeyword, const char *szValue );
edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName );
edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName );
*/
void HUDNotify(edict_t* pEntity, const char* msg_name);

void ClientPrint(edict_t* pEdict, int msg_dest, const char* msg_name);

void UTIL_SayText(const char* pText, edict_t* pEdict);

void UTIL_HostSay(edict_t* pEntity, const int teamonly, char* message);

bool VectorsNearerThan(const Vector& r_vOne, const Vector& r_vTwo, double value);

int UTIL_GetTeamColor(edict_t* pEntity);

int UTIL_GetTeam(edict_t* pEntity);

int UTIL_GetFlagsTeam(const edict_t* flag_edict);

int UTIL_GetClass(edict_t* pEntity);

int UTIL_GetBotIndex(edict_t* pEdict);

bot_t* UTIL_GetBotPointer(edict_t* pEdict);

bool IsAlive(edict_t* pEdict);

bool FInViewCone(const Vector& r_pOrigin, const edict_t* pEdict);

bool FVisible(const Vector& r_vecOrigin, edict_t* pEdict);

Vector Center(edict_t* pEdict);

Vector GetGunPosition(const edict_t* pEdict);

void UTIL_SelectItem(edict_t* pEdict, char* item_name);

Vector VecBModelOrigin(edict_t* pEdict);

bool UTIL_FootstepsHeard(edict_t* pEdict, edict_t* pPlayer);

void UTIL_ShowMenu(edict_t* pEdict, int slots, int displaytime, bool needmore, char* pText);

FILE* UTIL_OpenFoxbotLog(void);

void UTIL_BotLogPrintf(char* fmt, ...);

void UTIL_BuildFileName(char* filename, const int max_fn_length, char* arg1, char* arg2);

bool UTIL_ReadFileLine(char* string, const unsigned int max_length, FILE* file_ptr);

// my functions

edict_t* BotContactThink(bot_t* pBot);

void script(const char* sz);

int PlayerArmorPercent(const edict_t* pEdict);

int PlayerHealthPercent(const edict_t* pEdict);

void UTIL_SavePent(edict_t* pent);

void GetEntvarsKeyvalue(entvars_t* pev, KeyValueData* pkvd);

void BotSprayLogo(edict_t* pEntity, const bool sprayDownwards);

void BotForgetTeleportPair(bot_t* pBot, const int index);

int BotRecallTeleportEntranceIndex(const bot_t* pBot, edict_t* const teleportEntrance);

int BotGetFreeTeleportIndex(const bot_t* pBot);

short BotTeammatesNearWaypoint(const bot_t* pBot, const int waypoint);

edict_t* BotAllyAtVector(const bot_t* pBot, const Vector& r_vecOrigin, const float range, const bool stationaryOnly);

edict_t* BotEntityAtPoint(const char* entityName, const Vector& location, const float range);

bot_t* BotDefenderAtWaypoint(const bot_t* pBot, const int waypoint, const float range);

bool SpyAmbushAreaCheck(bot_t* pBot, Vector& r_wallVector);

void ResetBotHomeInfo(void);

void BotLookAbout(bot_t* pBot);

#endif // BOT_H
