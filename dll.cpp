//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// dll.cpp
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

#include "extdll.h"
#include <enginecallback.h>
//#include "util.h"
//#include "cbase.h"
#include <entity_state.h>

//#include <osdep.h>

#ifndef __linux__
#include <tchar.h>
#endif

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "waypoint.h"

// meta mod includes
#include <dllapi.h>
#include <meta_api.h>

// new stuff for botcam
#include <cbase.h>
#include "player.h"
#include "botcam.h"

#define VER_MAJOR 0
#define VER_MINOR 78
#define VER_BUILD 3

#define MENU_NONE 0
#define MENU_1 1
#define MENU_2 2
#define MENU_3 3
#define MENU_4 4
#define MENU_5 5
#define MENU_6 6
#define MENU_7 7

cvar_t foxbot = { "foxbot", "0.78-b3", FCVAR_SERVER | FCVAR_UNLOGGED, 0, NULL };
cvar_t enable_foxbot = { "enable_foxbot", "1", FCVAR_SERVER | FCVAR_UNLOGGED, 0, NULL };

cvar_t sv_bot = { "bot", "", 0, 0, NULL };

extern GETENTITYAPI other_GetEntityAPI;
extern GETNEWDLLFUNCTIONS other_GetNewDLLFunctions;
extern enginefuncs_t g_engfuncs;
extern int debug_engine;
extern globalvars_t* gpGlobals;
extern char* g_argv;
extern bool g_waypoint_on;
extern bool g_waypoint_cache;
extern bool g_auto_waypoint;
extern bool g_path_waypoint;
extern bool g_find_waypoint;
extern long g_find_wp;
extern bool g_path_connect;

extern bool g_path_oneway;
extern bool g_path_twoway;
extern int wpt1;
extern int wpt2;

extern int pipeCheckFrame;

extern int num_waypoints; // number of waypoints currently in use
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern float wp_display_time[MAX_WAYPOINTS];

extern bot_t bots[32];
extern bool botJustJoined[MAX_BOTS]; // tracks if bot is new to the game

// bot settings //////////////////
bool defensive_chatter = TRUE;
bool offensive_chatter = TRUE;
bool b_observer_mode = FALSE;
bool b_botdontshoot = FALSE;
bool botdontmove = FALSE;
int botchat = 50;
int bot_allow_moods = 1;  // whether bots can have different personality traits or not
int bot_allow_humour = 1; // whether bots can choose to do daft things or not
bool bot_can_use_teleporter = TRUE;
bool bot_can_build_teleporter = TRUE;
int bot_use_grenades = 2;
bool bot_team_balance = FALSE;
static bool bot_bot_balance = FALSE;
int min_bots = -1;
int max_bots = -1;
static int bot_total_varies = 0;
static float bot_create_interval = 3.0f;
int botskill_upper = 1;
int botskill_lower = 3;
int bot_skill_1_aim = 20;   // accuracy for skill 1 bots
int bot_aim_per_skill = 10; // accuracy modifier for bots from skill 1 downwards
bool bot_xmas = FALSE;      // silly stuff
bool g_bot_debug = FALSE;
int spectate_debug = 0; // spectators can trigger debug messages from bots

// waypoint author
extern char waypoint_author[256];

// this tracks the number of bots "wanting" to play on the server
// it should never be less than min_bots or more than max_bots
static int interested_bots = -1;

extern bot_weapon_t weapon_defs[MAX_WEAPONS];

int RoleStatus[] = { 50, 50, 50, 50 };

extern bool g_area_def;
extern AREA areas[MAX_WAYPOINTS];
extern int num_areas;

extern struct msg_com_struct msg_com[MSG_MAX];
extern char msg_msg[64][MSG_MAX];

const static double double_pi = 3.1415926535897932384626433832795;

// define the sources that a bot option/setting can be changed from
// used primarily by the changeBotSetting() function
enum {
    SETTING_SOURCE_CLIENT_COMMAND, // command issued at console by host on Listen server
    SETTING_SOURCE_SERVER_COMMAND, // command issued to the server directly
    SETTING_SOURCE_CONFIG_FILE
}; // command issued by a config file

static FILE* fp;

DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0);

int mod_id = TFC_DLL;
int m_spriteTexture = 0;
static int isFakeClientCommand = 0;
static int fake_arg_count;
static float bot_check_time = 30.0f;
static edict_t* first_player = NULL;
int num_bots = 0;
int prev_num_bots = 0;
bool g_GameRules = FALSE;
edict_t* clients[32];
static float welcome_time = 0.0;
static int welcome_index = -1;
static bool welcome_sent = FALSE;
static int g_menu_waypoint;
static int g_menu_state = 0;

float is_team_play = 0.0;
bool checked_teamplay = FALSE;
// char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
int num_teams = 0;
edict_t* pent_info_tfdetect = NULL;
edict_t* pent_info_ctfdetect = NULL;
edict_t* pent_info_frontline = NULL;
edict_t* pent_item_tfgoal = NULL;
int max_team_players[4];
int team_class_limits[4];
int team_allies[4]; // bit mapped allies BLUE, RED, YELLOW, and GREEN
int max_teams = 0;
FLAG_S flags[MAX_FLAGS];
int num_flags = 0;

static FILE* bot_cfg_fp = NULL;
// changed..cus we set it else where
static bool need_to_open_cfg = FALSE;
static bool need_to_open_cfg2 = FALSE;
static int cfg_file = 1;

// my display stuff...
static bool display_bot_vars = TRUE;
static float display_start_time;
static bool script_loaded = FALSE;
static bool script_parsed = FALSE;

static short scanpos;
static bool player_vis[8];

static float bot_cfg_pause_time = 0.0;
static float respawn_time = 0.0;
static bool spawn_time_reset = FALSE;
bool botcamEnabled = FALSE;

int flf_bug_fix;
int flf_bug_check;

chatClass chat; // bot chat stuff

// waypoint menu entries
char* show_menu_1 = { "Waypoint Tags\n\n1. Team Specific\n2. Locations\n3. Items\n4. Actions p1\n5. Actions p2\n6. "
                      "Control Points\n7. Exit" };
char* show_menu_2 = { "Team Specific Tags\n\n1. Team 1\n2. Team 2\n3. Team 3\n4. Team 4\n5. Exit" };
char* show_menu_3 = { "Location Tags\n\n1. Flag Location\n2. Flag Goal Location\n3. Exit" };
char* show_menu_4 = { "Item Tags\n\n1. Health\n2. Armor\n3. Ammo\n4. Exit" };
char* show_menu_5 = { "Action Tags p1\n\n1. Defend (Soldier/HW/Demo)\n2. Defend(Demo-man Only)\n3. Sniper\n4. Build "
                      "Sentry\n5. Rotate SG 180\n6. Build TP Entrance\n7. Build TP Exit\n8. Exit" };
char* show_menu_6 = { "Action Tags p2\n\n1. RJ/CJ\n2. Jump\n3. Wait For Lift\n4. Walk\n5. Detpack(clear "
                      "passageway)\n6. Detpack(Seal passageway)\n7. Path Check\n8. Exit" };
char* show_menu_7 = {
    "Waypoint Tags\n\n1. Point1\n2. Point2\n3. Point3\n4. Point4\n5. Point5\n6. Point6\n7. Point7\n8. Point8\n9 Exit"
};

// meta mod shiznit
extern bool mr_meta;

char prevmapname[32];

extern bool attack[4]; // teams attack
extern bool defend[4]; // teams defend

extern bool blue_av[8];
extern bool red_av[8];
extern bool green_av[8];
extern bool yellow_av[8];

char arg[255];
int playersPerTeam[4];
bool is_team[4];

float last_frame_time;
float last_frame_time_prev;

extern char sz_error_check[255];

// FUNCTION PROTOTYPES /////////////////
static void DisplayBotInfo();
void BotNameInit(void);
void UpdateClientData(const struct edict_s* ent, int sendweapons, struct clientdata_s* cd);
static void varyBotTotal(void);
static void TeamBalanceCheck(void);
static void BotBalanceTeams_Casual(void);
static bool BotBalanceTeams(int a, int b);
static bool BBotBalanceTeams(int a, int b);
static bool HBalanceTeams(int a, int b);
static void ProcessBotCfgFile(void);
static void changeBotSetting(const char* settingName,
    int* setting,
    const char* arg1,
    const int minValue,
    const int maxValue,
    const int settingSource);
static void kickBots(int totalToKick, const int team);
static void kickRandomBot(void);
static void ClearKickedBotsData(const int botIndex, const bool eraseBotsName);
// void UTIL_CSavePent(CBaseEntity *pent);

void UTIL_HudMessage(CBaseEntity* pEntity, const hudtextparms_t& textparms, const char* pMessage)
{
    if(!pEntity)
        return;

    MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, ENT(pEntity->pev));
    WRITE_BYTE(TE_TEXTMESSAGE);
    WRITE_BYTE(textparms.channel & 0xFF);

    WRITE_SHORT(FixedSigned16(textparms.x, 1 << 13));
    WRITE_SHORT(FixedSigned16(textparms.y, 1 << 13));
    WRITE_BYTE(textparms.effect);

    WRITE_BYTE(textparms.r1);
    WRITE_BYTE(textparms.g1);
    WRITE_BYTE(textparms.b1);
    WRITE_BYTE(textparms.a1);

    WRITE_BYTE(textparms.r2);
    WRITE_BYTE(textparms.g2);
    WRITE_BYTE(textparms.b2);
    WRITE_BYTE(textparms.a2);

    WRITE_SHORT(FixedUnsigned16(textparms.fadeinTime, 1 << 8));
    WRITE_SHORT(FixedUnsigned16(textparms.fadeoutTime, 1 << 8));
    WRITE_SHORT(FixedUnsigned16(textparms.holdTime, 1 << 8));

    if(textparms.effect == 2)
        WRITE_SHORT(FixedUnsigned16(textparms.fxTime, 1 << 8));

    if(strlen(pMessage) < 512) {
        WRITE_STRING(pMessage);
    } else {
        char temp[512];
        strncpy(temp, pMessage, 511);
        temp[511] = '\0';
        WRITE_STRING(temp);
    }

    MESSAGE_END();
}

void KewlHUDNotify(edict_t* pEntity, const char* msg_name)
{
    MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
    WRITE_BYTE(TE_TEXTMESSAGE);
    WRITE_BYTE(3 & 0xFF);
    WRITE_SHORT(FixedSigned16(1, -1 << 13));
    WRITE_SHORT(FixedSigned16(1, -1 << 13));
    WRITE_BYTE(2); // effect

    WRITE_BYTE(205);
    WRITE_BYTE(205);
    WRITE_BYTE(10);
    WRITE_BYTE(255);

    WRITE_BYTE(0);
    WRITE_BYTE(0);
    WRITE_BYTE(255);
    WRITE_BYTE(255);
    WRITE_SHORT(FixedUnsigned16(0.03, 1 << 8));
    WRITE_SHORT(FixedUnsigned16(1, 1 << 8));
    WRITE_SHORT(FixedUnsigned16(6, 1 << 8));
    WRITE_SHORT(FixedUnsigned16(4, 1 << 8));
    WRITE_STRING(msg_name);
    MESSAGE_END();
}

// If bot_total_varies is active then this function will periodically alter
// the number of bots that want to play on the server.
static void varyBotTotal(void)
{
    if(bot_total_varies == 0)
        return; // just in case

    // this governs when the number of bots wanting to play will next change
    static float f_interested_bots_change = 0;

    if(f_interested_bots_change < gpGlobals->time) {
        if(bot_total_varies == 3) // busy server(players coming/going often)
            f_interested_bots_change = gpGlobals->time + random_float(10.0f, 120.0f);
        else if(bot_total_varies == 2)
            f_interested_bots_change = gpGlobals->time + random_float(40.0f, 360.0f);
        else // slow changes in number of bots
            f_interested_bots_change = gpGlobals->time + random_float(90.0f, 600.0f);

        //	UTIL_BotLogPrintf("interested_bots:%d, time:%f, next change:%f\n",
        //	interested_bots, gpGlobals->time, f_interested_bots_change);

        // try and get some bots interested in joining the
        // game if the game has just started
        if(interested_bots < 0) {
            if(max_bots > 0 && max_bots > min_bots) {
                if(min_bots > -1)
                    interested_bots = random_long(min_bots, max_bots);
                else
                    interested_bots = random_long(1, max_bots);
            } else
                interested_bots = min_bots;
        }

        // randomly increase/decrease the number of interested bots
        if(max_bots > 0 && max_bots > min_bots) {
            if(random_long(1, 1000) > 500) {
                // favor increasing the bots
                // if max_bots is reached decrease the bots
                if(interested_bots < max_bots)
                    ++interested_bots;
                else if(min_bots > 0) {
                    if(interested_bots > min_bots)
                        --interested_bots;
                } else if(interested_bots > 0)
                    --interested_bots;
            } else {
                // favor decreasing the bots
                // if min_bots is reached increase the bots
                if(min_bots > 0 && interested_bots > min_bots)
                    --interested_bots;
                else if(min_bots < 1 && interested_bots > 0)
                    --interested_bots;
                else if(interested_bots < max_bots)
                    ++interested_bots;
            }
        }

        //	UTIL_BotLogPrintf("interested_bots changed to:%d\n", interested_bots);
    } else {
        // make sure f_interested_bots_change is sane
        // (gpGlobals->time resets on map change)
        if((f_interested_bots_change - 601.0f) > gpGlobals->time)
            f_interested_bots_change = gpGlobals->time + random_float(10.0f, 120.0f);
    }
}

// This function is the core function for checking if the teams need balancing
// and making sure the bots switch teams if they are supposed to.
static void TeamBalanceCheck(void)
{
    if(mod_id != TFC_DLL)
        return;

    if(bot_team_balance && !bot_bot_balance) {
        // team 1 has more than team 2?
        bool done = BotBalanceTeams(1, 2);
        if(!done)
            done = BotBalanceTeams(1, 3);
        if(!done)
            done = BotBalanceTeams(1, 4);

        if(!done)
            done = BotBalanceTeams(2, 1);
        if(!done)
            done = BotBalanceTeams(2, 3);
        if(!done)
            done = BotBalanceTeams(2, 4);

        if(!done)
            done = BotBalanceTeams(3, 1);
        if(!done)
            done = BotBalanceTeams(3, 2);
        if(!done)
            done = BotBalanceTeams(3, 4);

        if(!done)
            done = BotBalanceTeams(4, 1);
        if(!done)
            done = BotBalanceTeams(4, 2);
        if(!done)
            done = BotBalanceTeams(4, 3);
    }
    if(bot_bot_balance) {
        // team 1 has more than team 2?
        bool done = BBotBalanceTeams(1, 2);
        if(!done)
            done = BBotBalanceTeams(1, 3);
        if(!done)
            done = BBotBalanceTeams(1, 4);

        if(!done)
            done = BBotBalanceTeams(2, 1);
        if(!done)
            done = BBotBalanceTeams(2, 3);
        if(!done)
            done = BBotBalanceTeams(2, 4);

        if(!done)
            done = BBotBalanceTeams(3, 1);
        if(!done)
            done = BBotBalanceTeams(3, 2);
        if(!done)
            done = BBotBalanceTeams(3, 4);

        if(!done)
            done = BBotBalanceTeams(4, 1);
        if(!done)
            done = BBotBalanceTeams(4, 2);
        if(!done)
            done = BBotBalanceTeams(4, 3);

        if(!done && bot_team_balance) {
            // balance the humans!
            // team 1 has more than team 2?
            done = HBalanceTeams(1, 2);
            if(!done)
                done = HBalanceTeams(1, 3);
            if(!done)
                done = HBalanceTeams(1, 4);

            if(!done)
                done = HBalanceTeams(2, 1);
            if(!done)
                done = HBalanceTeams(2, 3);
            if(!done)
                done = HBalanceTeams(2, 4);

            if(!done)
                done = HBalanceTeams(3, 1);
            if(!done)
                done = HBalanceTeams(3, 2);
            if(!done)
                done = HBalanceTeams(3, 4);

            if(!done)
                done = HBalanceTeams(4, 1);
            if(!done)
                done = HBalanceTeams(4, 2);
            if(!done)
                HBalanceTeams(4, 3);
        }
    }

    // if auto-balance is switched off then let the bots
    // balance the teams if they "feel" like it
    else if(!bot_team_balance && !bot_bot_balance)
        BotBalanceTeams_Casual();
}

// This function should only be called if bot_team_balance is switched off.
// It allows bots to decide for themselves if they "feel" like switching teams
// to balance the teams.
static void BotBalanceTeams_Casual(void)
{
    if(mod_id != TFC_DLL)
        return;

    static float nextBalanceCheck = 5.0f;

    // perform a balance check every random number of seconds
    if(nextBalanceCheck < gpGlobals->time)
        nextBalanceCheck = gpGlobals->time + random_float(30.0f, 120.0f);
    else {
        // make sure nextBalanceCheck is sane(gpGlobals->time resets on map change)
        if((nextBalanceCheck - 600.0f) > gpGlobals->time)
            nextBalanceCheck = gpGlobals->time + random_float(15.0f, 120.0f);
        return;
    }

    // find out which teams have player limits, and are full
    int i;
    bool team_is_full[4] = { FALSE, FALSE, FALSE, FALSE };
    for(i = 0; i < 4; i++) {
        if(max_team_players[i] > 0 && playersPerTeam[i] >= max_team_players[i])
            team_is_full[i] = TRUE;
    }

    /*	UTIL_BotLogPrintf("team_is_full[] - 0:%d, 1:%d, 2:%d, 3:%d\n",
                    team_is_full[0], team_is_full[1], team_is_full[2], team_is_full[3]);*/

    // find out which teams have the most and the fewest players
    int smallest_team = -1;
    int biggest_team = -1;
    int size_difference = -1;
    for(i = 0; i < 3; i++) {
        // stop comparing if the next team isn't used on this map
        if(is_team[i + 1] == FALSE)
            break;

        // is this team bigger than the next one?
        if(!team_is_full[i + 1] && playersPerTeam[i] > playersPerTeam[i + 1]) {
            biggest_team = i;
            smallest_team = i + 1;
            size_difference = playersPerTeam[i] - playersPerTeam[i + 1];
        }
        // or is the next team bigger?
        else if(!team_is_full[i] && playersPerTeam[i] < playersPerTeam[i + 1]) {
            biggest_team = i + 1;
            smallest_team = i;
            size_difference = playersPerTeam[i + 1] - playersPerTeam[i];
        }

        if(size_difference > 1)
            break; // we've found a big enough imbalance
    }

    //	UTIL_BotLogPrintf("\nbiggest_team:%d, smallest_team:%d, size_difference:%d\n",
    //	biggest_team + 1, smallest_team + 1, size_difference);

    if(size_difference < 2 || biggest_team < 0 || smallest_team < 0)
        return;

    // add one to both teams as the bots use team numbers 1 to 4, not 0 to 3
    ++biggest_team;
    ++smallest_team;

    // let only one bot change team(if it wants to) once per function call
    for(i = 0; i < 32; i++) {
        // is this an active bot on the bigger team?
        // and is it willing to change teams?
        if(bots[i].is_used && bots[i].pEdict->v.team == biggest_team && random_long(1, 1000) < bots[i].trait.fairplay) {
            //	UTIL_BotLogPrintf("vteam:%d, team:%d\n",
            //		bots[i].pEdict->v.team, bots[i].bot_team);

            char msg[16];
            _snprintf(msg, 16, "%d", smallest_team);
            msg[15] = '\0';
            //	FakeClientCommand(bots[i].pEdict, "jointeam", msg, NULL);
            bots[i].bot_team = smallest_team; // choose your team
            bots[i].not_started = TRUE;       // join the team, pick a class
            bots[i].start_action = MSG_TFC_IDLE;
            bots[i].create_time = gpGlobals->time + 2.0;
            ClearKickedBotsData(i, FALSE);

            //	UTIL_BotLogPrintf("joined team:%d, vteam:%d, team:%d\n",
            //		smallest_team, bots[i].pEdict->v.team, bots[i].bot_team);

            break; // job done, success!
        }
    }
}

static bool BotBalanceTeams(int a, int b)
{
    if(playersPerTeam[a - 1] - 1 > playersPerTeam[b - 1] &&
        (max_team_players[b - 1] > playersPerTeam[b - 1] || max_team_players[b - 1] == 0) && is_team[b - 1]) {
        for(int i = 31; i >= 0; i--) {
            // is this slot used?
            if(bots[i].is_used && bots[i].pEdict->v.team == a) {
                char msg[32];
                _snprintf(msg, 32, "%d", b);
                //	FakeClientCommand(bots[i].pEdict, "jointeam", msg, NULL);
                bots[i].bot_team = b;       // choose your team
                bots[i].not_started = TRUE; // join the team, pick a class
                bots[i].start_action = MSG_TFC_IDLE;
                bots[i].create_time = gpGlobals->time + 2.0;
                ClearKickedBotsData(i, FALSE);
                return TRUE;
            }
        }
    }
    return FALSE;
}

// used with the bot_bot_balance setting
static bool BBotBalanceTeams(int a, int b)
{
    // now just set up teams to include bots in them :D
    int bteams[4] = { 0, 0, 0, 0 };

    for(int i = 0; i < 32; i++) //<32
    {
        if(bots[i].is_used) {
            char* infobuffer;
            char cl_name[128];
            cl_name[0] = '\0';
            infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(bots[i].pEdict);
            strcpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
            if(cl_name[0] != '\0') {
                int team = (bots[i].pEdict->v.team) - 1;

                if(team >= 0 && team < 4)
                    bteams[team] = bteams[team] + 1;
            }
        }
    }
    if(bteams[a - 1] - 1 > bteams[b - 1] &&
        (max_team_players[b - 1] > playersPerTeam[b - 1] || max_team_players[b - 1] == 0) && is_team[b - 1]) {
        for(int i = 31; i >= 0; i--) {
            // is this slot used?
            if(bots[i].is_used && bots[i].pEdict->v.team == a) {
                char msg[32];
                _snprintf(msg, 32, "%d", b);
                //	FakeClientCommand(bots[i].pEdict, "jointeam", msg, NULL);
                bots[i].bot_team = b;       // choose your team
                bots[i].not_started = TRUE; // join the team, pick a class
                bots[i].start_action = MSG_TFC_IDLE;
                bots[i].create_time = gpGlobals->time + 2.0;
                ClearKickedBotsData(i, FALSE);
                return TRUE;
            }
        }
    }
    return FALSE;
}

// This function will balance the human players amongst the teams by forcing
// them to switch teams if necessary.
static bool HBalanceTeams(int a, int b)
{
    if((playersPerTeam[a - 1] - 1) > playersPerTeam[b - 1] &&
        (max_team_players[b - 1] > playersPerTeam[b - 1] || max_team_players[b - 1] == 0) && is_team[b - 1]) {
        for(int i = 1; i <= 32; i++) {
            bool not_bot = TRUE;
            for(int j = 1; j <= 32; j++) {
                if(bots[j].is_used && bots[j].pEdict == INDEXENT(i))
                    not_bot = FALSE;
            }

            if(not_bot && INDEXENT(i) != NULL) {
                if(INDEXENT(i)->v.team == a && INDEXENT(i)->v.netname != 0) {
                    CLIENT_COMMAND(INDEXENT(i), UTIL_VarArgs("jointeam %d\n", b));
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// This function is called once upon startup
void GameDLLInit(void)
{
    CVAR_REGISTER(&foxbot);
    CVAR_REGISTER(&sv_bot);
    CVAR_REGISTER(&enable_foxbot);

    for(int i = 0; i < 32; i++)
        clients[i] = NULL;

    // initialize the bots array of structures...
    memset(bots, 0, sizeof(bots));

    // read the bot names from the bot name file
    BotNameInit();

    // read the chat strings from the bot chat file
    chat.readChatFile();

    if(!mr_meta)
        (*other_gFunctionTable.pfnGameInit)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

// Constructor for the chatClass class
// sets up the names of the chat section headers
chatClass::chatClass(void)
{
    this->sectionNames[CHAT_TYPE_GREETING] = "[GREETINGS]";
    this->sectionNames[CHAT_TYPE_KILL_HI] = "[KILL WINNING]";
    this->sectionNames[CHAT_TYPE_KILL_LOW] = "[KILL LOSING]";
    this->sectionNames[CHAT_TYPE_KILLED_HI] = "[KILLED WINNING]";
    this->sectionNames[CHAT_TYPE_KILLED_LOW] = "[KILLED LOSING]";
    this->sectionNames[CHAT_TYPE_SUICIDE] = "[SUICIDE]";

    int i, j;

    // explicitly clear all the chat strings
    for(i = 0; i < TOTAL_CHAT_TYPES; i++) {
        this->stringCount[i] = 0;
        for(j = 0; j < MAX_CHAT_STRINGS; j++) {
            this->strings[i][j] = "";
        }

        for(j = 0; j < 5; j++) {
            this->recentStrings[i][j] = -1;
        }
    }
}

// This function is responsible for reading in the chat from the bot chat file.
void chatClass::readChatFile(void)
{
    char filename[256];

    UTIL_BuildFileName(filename, 255, "foxbot_chat.txt", NULL);
    FILE* bfp = fopen(filename, "r");

    if(bfp == NULL) {
        UTIL_BotLogPrintf("Unable to read from the Foxbot chat file.  The bots will not chat.");
        botchat = 0; // stop the bots trying to chat
        return;
    }

    char buffer[MAX_CHAT_LENGTH] = "";
    char* ptr;
    int i;
    size_t length;
    int chat_section = -1;

    while(UTIL_ReadFileLine(buffer, MAX_CHAT_LENGTH, bfp)) {
        // ignore comment lines
        if(buffer[0] == '#')
            continue;

        length = strlen(buffer);

        // turn '\n' into '\0'
        if(buffer[length - 1] == '\n') {
            buffer[length - 1] = 0;
            length--;
        }

        // change %n to %s
        if((ptr = strstr(buffer, "%n")) != NULL)
            *(ptr + 1) = 's';

        // watch out for the chat section headers
        if(buffer[0] == '[') {
            bool newSectionFound = FALSE;
            for(i = 0; i < TOTAL_CHAT_TYPES; i++) {
                if(buffer == this->sectionNames[i]) {
                    chat_section = i;
                    newSectionFound = TRUE;
                }
            }
            if(newSectionFound)
                continue;
        }

        // this line is not a comment, empty, or a section header
        // so treat it as a chat string and load it up
        if(chat_section != -1 && this->stringCount[chat_section] < MAX_CHAT_STRINGS && length > 0) {
            // UTIL_BotLogPrintf("buffer %s %d\n", buffer, this->stringCount[chat_section]);

            this->strings[chat_section][this->stringCount[chat_section]] = buffer;
            ++this->stringCount[chat_section];
        }
    }

    fclose(bfp);
}

// This function will return a C ASCII string pointer to a randomly selected
// chat message of the type defined by chatSection.
// Some chat strings use a players name with the "%n" specifier, so you can
// specify a players name with playerName, or set it to NULL.
void chatClass::pickRandomChatString(char* msg, size_t maxLength, const int chatSection, const char* playerName)
{
    msg[0] = '\0'; // just in case

    // make sure this chat section contains at least one chat string
    if(this->stringCount[chatSection] < 1)
        return;

    int i, recentCount = 0;
    int randomIndex;
    bool used;

    // try to pick a string that hasn't been used recently
    while(recentCount < 5) {
        randomIndex = random_long(0, this->stringCount[chatSection] - 1);
        used = FALSE;
        for(i = 0; i < 5; i++) {
            if(this->recentStrings[chatSection][i] == randomIndex)
                used = TRUE;
        }

        if(used)
            ++recentCount;
        else
            break; // found an unused chat string
    }

    // push the selected string on to the list of recently used strings
    for(i = 4; i > 0; i--) {
        this->recentStrings[chatSection][i] = this->recentStrings[chatSection][i - 1];
    }
    this->recentStrings[chatSection][0] = randomIndex;

    // set up the message string
    // is "%s" in the text?
    if(playerName != NULL && strstr(this->strings[chatSection][randomIndex].c_str(), "%s") != NULL) {
        _snprintf(msg, maxLength, this->strings[chatSection][randomIndex].c_str(), playerName);
    } else
        _snprintf(msg, maxLength, this->strings[chatSection][randomIndex].c_str());

    msg[maxLength - 1] = '\0';
}

int DispatchSpawn(edict_t* pent)
{
    if(gpGlobals->deathmatch) {
        char* pClassname = (char*)STRING(pent->v.classname);

        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "DispatchSpawn: %p %s\n", (void*)pent, pClassname);
            if(pent->v.model != 0)
                fprintf(fp, " model=%s\n", STRING(pent->v.model));
            if(pent->v.target != 0)
                fprintf(fp, " t=%s\n", STRING(pent->v.target));
            if(pent->v.targetname != 0)
                fprintf(fp, " tn=%s\n", STRING(pent->v.targetname));
            fclose(fp);
        }

        if(strcmp(pClassname, "worldspawn") == 0) {
            // do level initialization stuff here...

            WaypointInit();
            WaypointLoad(NULL);
            AreaDefLoad(NULL);

            // my clear var for lev reload..
            strcpy(prevmapname, "null");

            pent_info_tfdetect = NULL;
            pent_info_ctfdetect = NULL;
            pent_info_frontline = NULL;
            pent_item_tfgoal = NULL;

            int index;
            for(index = 0; index < 4; index++) {
                max_team_players[index] = 0;  // no player limit
                team_class_limits[index] = 0; // no class limits
                team_allies[index] = 0;
            }

            max_teams = 0;
            is_team[0] = FALSE;
            is_team[1] = FALSE;
            is_team[2] = FALSE;
            is_team[3] = FALSE;
            num_flags = 0;

            g_waypoint_cache = FALSE;
            if(g_waypoint_on || g_area_def) {
                // precache stuff here
                PRECACHE_MODEL("sprites/dot.spr");
                PRECACHE_MODEL("sprites/arrow1.spr");
                PRECACHE_MODEL("sprites/gargeye1.spr");
                PRECACHE_MODEL("sprites/cnt1.spr");
                PRECACHE_MODEL("models/w_medkit.mdl");
                PRECACHE_MODEL("models/r_armor.mdl");
                PRECACHE_MODEL("models/backpack.mdl");
                PRECACHE_MODEL("models/p_sniper.mdl");
                PRECACHE_MODEL("models/flag.mdl");
                PRECACHE_MODEL("models/bigrat.mdl");
                PRECACHE_MODEL("models/sentry1.mdl");
                PRECACHE_MODEL("models/w_longjump.mdl");
                PRECACHE_MODEL("models/detpack.mdl");
                PRECACHE_MODEL("models/teleporter.mdl");
                PRECACHE_MODEL("models/mechgibs.mdl");
                PRECACHE_SOUND("weapons/xbow_hit1.wav");     // waypoint add
                PRECACHE_SOUND("weapons/mine_activate.wav"); // waypoint delete
                PRECACHE_SOUND("common/wpn_hudoff.wav");     // path add/delete start
                PRECACHE_SOUND("common/wpn_hudon.wav");      // path add/delete done
                PRECACHE_SOUND("common/wpn_moveselect.wav"); // path add/delete cancel
                PRECACHE_SOUND("common/wpn_denyselect.wav"); // path add/delete error

                g_waypoint_cache = TRUE;
            }

            PRECACHE_MODEL("models/presentlg.mdl");
            PRECACHE_MODEL("models/presentsm.mdl");

            PRECACHE_SOUND("barney/c1a4_ba_octo4.wav");
            PRECACHE_SOUND("misc/b2.wav");
            PRECACHE_SOUND("misc/party2.wav");
            PRECACHE_SOUND("misc/party1.wav");

            m_spriteTexture = PRECACHE_MODEL("sprites/lgtning.spr");

            g_GameRules = TRUE;

            is_team_play = 0.0;
            //	memset(team_names, 0, sizeof(team_names));
            num_teams = 0;
            checked_teamplay = FALSE;

            respawn_time = 0.0;
            spawn_time_reset = FALSE;

            prev_num_bots = num_bots;
            num_bots = 0;

            flf_bug_fix = 0;
            flf_bug_check = 0;

            bot_check_time = gpGlobals->time + 30.0;
        }
    }

    if(!mr_meta)
        return (*other_gFunctionTable.pfnSpawn)(pent);
    else
        RETURN_META_VALUE(MRES_HANDLED, 0);
}

void DispatchThink(edict_t* pent)
{
    if(FStrEq(STRING(pent->v.classname), "entity_botcam")) {
        TraceResult tr;
        int off_f = 16;
        UTIL_MakeVectors(pent->v.euser1->v.v_angle);
        if(pent->v.euser1 != NULL && !FNullEnt(pent->v.euser1) && pent->v.owner != NULL && !FNullEnt(pent->v.owner)) {
            bot_t* pBot = UTIL_GetBotPointer(pent->v.euser1);

            UTIL_TraceLine(pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs + gpGlobals->v_forward * off_f,
                pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs + gpGlobals->v_forward * 4000, ignore_monsters,
                pent->v.euser1, &tr);

            Vector st;
            Vector end;

            MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pent->v.owner);
            WRITE_BYTE(TE_TEXTMESSAGE);
            WRITE_BYTE(2 & 0xFF);
            WRITE_SHORT(FixedSigned16(1, -1 << 13));
            WRITE_SHORT(FixedSigned16(1, 0 << 13));
            WRITE_BYTE(1); // effect

            WRITE_BYTE(255);
            WRITE_BYTE(255);
            WRITE_BYTE(255);
            WRITE_BYTE(255);

            WRITE_BYTE(255);
            WRITE_BYTE(255);
            WRITE_BYTE(255);
            WRITE_BYTE(255);
            WRITE_SHORT(FixedUnsigned16(1, 1 << 8));
            WRITE_SHORT(FixedUnsigned16(3, 1 << 8));
            WRITE_SHORT(FixedUnsigned16(1, 1 << 8));

            // Try putting together a string for more information in botcam
            char msg[255];
            if(pBot->mission == ROLE_ATTACKER) {
                _snprintf(msg, 254, "BotCam: Role:Attacker\n ammoStatus:%d deathsTillClassChange:%d trait.health:%d",
                    pBot->ammoStatus, pBot->deathsTillClassChange, pBot->trait.health);
                WRITE_STRING(msg);
            } else if(pBot->mission == ROLE_DEFENDER) {
                _snprintf(msg, 254, "BotCam: Role:Defender\n ammoStatus:%d deathsTillClassChange:%d trait.health:%d",
                    pBot->ammoStatus, pBot->deathsTillClassChange, pBot->trait.health);
                WRITE_STRING(msg);
            } else {
                _snprintf(msg, 254, "BotCam: Role:None\n ammoStatus:%d deathsTillClassChange:%d trait.health:%d",
                    pBot->ammoStatus, pBot->deathsTillClassChange, pBot->trait.health);
                WRITE_STRING(msg);
            }
            MESSAGE_END();

            if(pBot->enemy.ptr != NULL) {
                float zz = pBot->enemy.ptr->v.maxs.z;
                zz = (zz / 4) / 2;
                float xx = pBot->enemy.ptr->v.maxs.x;
                xx = (xx / 2);

                if(scanpos > 1)
                    zz = zz * 2;
                if(scanpos > 3)
                    zz = zz * 2;
                if(scanpos > 5)
                    zz = zz * 2;
                if(scanpos == 0 || scanpos == 2 || scanpos == 4 || scanpos == 6)
                    xx = -xx;
                float xo = xx;
                Vector vang = pBot->enemy.ptr->v.origin - pBot->pEdict->v.origin;
                float distance = vang.Length();
                vang = UTIL_VecToAngles(vang);
                vang.y = vang.y + 45;
                if(vang.y < 0)
                    vang.y += 360;
                vang.y = vang.y * double_pi;
                vang.y = vang.y / 180;
                xx = xo * cos(vang.y);
                float yy = xo * sin(vang.y);

                tr.pHit = NULL;
                UTIL_TraceLine(pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs,
                    pBot->enemy.ptr->v.origin + Vector(xx, yy, zz), dont_ignore_monsters, dont_ignore_glass,
                    pent->v.euser1, &tr);

                if(tr.pHit == pBot->enemy.ptr || tr.flFraction == 1.0)
                    player_vis[scanpos] = TRUE;
                else
                    player_vis[scanpos] = FALSE;
                scanpos++;
                if(scanpos > 7) // 7
                    scanpos = 0;
                float p_vis = 0;
                for(int i = 0; i < 8; i++) {
                    if(player_vis[i])
                        p_vis = p_vis + 12.5;
                }

                char msg[511];
                int amb = 0;
                int vidsize = 2;
                float sz = ((pBot->enemy.ptr->v.maxs.z) * (vidsize / distance)) * 100;
                int d = GETENTITYILLUM(pBot->enemy.ptr);
                if(amb == 0) {
                    edict_t* pent = NULL;
                    edict_t* pPoint = NULL;
                    while((pent = FIND_ENTITY_IN_SPHERE(pent, pBot->enemy.ptr->v.origin, 50)) != NULL &&
                        (!FNullEnt(pent)) && amb == 0) {
                        if(strcmp(STRING(pent->v.classname), "entity_botlightvalue") == 0)
                            pPoint = pent;
                    }
                    if(pPoint == NULL) {
                        pPoint = CREATE_NAMED_ENTITY(MAKE_STRING("info_target"));
                        DispatchSpawn(pPoint);
                        pPoint->v.origin = pBot->enemy.ptr->v.origin;
                        pPoint->v.takedamage = DAMAGE_NO;
                        pPoint->v.solid = SOLID_NOT;
                        pPoint->v.owner = pBot->enemy.ptr;
                        pPoint->v.movetype = MOVETYPE_FLY; // noclip
                        pPoint->v.classname = MAKE_STRING("entity_botlightvalue");
                        pPoint->v.nextthink = gpGlobals->time;
                        pPoint->v.rendermode = kRenderNormal;
                        pPoint->v.renderfx = kRenderFxNone;
                        pPoint->v.renderamt = 0;
                        SET_MODEL(pPoint, "models/mechgibs.mdl");
                        amb = GETENTITYILLUM(pPoint);
                    } else {
                        amb = GETENTITYILLUM(pPoint);
                        REMOVE_ENTITY(pPoint);
                    }
                }

                Vector v = pBot->pEdict->v.v_angle;
                v.y = v.y + 180;
                if(v.y > 180)
                    v.y -= 360;
                if(v.y < -180)
                    v.y += 360;

                double dgrad = static_cast<double>(v.y);
                dgrad = dgrad + 180;
                if(dgrad > 180)
                    dgrad -= 360;
                if(dgrad < -180)
                    dgrad += 360;
                dgrad = dgrad * double_pi;
                dgrad = dgrad / 180;

                Vector vel = pBot->enemy.ptr->v.velocity;
                vel.x = vel.x * sin(dgrad);
                vel.y = vel.y * cos(dgrad);
                dgrad = static_cast<double>(v.x);
                dgrad = dgrad + 180;
                if(dgrad > 180)
                    dgrad -= 360;
                if(dgrad < -180)
                    dgrad += 360;
                dgrad = dgrad * double_pi;
                dgrad = dgrad / 180;
                vel.z = vel.z * cos(dgrad);

                if(vel.x < 0)
                    vel.x = -vel.x;
                if(vel.y < 0)
                    vel.y = -vel.y;
                if(vel.z < 0)
                    vel.z = -vel.z;

                float veloff = vel.x + vel.y + vel.z;
                veloff = (veloff * (vidsize / distance)) * 10;

                _snprintf(msg, 510, "Vis %.1f\nAmb %d %d\nSz %.1f\nveloff %.1f", p_vis, amb, d, sz, veloff);

                MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pent->v.owner);
                WRITE_BYTE(TE_TEXTMESSAGE);
                WRITE_BYTE(4 & 0xFF);
                WRITE_SHORT(FixedSigned16(0.3, 1 << 13));
                WRITE_SHORT(FixedSigned16(0.3, 1 << 13));
                WRITE_BYTE(1); // effect
                WRITE_BYTE(255);
                WRITE_BYTE(255);
                WRITE_BYTE(255);
                WRITE_BYTE(255);
                WRITE_BYTE(255);
                WRITE_BYTE(255);
                WRITE_BYTE(255);
                WRITE_BYTE(255);
                WRITE_SHORT(FixedUnsigned16(0, 1 << 8));
                WRITE_SHORT(FixedUnsigned16(0, 1 << 8));
                WRITE_SHORT(FixedUnsigned16(1, 1 << 8));
                WRITE_STRING(msg);
                MESSAGE_END();

                MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pent->v.owner);
                WRITE_BYTE(TE_BOX);
                st = pBot->enemy.ptr->v.absmin;
                end = pBot->enemy.ptr->v.absmax;
                WRITE_COORD(st.x);
                WRITE_COORD(st.y);
                WRITE_COORD(st.z);
                WRITE_COORD(end.x);
                WRITE_COORD(end.y);
                WRITE_COORD(end.z);
                WRITE_SHORT(1);  // life in 0.1's
                WRITE_BYTE(255); // r, g, b
                WRITE_BYTE(0);   // r, g, b
                WRITE_BYTE(0);   // r, g, b
                MESSAGE_END();
            }
            // this could be adapted to highlight an object the bot is interested in
            // pBot->pBotPickupItem is no longer used
            /*	else if(pBot->pBotPickupItem != NULL)
                    {
                            MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pent->v.owner);
                            WRITE_BYTE( TE_BOX);
                            st = pBot->pBotPickupItem->v.absmin;
                            end = pBot->pBotPickupItem->v.absmax;
                            WRITE_COORD(st.x);
                            WRITE_COORD(st.y);
                            WRITE_COORD(st.z);
                            WRITE_COORD(end.x);
                            WRITE_COORD(end.y);
                            WRITE_COORD(end.z);
                            WRITE_SHORT( 1 ); // life in 0.1's
                            WRITE_BYTE( 0);   // r, g, b
                            WRITE_BYTE( 255 );   // r, g, b
                            WRITE_BYTE( 0 );   // r, g, b
                            MESSAGE_END();
                    }*/
        }
        // UTIL_MakeVectors(pent->v.euser1->v.v_angle);
        UTIL_TraceLine(pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs,
            pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs +
                gpGlobals->v_forward * off_f, // + gpGlobals->v_up * 10
            ignore_monsters,
            pent->v.euser1, &tr);

        // UTIL_SetOrigin(pent, tr.vecEndPos);
        pent->v.origin = tr.vecEndPos;

        pent->v.angles = pent->v.euser1->v.v_angle;
        pent->v.velocity = Vector(0, 0, 0);
        if(pent->v.angles.y >= 360)
            pent->v.angles.y -= 360;
        if(pent->v.angles.y < 0)
            pent->v.angles.y += 360;
        pent->v.nextthink = gpGlobals->time;
    }
    if(!mr_meta)
        (*other_gFunctionTable.pfnThink)(pent);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void DispatchUse(edict_t* pentUsed, edict_t* pentOther)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "DispatchUse: used %p other %p\n", (void*)pentUsed, (void*)pentOther);
        fclose(fp);
    }

    if(!mr_meta)
        (*other_gFunctionTable.pfnUse)(pentUsed, pentOther);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void DispatchTouch(edict_t* pentTouched, edict_t* pentOther)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void DispatchBlocked(edict_t* pentBlocked, edict_t* pentOther)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnBlocked)(pentBlocked, pentOther);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void DispatchKeyValue(edict_t* pentKeyvalue, KeyValueData* pkvd)
{
    static edict_t* temp_pent;
    static int flag_index;

    if(mod_id == TFC_DLL) {
        if(pentKeyvalue == pent_info_tfdetect) {
            if(strcmp(pkvd->szKeyName, "ammo_medikit") == 0) // max BLUE players
                max_team_players[0] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "ammo_detpack") == 0) // max RED players
                max_team_players[1] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_medikit") == 0) // max YELLOW players
                max_team_players[2] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_detpack") == 0) // max GREEN players
                max_team_players[3] = atoi(pkvd->szValue);

            else if(strcmp(pkvd->szKeyName, "maxammo_shells") == 0) // BLUE class limits
                team_class_limits[0] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_nails") == 0) // RED class limits
                team_class_limits[1] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_rockets") == 0) // YELLOW class limits
                team_class_limits[2] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_cells") == 0) // GREEN class limits
                team_class_limits[3] = atoi(pkvd->szValue);

            else if(strcmp(pkvd->szKeyName, "team1_allies") == 0) // BLUE allies
                team_allies[0] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "team2_allies") == 0) // RED allies
                team_allies[1] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "team3_allies") == 0) // YELLOW allies
                team_allies[2] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "team4_allies") == 0) // GREEN allies
                team_allies[3] = atoi(pkvd->szValue);
        } else if(pent_info_tfdetect == NULL) {
            if((strcmp(pkvd->szKeyName, "classname") == 0) && (strcmp(pkvd->szValue, "info_tfdetect") == 0)) {
                pent_info_tfdetect = pentKeyvalue;
            }
        }

        if(pentKeyvalue == pent_item_tfgoal) {
            if(strcmp(pkvd->szKeyName, "team_no") == 0)
                flags[flag_index].team_no = atoi(pkvd->szValue);

            if((strcmp(pkvd->szKeyName, "mdl") == 0) && ((strcmp(pkvd->szValue, "models/flag.mdl") == 0) ||
                                                            (strcmp(pkvd->szValue, "models/keycard.mdl") == 0) ||
                                                            (strcmp(pkvd->szValue, "models/ball.mdl") == 0))) {
                flags[flag_index].mdl_match = TRUE;
                num_flags++;
            }
        } else if(pent_item_tfgoal == NULL) {
            if((strcmp(pkvd->szKeyName, "classname") == 0) && (strcmp(pkvd->szValue, "item_tfgoal") == 0)) {
                if(num_flags < MAX_FLAGS) {
                    pent_item_tfgoal = pentKeyvalue;

                    flags[num_flags].mdl_match = FALSE;
                    flags[num_flags].team_no = 0; // any team unless specified
                    flags[num_flags].edict = pentKeyvalue;

                    flag_index = num_flags; // in case the mdl comes before team_no
                }
            }
        } else
            pent_item_tfgoal = NULL; // reset for non-flag item_tfgoal's

        if((strcmp(pkvd->szKeyName, "classname") == 0) &&
            ((strcmp(pkvd->szValue, "info_player_teamspawn") == 0) ||
                (strcmp(pkvd->szValue, "info_tf_teamcheck") == 0) || (strcmp(pkvd->szValue, "i_p_t") == 0))) {
            temp_pent = pentKeyvalue;
        } else if(pentKeyvalue == temp_pent) {
            if(strcmp(pkvd->szKeyName, "team_no") == 0) {
                int value = atoi(pkvd->szValue);

                is_team[value - 1] = TRUE;
                if(value > max_teams)
                    max_teams = value;
            }
        }
    } /*else if(mod_id == GEARBOX_DLL) {
        if(pent_info_ctfdetect == NULL) {
            if((strcmp(pkvd->szKeyName, "classname") == 0) && (strcmp(pkvd->szValue, "info_ctfdetect") == 0))
                pent_info_ctfdetect = pentKeyvalue;
        }
    }*/

    if(!mr_meta)
        (*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void DispatchSave(edict_t* pent, SAVERESTOREDATA* pSaveData)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSave)(pent, pSaveData);
    else
        SET_META_RESULT(MRES_HANDLED);
}

int DispatchRestore(edict_t* pent, SAVERESTOREDATA* pSaveData, int globalEntity)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnRestore)(pent, pSaveData, globalEntity);
    else {
        SET_META_RESULT(MRES_HANDLED);
        return (0);
    }
}

void DispatchObjectCollsionBox(edict_t* pent)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSetAbsBox)(pent);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void SaveWriteFields(SAVERESTOREDATA* pSaveData,
    const char* pname,
    void* pBaseData,
    TYPEDESCRIPTION* pFields,
    int fieldCount)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSaveWriteFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void SaveReadFields(SAVERESTOREDATA* pSaveData,
    const char* pname,
    void* pBaseData,
    TYPEDESCRIPTION* pFields,
    int fieldCount)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSaveReadFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void SaveGlobalState(SAVERESTOREDATA* pSaveData)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSaveGlobalState)(pSaveData);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void RestoreGlobalState(SAVERESTOREDATA* pSaveData)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnRestoreGlobalState)(pSaveData);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void ResetGlobalState(void)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnResetGlobalState)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

BOOL ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
    if(gpGlobals->deathmatch) {
        // int count = 0;
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            if(fp != NULL) {
                fprintf(fp, "ClientConnect: pent=%p name=%s\n", (void*)pEntity, pszName);
                fclose(fp);
            }
        }

        // ALERT(at_console, "[FOXBOT] This server is running FoxBot (%d build %d), Get it at foxbot.net!\n", VER_MINOR,
        // VER_BUILD);
        if(!mr_meta) {
            char msg[255];
            // sprintf(msg, "[FOXBOT] This server is running FoxBot (v %d build %d), Get it at foxbot.net!\n",
            // VER_MINOR, VER_BUILD);
            // sprintf(msg, "[FOXBOT] This server is running FoxBot (v%d.%d), Get it at foxbot.net!\n", VER_MAJOR,
            // VER_MINOR);
            _snprintf(msg, 254, "[FOXBOT] This server is running FoxBot (v%d.%d), get it at www.apg-clan.org\n",
                VER_MAJOR, VER_MINOR);
            CLIENT_PRINTF(pEntity, print_console, msg);
        }

        // check if this is NOT a bot joining the server...
        if(strcmp(pszAddress, "127.0.0.1") != 0) {
            int i = 0;
            while((i < 32) && (clients[i] != NULL && clients[i] != pEntity))
                i++;
            if(i < 32)
                clients[i] = pEntity;
            if(welcome_index == -1)
                welcome_index = i;
            // don't try to add bots for 30 seconds, give client time to get added
            bot_check_time = gpGlobals->time + 30.0;
            // save the edict of the first player to join this server...
            if(first_player == NULL)
                first_player = pEntity;
        }
    }

    if(!mr_meta)
        return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
    else
        RETURN_META_VALUE(MRES_HANDLED, TRUE);
}

BOOL ClientConnect_Post(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
    if(gpGlobals->deathmatch) {
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            if(fp != NULL) {
                fprintf(fp, "ClientConnect_Post: pent=%p name=%s\n", (void*)pEntity, pszName);
                fclose(fp);
            }
        }

        // ALERT(at_console, "[FOXBOT] This server is running FoxBot (%d build %d), Get it at foxbot.net!\n", VER_MINOR,
        // VER_BUILD);
        char msg[255];
        // sprintf(msg, "[FOXBOT] This server is running FoxBot (v %d build %d), Get it at foxbot.net!\n", VER_MINOR,
        // VER_BUILD);
        // sprintf(msg, "[FOXBOT] This server is running FoxBot (v%d.%d), Get it at foxbot.net!\n", VER_MAJOR,
        // VER_MINOR);
        _snprintf(msg, 254, "[FOXBOT] This server is running FoxBot (v%d.%d), get it at www.apg-clan.org\n", VER_MAJOR,
            VER_MINOR);
        CLIENT_PRINTF(pEntity, print_console, msg);
    }
    RETURN_META_VALUE(MRES_HANDLED, TRUE);
}

void ClientDisconnect(edict_t* pEntity)
{
    if(gpGlobals->deathmatch) {
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            if(fp != NULL) {
                fprintf(fp, "ClientDisconnect: %p\n", (void*)pEntity);
                fclose(fp);
            }
        }

        int i, index = -1;

        for(i = 0; i < 32; i++) {
            if(bots[i].pEdict == pEntity && bots[i].is_used) {
                index = i;
                break;
            }
        }

        if(index != -1) {
            // someone has kicked this bot off of the server...

            ClearKickedBotsData(index, FALSE);
            bots[index].is_used = FALSE;               // this bot index is now free to re-use
            bots[index].f_kick_time = gpGlobals->time; // save the kicked time
        } else {
            i = 0;

            while((i < 32) && (clients[i] != pEntity))
                i++;

            if(i < 32)
                clients[i] = NULL;
            // human left?
            // what about level changes?
        }
    }

    if(!mr_meta)
        (*other_gFunctionTable.pfnClientDisconnect)(pEntity);
    else {
        RETURN_META(MRES_HANDLED);
    }
}

void ClientKill(edict_t* pEntity)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "ClientKill: %p\n", (void*)pEntity);
        fclose(fp);
    }
    if(!mr_meta)
        (*other_gFunctionTable.pfnClientKill)(pEntity);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void ClientPutInServer(edict_t* pEntity)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "ClientPutInServer: %p\n", (void*)pEntity);
        fclose(fp);
    }

    if(!mr_meta)
        (*other_gFunctionTable.pfnClientPutInServer)(pEntity);
    else {
        RETURN_META(MRES_HANDLED);
    }
}

void ClientCommand(edict_t* pEntity)
{
    if(mod_id == TFC_DLL && pEntity != NULL) {
        const char* pcmd = CMD_ARGV(0);
        const char* arg1 = CMD_ARGV(1);
        const char* arg2 = CMD_ARGV(2);
        const char* arg3 = CMD_ARGV(3);
        const char* arg4 = CMD_ARGV(4);
        // char msg[80];
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "ClientCommand: %s %p", pcmd, (void*)pEntity);
            if((arg1 != NULL)) {
                if((*arg1 != 0))
                    fprintf(fp, " 1:%s", arg1);
            }
            if((arg2 != NULL)) {
                if((*arg2 != 0))
                    fprintf(fp, " 2:%s", arg2);
            }
            if((arg3 != NULL)) {
                if((*arg3 != 0))
                    fprintf(fp, " 3:%s", arg3);
            }
            if((arg4 != NULL)) {
                if((*arg4 != 0))
                    fprintf(fp, " 4:%s", arg4);
            }
            fprintf(fp, "\n");
            fclose(fp);
        }
    }

    if((gpGlobals->deathmatch) && (!IS_DEDICATED_SERVER()) && pEntity == INDEXENT(1)) {
        const char* pcmd = CMD_ARGV(0);
        const char* arg1 = CMD_ARGV(1);
        const char* arg2 = CMD_ARGV(2);
        const char* arg3 = CMD_ARGV(3);
        const char* arg4 = CMD_ARGV(4);
        char msg[512];

        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "ClientCommand: %s %p", pcmd, (void*)pEntity);
            if((arg1 != NULL)) {
                if((*arg1 != 0))
                    fprintf(fp, " '%s'(1)", arg1);
            }
            if((arg2 != NULL)) {
                if((*arg2 != 0))
                    fprintf(fp, " '%s'(2)", arg2);
            }
            if((arg3 != NULL)) {
                if((*arg3 != 0))
                    fprintf(fp, " '%s'(3)", arg3);
            }
            if((arg4 != NULL)) {
                if((*arg4 != 0))
                    fprintf(fp, " '%s'(4)", arg4);
            }
            fprintf(fp, "\n");
            fclose(fp);
        }

        if(FStrEq(pcmd, "addbot") || FStrEq(pcmd, "foxbot_addbot")) {
            if(arg2 != NULL && (*arg2 != 0))
                BotCreate(pEntity, arg1, arg2, arg3, arg4);
            else {
                char c[8];
                strcpy(c, "-1");
                BotCreate(pEntity, arg1, c, arg3, arg4);
            }
            bot_check_time = gpGlobals->time + 5.0;
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "min_bots")) {
            changeBotSetting("min_bots", &min_bots, arg1, -1, 31, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "max_bots")) {
            changeBotSetting("max_bots", &max_bots, arg1, -1, MAX_BOTS, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_total_varies")) {
            changeBotSetting("bot_total_varies", &bot_total_varies, arg1, 0, 3, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_info")) {
            DisplayBotInfo();
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_team_balance")) {
            if((arg1 != NULL)) {
                if((*arg1 != 0)) {
                    int temp = atoi(arg1);
                    if(temp)
                        bot_team_balance = TRUE;
                    else
                        bot_team_balance = FALSE;
                }
            }
            if(bot_team_balance)
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_team_balance (1) On\n");
            else
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_team_balance (0) Off\n");
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_bot_balance")) {
            if((arg1 != NULL)) {
                if((*arg1 != 0)) {
                    int temp = atoi(arg1);
                    if(temp)
                        bot_bot_balance = TRUE;
                    else
                        bot_bot_balance = FALSE;
                }
            }

            if(bot_bot_balance)
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_bot_balance (1) On\n");
            else
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_bot_balance (0) Off\n");

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "kickall") || FStrEq(pcmd, "foxbot_kickall")) {
            // kick all bots off of the server after time/frag limit...
            kickBots(MAX_BOTS, -1);
        } else if(FStrEq(pcmd, "kickteam") || FStrEq(pcmd, "foxbot_kickteam")) {
            if(arg1 != NULL && *arg1 != 0) {
                int whichTeam = atoi(arg1);
                kickBots(MAX_BOTS, whichTeam);
            }
        } else if(FStrEq(pcmd, "observer")) {
            if(arg1 != NULL && *arg1 != 0) {
                int temp = atoi(arg1);
                if(temp)
                    b_observer_mode = TRUE;
                else
                    b_observer_mode = FALSE;
            }

            if(b_observer_mode)
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
            else
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "botskill_lower")) {
            changeBotSetting("botskill_lower", &botskill_lower, arg1, 1, 5, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "botskill_upper")) {
            changeBotSetting("botskill_upper", &botskill_upper, arg1, 1, 5, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_skill_1_aim")) {
            changeBotSetting("bot_skill_1_aim", &bot_skill_1_aim, arg1, 0, 200, SETTING_SOURCE_CLIENT_COMMAND);

            BotUpdateSkillInaccuracy();
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_aim_per_skill")) {
            changeBotSetting("bot_aim_per_skill", &bot_aim_per_skill, arg1, 5, 50, SETTING_SOURCE_CLIENT_COMMAND);

            BotUpdateSkillInaccuracy();
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "spectate_debug")) {
            changeBotSetting("spectate_debug", &spectate_debug, arg1, 0, 4, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "botdontshoot")) {
            if((arg1 != NULL)) {
                if((*arg1 != 0)) {
                    int temp = atoi(arg1);
                    if(temp)
                        b_botdontshoot = TRUE;
                    else
                        b_botdontshoot = FALSE;
                }
            }

            if(b_botdontshoot)
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot ENABLED\n");
            else
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot DISABLED\n");

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "botdontmove")) {
            if((arg1 != NULL)) {
                if((*arg1 != 0)) {
                    int temp = atoi(arg1);
                    if(temp)
                        b_botdontshoot = TRUE;
                    else
                        b_botdontshoot = FALSE;
                }
            }

            if(b_botdontshoot)
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontmove ENABLED\n");
            else
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontmove DISABLED\n");

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "debug_engine") || FStrEq(pcmd, "foxbot_debug_engine")) {
            if(FStrEq(arg1, "on")) {
                debug_engine = 1;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine enabled!\n");
            } else if(FStrEq(arg1, "off")) {
                debug_engine = 0;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine disabled!\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        }

        // botcam
        else if(FStrEq(pcmd, "botcam")) {
            edict_t* pBot = NULL;
            char botname[BOT_NAME_LEN + 1];
            int index;

            botname[0] = 0;

            if((arg1 != NULL)) {
                if((*arg1 != 0)) {
                    if(strstr(arg1, "\"") == NULL)
                        strcpy(botname, arg1);
                    else
                        sscanf(arg1, "\"%s\"", &botname[0]);

                    index = 0;

                    while(index < 32) {
                        if((bots[index].is_used) && (stricmp(bots[index].name, botname) == 0))
                            break;
                        else
                            index++;
                    }

                    if(index < 32) {
                        pBot = bots[index].pEdict;
                    }
                }
            } else {
                index = 0;

                while((bots[index].is_used == FALSE) && (index < 32))
                    index++;

                if(index < 32) {
                    pBot = bots[index].pEdict;
                }
            }

            if(pBot == NULL) {
                if(botname[0])
                    CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs("there is no bot named \"%s\"!\n", botname));
                else
                    CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs("there are no bots!\n"));
            } else {
                if(!g_waypoint_cache)
                    CLIENT_PRINTF(pEntity, print_console,
                        "Turn waypoints on so proper entities can be cached. Then retry botcam.");
                else {
                    KillCamera(pEntity);
                    CreateCamera(pEntity, pBot);
                    botcamEnabled = TRUE;
                }
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "nobotcam")) {
            KillCamera(pEntity);
            botcamEnabled = FALSE;
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "waypoint_author")) {
            if((arg1 != NULL)) {
                if((*arg1 != 0)) {
                    char msg[512];
                    sprintf(msg, "Waypoint author set to : %s", arg1);
                    CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs(msg));
                    strncpy(waypoint_author, arg1, 250);
                    waypoint_author[251] = '\0';
                    CBaseEntity* p;
                    p = CBaseEntity::Instance(INDEXENT(1));
                    hudtextparms_t h;
                    h.channel = 2;
                    h.effect = 1;
                    h.r1 = 255;
                    h.g1 = 128;
                    h.b1 = 0;
                    h.a1 = 255;
                    h.r2 = 255;
                    h.g2 = 170;
                    h.b2 = 0;
                    h.a2 = 255;
                    h.fadeinTime = 1;
                    h.fadeoutTime = 1;
                    h.holdTime = 7;
                    h.x = -1;
                    h.y = 0.8;
                    sprintf(msg, "-- Waypoint author: %s --", waypoint_author);
                    UTIL_HudMessage(p, h, msg);
                }
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "waypoint")) {
            if(FStrEq(arg1, "on")) {
                g_waypoint_on = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_waypoint_on = FALSE;
                g_path_oneway = FALSE;
                g_path_twoway = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
            } else if(FStrEq(arg1, "add")) {
                if(!g_waypoint_on)
                    g_waypoint_on = TRUE; // turn waypoints on if off

                WaypointAdd(pEntity);
            } else if(FStrEq(arg1, "delete") && g_waypoint_on) {
                if(!g_waypoint_on)
                    g_waypoint_on = TRUE; // turn waypoints on if off

                WaypointDelete(pEntity);
            } else if(FStrEq(arg1, "save") && g_waypoint_on) {
                WaypointSave();

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints saved\n");
            } else if(FStrEq(arg1, "load")) {
                if(WaypointLoad(pEntity))
                    ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");
            } else if(FStrEq(arg1, "menu") && g_waypoint_on) {
                int index = WaypointFindNearest_E(pEntity, 50.0, -1);
                if(num_waypoints > 0 && index != -1) {
                    g_menu_waypoint = index;
                    g_menu_state = MENU_1;

                    UTIL_ShowMenu(pEntity, 0x7F, -1, FALSE, show_menu_1);
                    //	UTIL_ShowMenu(pEntity, 0x3F, -1, FALSE, show_menu_1);
                }
            } else if(FStrEq(arg1, "info")) {
                WaypointPrintInfo(pEntity);
            } else if(FStrEq(arg1, "autobuild")) {
                if(!g_waypoint_on)
                    g_waypoint_on = TRUE; // turn waypoints on if off

                WaypointAutoBuild(pEntity);
            } else {
                if(g_waypoint_on)
                    ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
                else
                    ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
            }

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "autowaypoint")) {
            if(FStrEq(arg1, "on")) {
                g_auto_waypoint = TRUE;
                g_waypoint_on = TRUE; // turn this on just in case
            } else if(FStrEq(arg1, "off")) {
                g_auto_waypoint = FALSE;
            }

            if(g_auto_waypoint)
                sprintf(msg, "autowaypoint is ON\n");
            else
                sprintf(msg, "autowaypoint is OFF\n");

            ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_debug")) {
            if(FStrEq(arg1, "on")) {
                CVAR_SET_STRING("developer", "1");
                g_bot_debug = TRUE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot debugging is ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_bot_debug = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot debugging is OFF\n");
                CVAR_SET_STRING("developer", "0");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "available")) {
            int blue_state, red_state, yellow_state, green_state;
            for(int i = 0; i < 8; i++) {
                blue_state = red_state = yellow_state = green_state = 0;

                if(blue_av[i])
                    blue_state = 1;
                if(red_av[i])
                    red_state = 1;
                if(green_av[i])
                    green_state = 1;
                if(yellow_av[i])
                    yellow_state = 1;

                _snprintf(msg, 250, "POINT:%d, AVAILABILITY: Blue %d Red %d Yellow %d Green %d\n", i + 1, blue_state,
                    red_state, yellow_state, green_state);
                msg[251] = '\0';
                ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
            }
            hudtextparms_t h;
            h.channel = 4;
            h.effect = 1;
            h.r1 = 10;
            h.g1 = 53;
            h.b1 = 81;
            h.a1 = 255;
            h.r2 = 10;
            h.g2 = 53;
            h.b2 = 81;
            h.a2 = 168;
            h.fadeinTime = 1;
            h.fadeoutTime = 1;
            h.holdTime = 20;
            h.x = 0;
            h.y = 0;
            CBaseEntity* p;
            p = CBaseEntity::Instance(INDEXENT(1));
            UTIL_HudMessage(p, h, msg);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_chat")) {
            changeBotSetting("bot_chat", &botchat, arg1, 0, 1000, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_xmas")) {
            if(FStrEq(arg1, "on")) {
                bot_xmas = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot xmas is ON\n");
            } else if(FStrEq(arg1, "off")) {
                bot_xmas = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot xmas is OFF\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_allow_moods")) {
            changeBotSetting("bot_allow_moods", &bot_allow_moods, arg1, 0, 1, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_allow_humour")) {
            changeBotSetting("bot_allow_humour", &bot_allow_humour, arg1, 0, 1, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_use_grenades")) {
            changeBotSetting("bot_use_grenades", &bot_use_grenades, arg1, 0, 2, SETTING_SOURCE_CLIENT_COMMAND);

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_can_use_teleporter")) // bot_can_use_teleporter - by yuraj
        {
            if(FStrEq(arg1, "on")) {
                bot_can_use_teleporter = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_use_teleporter is ON\n");
            } else if(FStrEq(arg1, "off")) {
                bot_can_use_teleporter = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_use_teleporter is OFF\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "bot_can_build_teleporter")) // bot_can_build_teleporter - by yuraj
        {
            if(FStrEq(arg1, "on")) {
                bot_can_build_teleporter = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_build_teleporter is ON\n");
            } else if(FStrEq(arg1, "off")) {
                bot_can_build_teleporter = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_build_teleporter is OFF\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "locate_waypoint")) {
            if(FStrEq(arg1, "on")) {
                g_find_waypoint = TRUE;
                g_path_waypoint = TRUE;
                g_waypoint_on = TRUE; // turn this on just in case
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "locate waypoint is ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_find_waypoint = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "locate waypoint is OFF\n");
            } else {
                g_find_waypoint = TRUE;
                g_waypoint_on = TRUE; // turn this on just in case

                int l_waypoint = atoi(arg1);
                if(l_waypoint <= num_waypoints) {
                    g_find_wp = l_waypoint;
                    char s[255];
                    sprintf(s, "locate waypoint %d\n", l_waypoint);
                    ClientPrint(pEntity, HUD_PRINTNOTIFY, s);
                }
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "pathwaypoint_connect")) {
            if(FStrEq(arg1, "on")) {
                g_path_waypoint = TRUE;
                g_waypoint_on = TRUE; // turn this on just in case
                g_path_connect = TRUE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint auto connect is ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_path_connect = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint auto connect is OFF\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "pathrun_oneway")) {
            if(FStrEq(arg1, "on")) {
                wpt1 = -1;
                wpt2 = -1;
                g_path_waypoint = TRUE;
                g_waypoint_on = TRUE; // turn this on just in case
                g_path_connect = FALSE;
                g_path_oneway = TRUE;
                g_path_twoway = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun oneway ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_path_oneway = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun oneway OFF\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "pathrun_twoway")) {
            if(FStrEq(arg1, "on")) {
                wpt1 = -1;
                wpt2 = -1;
                g_path_waypoint = TRUE;
                g_waypoint_on = TRUE; // turn this on just in case
                g_path_connect = FALSE;
                g_path_oneway = FALSE;
                g_path_twoway = TRUE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun twoway ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_path_twoway = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun twoway OFF\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "pathwaypoint")) {
            if(FStrEq(arg1, "on")) {
                g_path_waypoint = TRUE;
                g_waypoint_on = TRUE; // turn this on just in case

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_path_waypoint = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is OFF\n");
            } else if(FStrEq(arg1, "create1") && g_path_waypoint) {
                WaypointCreatePath(pEntity, 1);
            } else if(FStrEq(arg1, "create2") && g_path_waypoint) {
                WaypointCreatePath(pEntity, 2);
            } else if(FStrEq(arg1, "remove1") && g_path_waypoint) {
                WaypointRemovePath(pEntity, 1);
            } else if(FStrEq(arg1, "remove2") && g_path_waypoint) {
                WaypointRemovePath(pEntity, 2);
            }

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "area")) {
            if(FStrEq(arg1, "on")) {
                g_area_def = TRUE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is ON\n");
            } else if(FStrEq(arg1, "off")) {
                g_area_def = FALSE;

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is OFF\n");
            } else if(FStrEq(arg1, "add")) {
                g_area_def = TRUE;
                AreaDefCreate(pEntity);
            } else if(FStrEq(arg1, "delete") && g_area_def) {
                g_area_def = TRUE;
                AreaDefDelete(pEntity);
            } else if(FStrEq(arg1, "save") && g_area_def) {
                AreaDefSave();

                ClientPrint(pEntity, HUD_PRINTNOTIFY, "areas saved\n");
            } else if(FStrEq(arg1, "load")) {
                if(AreaDefLoad(pEntity))
                    ClientPrint(pEntity, HUD_PRINTNOTIFY, "areas loaded\n");
            } else if(FStrEq(arg1, "info")) {
                AreaDefPrintInfo(pEntity);
            } else if(FStrEq(arg1, "name") && g_area_def) {
                int i = AreaInsideClosest(pEntity);
                if(i != -1 && strlen(arg2) < 64) {
                    strcpy(areas[i].namea, arg2);
                    strcpy(areas[i].nameb, arg2);
                    strcpy(areas[i].namec, arg2);
                    strcpy(areas[i].named, arg2);
                }
                AreaDefPrintInfo(pEntity);
            } else if(FStrEq(arg1, "name1") && g_area_def) {
                int i = AreaInsideClosest(pEntity);
                if(i != -1 && strlen(arg2) < 64) {
                    strcpy(areas[i].namea, arg2);
                }
                AreaDefPrintInfo(pEntity);
            } else if(FStrEq(arg1, "name2") && g_area_def) {
                int i = AreaInsideClosest(pEntity);
                if(i != -1 && strlen(arg2) < 64) {
                    strcpy(areas[i].nameb, arg2);
                }
                AreaDefPrintInfo(pEntity);
            } else if(FStrEq(arg1, "name3") && g_area_def) {
                int i = AreaInsideClosest(pEntity);
                if(i != -1 && strlen(arg2) < 64) {
                    strcpy(areas[i].namec, arg2);
                }
                AreaDefPrintInfo(pEntity);
            } else if(FStrEq(arg1, "name4") && g_area_def) {
                int i = AreaInsideClosest(pEntity);
                if(i != -1 && strlen(arg2) < 64) {
                    strcpy(areas[i].named, arg2);
                }
                AreaDefPrintInfo(pEntity);
            } else if(FStrEq(arg1, "autobuild1")) {
                g_area_def = TRUE;

                AreaAutoBuild1();

                // ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is ON\n");
            } else if(FStrEq(arg1, "merge")) {
                g_area_def = TRUE;

                AreaAutoMerge();

                // ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is ON\n");
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else if(FStrEq(pcmd, "menuselect") && (g_menu_state != MENU_NONE)) {
            if(g_menu_state == MENU_1) // main menu...
            {
                if(FStrEq(arg1, "1")) // team specific...
                {
                    g_menu_state = MENU_2; // display team specific menu...
                    UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_2);
                    if(mr_meta)
                        RETURN_META(MRES_SUPERCEDE);
                    return;
                } else if(FStrEq(arg1, "2")) // display locations menu
                {
                    g_menu_state = MENU_3;
                    UTIL_ShowMenu(pEntity, 0x07, -1, FALSE, show_menu_3);

                    if(mr_meta)
                        RETURN_META(MRES_SUPERCEDE);
                    return;
                } else if(FStrEq(arg1, "3")) // display items menu
                {
                    g_menu_state = MENU_4;
                    UTIL_ShowMenu(pEntity, 0x0F, -1, FALSE, show_menu_4);

                    if(mr_meta)
                        RETURN_META(MRES_SUPERCEDE);
                    return;
                } else if(FStrEq(arg1, "4")) // display actions menu 1
                {
                    g_menu_state = MENU_5;
                    UTIL_ShowMenu(pEntity, 0xFF, -1, FALSE, show_menu_5);

                    if(mr_meta)
                        RETURN_META(MRES_SUPERCEDE);
                    return;
                } else if(FStrEq(arg1, "5")) // display actions menu 2
                {
                    g_menu_state = MENU_6;
                    UTIL_ShowMenu(pEntity, 0xFF, -1, FALSE, show_menu_6);

                    if(mr_meta)
                        RETURN_META(MRES_SUPERCEDE);
                    return;
                } else if(FStrEq(arg1, "6")) // control points
                {
                    g_menu_state = MENU_7;
                    UTIL_ShowMenu(pEntity, 0x1FF, -1, FALSE, show_menu_7);
                    if(mr_meta)
                        RETURN_META(MRES_SUPERCEDE);
                    return;
                }
            } else if(g_menu_state == MENU_2) // team specific tags
            {
                int team = atoi(arg1);
                team--; // make 0 - 3
                if((waypoints[g_menu_waypoint].flags & W_FL_TEAM_SPECIFIC) && (team >= 0 && team <= 3)) {
                    waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM;
                    waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM_SPECIFIC; // off
                } else {
                    if(team >= 0 && team <= 3) {
                        waypoints[g_menu_waypoint].flags |= team;
                        waypoints[g_menu_waypoint].flags |= W_FL_TEAM_SPECIFIC; // on
                    }
                }
            } else if(g_menu_state == MENU_3) // location tags
            {
                if(mod_id == TFC_DLL) {
                    if(FStrEq(arg1, "1")) // flag location
                    {
                        if(waypoints[g_menu_waypoint].flags & W_FL_TFC_FLAG)
                            waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_FLAG; // off
                        else
                            waypoints[g_menu_waypoint].flags |= W_FL_TFC_FLAG; // on
                    } else if(FStrEq(arg1, "2"))                               // flag goal
                    {
                        if(waypoints[g_menu_waypoint].flags & W_FL_TFC_FLAG_GOAL)
                            waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_FLAG_GOAL; // off
                        else
                            waypoints[g_menu_waypoint].flags |= W_FL_TFC_FLAG_GOAL; // on
                    }
                }
            } else if(g_menu_state == MENU_4) // item tags
            {
                if(FStrEq(arg1, "1")) // set health
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_HEALTH)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_HEALTH; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_HEALTH; // on
                } else if(FStrEq(arg1, "2"))                             // set armor
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_ARMOR)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_ARMOR; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_ARMOR; // on
                } else if(FStrEq(arg1, "3"))                            // set ammo
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_AMMO)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_AMMO; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_AMMO; // on
                }
            } else if(g_menu_state == MENU_5) // action tags
            {
                if(FStrEq(arg1, "1")) // set soldier/hw defender
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_PL_DEFEND)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_PL_DEFEND; // off
                    else {
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_PL_DEFEND; // on
                        WaypointAddAiming(pEntity);
                    }
                } else if(FStrEq(arg1, "2")) // set demo defender
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_PIPETRAP)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_PIPETRAP; // off
                    else {
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_PIPETRAP; // on
                        WaypointAddAiming(pEntity);
                    }
                } else if(FStrEq(arg1, "3")) // sniper spot
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_SNIPER)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_SNIPER; // off
                    else {
                        waypoints[g_menu_waypoint].flags |= W_FL_SNIPER; // on
                        // set the aiming waypoint...
                        WaypointAddAiming(pEntity);
                    }
                } else if(FStrEq(arg1, "4")) // set sentry
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_SENTRY)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_SENTRY; // off
                    else {
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_SENTRY; // on
                        WaypointAddAiming(pEntity);
                    }
                } else if(FStrEq(arg1, "5")) // set 180 sentry
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_SENTRY) {
                        if(waypoints[g_menu_waypoint].flags & W_FL_TFC_SENTRY_180)
                            waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_SENTRY_180; // off
                        else {
                            waypoints[g_menu_waypoint].flags |= W_FL_TFC_SENTRY_180; // on
                        }
                    } else {
                        ClientPrint(pEntity, HUD_PRINTNOTIFY, "This MUST be placed on an existing sentry waypoint\n");
                    }
                } else if(FStrEq(arg1, "6")) // set to teleport entrance
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_TELEPORTER_ENTRANCE)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_TELEPORTER_ENTRANCE; // off
                    else {
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_TELEPORTER_ENTRANCE; // on
                        WaypointAddAiming(pEntity);
                    }
                } else if(FStrEq(arg1, "7")) // set to teleport exit
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_TELEPORTER_EXIT)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_TELEPORTER_EXIT; // off
                    else {
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_TELEPORTER_EXIT; // on
                        WaypointAddAiming(pEntity);
                    }
                }
            } else if(g_menu_state == MENU_6) // action tags p2
            {
                if(FStrEq(arg1, "1")) // RJ CJ
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_JUMP)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_JUMP; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_JUMP; // on
                } else if(FStrEq(arg1, "2"))                               // set jump
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_JUMP)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_JUMP; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_JUMP; // on
                } else if(FStrEq(arg1, "3"))                           // wait for lift...
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_LIFT)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_LIFT; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_LIFT; // on
                } else if(FStrEq(arg1, "4"))                           // walk waypoint
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_WALK)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_WALK; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_WALK; // on
                } else if(FStrEq(arg1, "5"))                           // set detpack(blow passage open only)
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_DETPACK_CLEAR)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_DETPACK_CLEAR; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_DETPACK_CLEAR; // on
                } else if(FStrEq(arg1, "6")) // set detpack(blow passage closed only)
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_TFC_DETPACK_SEAL)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_DETPACK_SEAL; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_TFC_DETPACK_SEAL; // on
                } else if(FStrEq(arg1, "7"))                                       // check if path is obstructed
                {
                    if(waypoints[g_menu_waypoint].flags & W_FL_PATHCHECK)
                        waypoints[g_menu_waypoint].flags &= ~W_FL_PATHCHECK; // off
                    else
                        waypoints[g_menu_waypoint].flags |= W_FL_PATHCHECK; // on
                }
            } else if(g_menu_state == MENU_7) // control point tags
            {
                if(FStrEq(arg1, "1")) // point1
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT1)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT1; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT1; // on
                } else if(FStrEq(arg1, "2"))                                    // point2
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT2)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT2; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT2; // on
                } else if(FStrEq(arg1, "3"))                                    // point3
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT3)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT3; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT3; // on
                } else if(FStrEq(arg1, "4"))                                    // point4
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT4)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT4; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT4; // on
                } else if(FStrEq(arg1, "5"))                                    // point5
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT5)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT5; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT5; // on
                } else if(FStrEq(arg1, "6"))                                    // point6
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT6)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT6; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT6; // on
                } else if(FStrEq(arg1, "7"))                                    // point7
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT7)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT7; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT7; // on
                } else if(FStrEq(arg1, "8"))                                    // point8
                {
                    if(waypoints[g_menu_waypoint].script_flags & S_FL_POINT8)
                        waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT8; // off
                    else
                        waypoints[g_menu_waypoint].script_flags |= S_FL_POINT8; // on
                }
                g_menu_state = MENU_NONE;
            }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;

        } else if(FStrEq(pcmd, "search")) {
            edict_t* pent = NULL;
            char str[80];
            ClientPrint(pEntity, HUD_PRINTCONSOLE, "searching...\n");
            while((pent = FIND_ENTITY_IN_SPHERE(pent, pEntity->v.origin, 200.0f)) != NULL && (!FNullEnt(pent))) {
                sprintf(str, "Found %s at %5.2f %5.2f %5.2f modelindex- %d t %s tn %s\n", STRING(pent->v.classname),
                    pent->v.origin.x, pent->v.origin.y, pent->v.origin.z, pent->v.modelindex, STRING(pent->v.target),
                    STRING(pent->v.targetname));
                ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

                FILE* fp = UTIL_OpenFoxbotLog();
                if(fp != NULL) {
                    fprintf(fp, "ClientCommmand: search %s\n", str);
                    // fwrite(&pent->v, sizeof(pent->v), 1, fp);
                    fclose(fp);
                }
                UTIL_SavePent(pent);
            }

            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        }
        // DREVIL CVARS
        else if(FStrEq(pcmd, "defensive_chatter")) {
            if(FStrEq(arg1, "on")) {
                defensive_chatter = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is ON\n");
            } else if(FStrEq(arg1, "off")) {
                defensive_chatter = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is OFF\n");
            } else if(FStrEq(arg1, "1")) {
                defensive_chatter = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is ON\n");
            } else if(FStrEq(arg1, "0")) {
                defensive_chatter = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is OFF\n");
            }
            return;
        } else if(FStrEq(pcmd, "offensive_chatter")) {
            if(FStrEq(arg1, "on")) {
                offensive_chatter = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is ON\n");
            } else if(FStrEq(arg1, "off")) {
                offensive_chatter = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is OFF\n");
            } else if(FStrEq(arg1, "1")) {
                offensive_chatter = TRUE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is ON\n");
            } else if(FStrEq(arg1, "0")) {
                offensive_chatter = FALSE;
                ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is OFF\n");
            }
            return;
        }
        /*		else if(FStrEq(pcmd, "frag_commander"))
                        {
                                if(FStrEq(arg1, "on"))
                                {
                                        frag_commander = TRUE;
                                        ClientPrint(pEntity, HUD_PRINTNOTIFY,
                                                "Score-based commander is ON\n");
                                }
                                else if(FStrEq(arg1, "off"))
                                {
                                        frag_commander = FALSE;
                                        ClientPrint(pEntity, HUD_PRINTNOTIFY,
                                                "Score-based commander is OFF\n");
                                }
                                else if(FStrEq(arg1, "1"))
                                {
                                        frag_commander = TRUE;
                                        ClientPrint(pEntity, HUD_PRINTNOTIFY,
                                                "Score-based commander is ON\n");
                                }
                                else if(FStrEq(arg1, "0"))
                                {
                                        frag_commander = FALSE;
                                        ClientPrint(pEntity, HUD_PRINTNOTIFY,
                                                "Score-based commander is OFF\n");
                                }
                                return;
                        }*/
    }

    if(!mr_meta)
        (*other_gFunctionTable.pfnClientCommand)(pEntity);
    else {
        SET_META_RESULT(MRES_HANDLED);
    }
}

void ClientUserInfoChanged(edict_t* pEntity, char* infobuffer)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "ClientUserInfoChanged: pEntity=%p infobuffer=%s\n", (void*)pEntity, infobuffer);
        fclose(fp);
    }

    if(!mr_meta)
        (*other_gFunctionTable.pfnClientUserInfoChanged)(pEntity, infobuffer);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void ServerDeactivate(void)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnServerDeactivate)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

void PlayerPreThink(edict_t* pEntity)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnPlayerPreThink)(pEntity);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void PlayerPostThink(edict_t* pEntity)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnPlayerPostThink)(pEntity);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void StartFrame(void)
{
    // v7 last frame timing
    last_frame_time = last_frame_time_prev;
    last_frame_time_prev = gpGlobals->time;

    // Reset pipeCheck frame counter.
    if(pipeCheckFrame < 0)
        pipeCheckFrame = 20;

    if(gpGlobals->deathmatch) {
        edict_t* pPlayer;
        static float check_server_cmd;
        check_server_cmd = gpGlobals->time;
        static int i, index, player_index, bot_index;
        static float previous_time = -1.0f;
        static float client_update_time = 0.0;
        clientdata_s cd;
        char msg[256];
        int count;

        // if a new map has started then (MUST BE FIRST IN StartFrame)...
        // if((gpGlobals->time + 0.1) < previous_time)
        if(strcmp(STRING(gpGlobals->mapname), prevmapname) != 0) {
            char filename[256];
            char mapname[64];

            first_player = NULL;
            display_bot_vars = TRUE;
            display_start_time = gpGlobals->time + 10;

            need_to_open_cfg = TRUE;

            // check if mapname_bot.cfg file exists...

            if((gpGlobals->time + 0.1) < previous_time) {
                strcpy(mapname, STRING(gpGlobals->mapname));
                strcat(mapname, "_bot.cfg");

                bot_cfg_fp = NULL;
                UTIL_BuildFileName(filename, 255, "configs", mapname);
                bot_cfg_fp = fopen(filename, "r");

                if(bot_cfg_fp == NULL) {
                    UTIL_BuildFileName(filename, 255, "default.cfg", NULL);
                    bot_cfg_fp = fopen(filename, "r");
                }

                if(bot_cfg_fp != NULL) {
                    // sprintf(msg, "Executing %s\n", filename);
                    // ALERT( at_console, msg );

                    for(index = 0; index < 32; index++) {
                        bots[index].is_used = FALSE;
                        bots[index].respawn_state = 0;
                        bots[index].f_kick_time = 0.0;
                    }
                    fclose(bot_cfg_fp);
                    bot_cfg_fp = NULL;
                } else {
                    count = 0;
                    // mark the bots as needing to be respawned...
                    for(index = 0; index < 32; index++) {
                        if(count >= prev_num_bots) {
                            bots[index].is_used = FALSE;
                            bots[index].respawn_state = 0;
                            bots[index].f_kick_time = 0.0;
                        }

                        if(bots[index].is_used) // is this slot used?
                        {
                            bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
                            count++;
                        }

                        // check for any bots that were very recently kicked...
                        if((bots[index].f_kick_time + 5.0) > previous_time) {
                            bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
                            count++;
                        } else
                            bots[index].f_kick_time = 0.0; // reset to prevent false spawns later
                    }

                    // set the respawn time
                    if(IS_DEDICATED_SERVER())
                        respawn_time = gpGlobals->time + 10.0;
                    else
                        respawn_time = gpGlobals->time + 20.0;
                }
            }

            // start updating client data again
            client_update_time = gpGlobals->time + 10.0;

            bot_check_time = gpGlobals->time + 30.0;
        }

        // end of config map check stuff.

        if(!IS_DEDICATED_SERVER()) {
            if((welcome_index != -1) && (welcome_sent == FALSE) && (welcome_time < 1.0)) {
                // are they out of observer mode yet?
                if(clients[welcome_index] != NULL) {
                    // welcome in 5 seconds
                    if(IsAlive(clients[welcome_index]))
                        welcome_time = gpGlobals->time + 5.0;
                }
            }

            if((welcome_time > 0.0) && (welcome_time < gpGlobals->time) && (welcome_sent == FALSE)) {
                char welcome_msg[128] = "--FoxBot--\n\nwww.foxbot.net\nAnd: www.apg-clan.org\n";
                char version[32];

                // sprintf(version," Beta v%d.%d Build#%d\n", VER_MAJOR, VER_MINOR, VER_BUILD);
                sprintf(version, " v%d.%d \n", VER_MAJOR, VER_MINOR);
                strcat(welcome_msg, version);
                // let's send a welcome message to this client...
                // UTIL_SayText(welcome_msg, clients[welcome_index]);

                KewlHUDNotify(clients[welcome_index], welcome_msg);
                welcome_sent = TRUE; // clear this so we only do it once
            }
        }

        if(client_update_time <= gpGlobals->time) {
            // if(debug_engine) { fp = UTIL_OpenFoxbotLog(); fprintf(fp, "clientupdate\n"); fclose(fp); }
            client_update_time = gpGlobals->time + 1.0;

            for(i = 0; i < 32; i++) {
                if(bots[i].is_used) {
                    memset(&cd, 0, sizeof(cd));

                    if(mr_meta)
                        MDLL_UpdateClientData(bots[i].pEdict, 1, &cd);
                    else
                        UpdateClientData(bots[i].pEdict, 1, &cd);

                    // see if a weapon was dropped...
                    if(bots[i].bot_weapons != cd.weapons)
                        bots[i].bot_weapons = cd.weapons;
                }
            }
        }

        count = 0;

        UpdateFlagCarrierList(); // need to do this once per frame

        /*	// useful debugging info
                if(spectate_debug == 0)
                {  // starto!
                        char msg[128];

                        _snprintf(msg, 128, "check_time %f, time %f\nmin_bots %d, max_bots %d",
                                bot_check_time, gpGlobals->time, min_bots, max_bots);

                        msg[127] = '\0';

                        // output the message
                        MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, INDEXENT(1));
                                WRITE_BYTE( TE_TEXTMESSAGE);
                                WRITE_BYTE( 2 & 0xFF );

                                WRITE_SHORT( 0 ); // coordinates X
                                WRITE_SHORT( -8192 ); // coordinates Y

                                WRITE_BYTE(1); //effect

                                WRITE_BYTE(255);
                                WRITE_BYTE(255);
                                WRITE_BYTE(255);
                                WRITE_BYTE(255);

                                WRITE_BYTE(255);
                                WRITE_BYTE(255);
                                WRITE_BYTE(255);
                                WRITE_BYTE(255);
                                WRITE_SHORT( FixedUnsigned16( 0, 1 << 8 ) );  // fade-in time
                                WRITE_SHORT( FixedUnsigned16( 0, 1 << 8 ) );  // fade-out time
                                WRITE_SHORT( FixedUnsigned16( 1, 1 << 8 ) );  // hold time

                                WRITE_STRING(msg);
                        MESSAGE_END();
                } // endo!*/

        for(bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++) {
            // if this bot is active, and the bot is not respawning
            if((bots[bot_index].is_used) && (bots[bot_index].respawn_state == RESPAWN_IDLE)) {
                //	UTIL_BotLogPrintf("running bot %d time:%f\n", bot_index, gpGlobals->time);
                BotThink(&bots[bot_index]);

                count++;

                /*if((mod_id == FRONTLINE_DLL) && (flf_bug_check == 0)) {
                    edict_t* pent = NULL;
                    // int fix_flag = 0;

                    flf_bug_check = 1;

                    while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "capture_point")) != NULL && (!FNullEnt(pent))) {
                        if(pent->v.skin != 0) // not blue skin?
                            flf_bug_fix = 1;  // need to use bug fix code
                    }
                }*/
            }
        }

        if(count > num_bots)
            num_bots = count;

        for(player_index = 1; player_index <= gpGlobals->maxClients; player_index++) {
            pPlayer = INDEXENT(player_index);

            if(pPlayer && !pPlayer->free) {
                if((g_area_def || g_waypoint_on) && FBitSet(pPlayer->v.flags, FL_CLIENT) &&
                    !FBitSet(pPlayer->v.flags, FL_FAKECLIENT)) {
                    WaypointThink(pPlayer);
                }

                /*if((mod_id == FRONTLINE_DLL) && (flf_bug_check == 0)) {
                    edict_t* pent = NULL;
                    // int fix_flag = 0;

                    flf_bug_check = 1;

                    while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "capture_point")) != NULL && (!FNullEnt(pent))) {
                        if(pent->v.skin != 0) // not blue skin?
                            flf_bug_fix = 1;  // need to use bug fix code
                    }
                }*/
            }
        }

        // are we currently respawning bots and is it time to spawn one yet?
        if((respawn_time > 1.0) && (respawn_time <= gpGlobals->time)) {
            int index = 0;

            // find bot needing to be respawned...
            while((index < 32) && (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
                index++;

            if(index < 32) {
                bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
                bots[index].is_used = FALSE; // free up this slot

                // respawn 1 bot then wait a while(otherwise engine crashes)
                if((mod_id == VALVE_DLL)) {
                    char c_skill[2];

                    sprintf(c_skill, "%d", bots[index].bot_skill);

                    BotCreate(NULL, bots[index].skin, bots[index].name, c_skill, NULL);
                } else {
                    char c_skill[2];
                    char c_team[2];
                    char c_class[3];

                    sprintf(c_skill, "%d", bots[index].bot_skill);
                    sprintf(c_team, "%d", bots[index].bot_team);
                    sprintf(c_class, "%d", bots[index].bot_class);

                    if((mod_id == TFC_DLL))
                        BotCreate(NULL, NULL, NULL, bots[index].name, c_skill);
                    else
                        BotCreate(NULL, c_team, c_class, bots[index].name, c_skill);
                }

                respawn_time = gpGlobals->time + 2.0; // set next respawn time

                bot_check_time = gpGlobals->time + 5.0;
            } else
                respawn_time = 0.0;
        }

        if(g_GameRules) {
            if(need_to_open_cfg) // have we opened foxbot.cfg file yet?
            {
                char filename[256];
                char mapname[64];

                need_to_open_cfg = FALSE; // only do this once!!!
                cfg_file = 1;
                display_bot_vars = TRUE;
                display_start_time = gpGlobals->time + 10;

                // check if mapname_bot.cfg file exists...

                strcpy(mapname, STRING(gpGlobals->mapname));
                strcat(mapname, "_bot.cfg");

                bot_cfg_fp = NULL;
                UTIL_BuildFileName(filename, 255, "foxbot.cfg", NULL);
                bot_cfg_fp = fopen(filename, "r");

                if(bot_cfg_fp == NULL) {
                    if(IS_DEDICATED_SERVER())
                        printf("foxbot.cfg file not found\n");
                    else
                        ALERT(at_console, "foxbot.cfg file not found\n");
                } else {
                    sprintf(msg, "Executing %s\n", filename);
                    if(IS_DEDICATED_SERVER())
                        printf(msg);
                    else
                        ALERT(at_console, msg);
                }

                if(IS_DEDICATED_SERVER())
                    bot_cfg_pause_time = gpGlobals->time + 2.0; // was 5
                else
                    bot_cfg_pause_time = gpGlobals->time + 2.0; // was 20
            }

            if(need_to_open_cfg2) // have we opened foxbot.cfg file yet?
            {
                bot_cfg_pause_time = gpGlobals->time + 1.0;
                char filename[256];
                char mapname[64];

                need_to_open_cfg2 = FALSE; // only do this once!!!
                cfg_file = 2;
                // display_bot_vars=TRUE;
                // display_start_time=gpGlobals->time+10;

                // check if mapname_bot.cfg file exists...

                strcpy(mapname, STRING(gpGlobals->mapname));
                strcat(mapname, "_bot.cfg");

                bot_cfg_fp = NULL;
                UTIL_BuildFileName(filename, 255, "configs", mapname);
                bot_cfg_fp = fopen(filename, "r");

                if(bot_cfg_fp != NULL) {
                    sprintf(msg, "\nExecuting %s\n", filename);
                    if(IS_DEDICATED_SERVER())
                        printf(msg);
                    else
                        ALERT(at_console, msg);
                } else {
                    // first say map config not found
                    sprintf(msg, "\n%s not found\n", filename);
                    if(IS_DEDICATED_SERVER())
                        printf(msg);
                    else
                        ALERT(at_console, msg);

                    bot_cfg_fp = NULL;
                    UTIL_BuildFileName(filename, 255, "default.cfg", NULL);
                    bot_cfg_fp = fopen(filename, "r");

                    if(bot_cfg_fp == NULL) {
                        if(IS_DEDICATED_SERVER())
                            printf("\ndefault.cfg file not found\n");
                        else
                            ALERT(at_console, "\ndefault.cfg file not found\n");
                    } else {
                        sprintf(msg, "\nExecuting %s\n", filename);
                        if(IS_DEDICATED_SERVER())
                            printf(msg);
                        else
                            ALERT(at_console, msg);
                    }
                }
            }

            // end need config

            if(!IS_DEDICATED_SERVER() && !spawn_time_reset) {
                if(first_player != NULL) {
                    if(IsAlive(first_player)) {
                        spawn_time_reset = TRUE;

                        if(respawn_time >= 1.0)
                            respawn_time = min(respawn_time, gpGlobals->time + 1.0f);

                        if(bot_cfg_pause_time >= 1.0)
                            bot_cfg_pause_time = min(bot_cfg_pause_time, gpGlobals->time + 1.0f);
                    }
                }
            }
            if( //(bot_cfg_fp) &&
                (bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= gpGlobals->time)) {
                // process bot.cfg file options...
                ProcessBotCfgFile();
                display_start_time = 0;
            } else if(bot_cfg_fp == NULL && display_bot_vars && display_start_time <= gpGlobals->time) {
                DisplayBotInfo();
                display_bot_vars = FALSE;
            }
        }

        // if time to check for server commands then do so...
        if((check_server_cmd <= gpGlobals->time) && IS_DEDICATED_SERVER()) {
            check_server_cmd = gpGlobals->time + 1.0;

            char* cvar_bot = (char*)CVAR_GET_STRING("bot");

            if(cvar_bot && cvar_bot[0]) {
                char cmd_line[80];
                char *cmd, *arg1, *arg2, *arg3, *arg4;

                strcpy(cmd_line, cvar_bot);

                index = 0;
                cmd = cmd_line;
                arg1 = arg2 = arg3 = arg4 = NULL;

                // skip to blank or end of string...
                while((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                    index++;

                if(cmd_line[index] == ' ') {
                    cmd_line[index++] = 0;
                    arg1 = &cmd_line[index];

                    // skip to blank or end of string...
                    while((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                        index++;

                    if(cmd_line[index] == ' ') {
                        cmd_line[index++] = 0;
                        arg2 = &cmd_line[index];

                        // skip to blank or end of string...
                        while((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                            index++;

                        if(cmd_line[index] == ' ') {
                            cmd_line[index++] = 0;
                            arg3 = &cmd_line[index];

                            // skip to blank or end of string...
                            while((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                                index++;

                            if(cmd_line[index] == ' ') {
                                cmd_line[index++] = 0;
                                arg4 = &cmd_line[index];
                            }
                        }
                    }
                }

                if(strcmp(cmd, "addbot") == 0) {
                    BotCreate(NULL, arg1, arg2, arg3, arg4);
                    bot_check_time = gpGlobals->time + 5.0;
                } else if(strcmp(cmd, "min_bots") == 0) {
                    changeBotSetting("min_bots", &min_bots, arg1, -1, 31, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "max_bots") == 0) {
                    changeBotSetting("max_bots", &max_bots, arg1, -1, MAX_BOTS, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "bot_total_varies") == 0) {
                    changeBotSetting("bot_total_varies", &bot_total_varies, arg1, 0, 3, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "bot_info") == 0) {
                    DisplayBotInfo();
                } else if(strcmp(cmd, "bot_team_balance") == 0) {
                    if((arg1 != NULL)) {
                        if((*arg1 != 0)) {
                            int temp = atoi(arg1);
                            if(temp)
                                bot_team_balance = TRUE;
                            else
                                bot_team_balance = FALSE;
                        }
                    }

                    if(bot_team_balance)
                        printf("bot_team_balance (1) On\n");
                    else
                        printf("bot_team_balance (0) Off\n");
                } else if(strcmp(cmd, "bot_bot_balance") == 0) {
                    if((arg1 != NULL)) {
                        if((*arg1 != 0)) {
                            int temp = atoi(arg1);
                            if(temp)
                                bot_bot_balance = TRUE;
                            else
                                bot_bot_balance = FALSE;
                        }
                    }

                    if(bot_bot_balance)
                        printf("bot_bot_balance (1) On\n");
                    else
                        printf("bot_bot_balance (0) Off\n");
                } else if(strcmp(cmd, "kickall") == 0) {
                    // kick all bots off of the server after time/frag limit...
                    kickBots(MAX_BOTS, -1);
                } else if((strcmp(cmd, "kickteam") == 0) || (strcmp(cmd, "foxbot_kickteam") == 0)) {
                    if(!arg1)
                        printf("Proper syntax, e.g. bot \"kickteam 1\"\n");
                    else {
                        int whichTeam;
                        if((strcmp(arg1, "blue") == 0))
                            whichTeam = 1;
                        else if((strcmp(arg1, "red") == 0))
                            whichTeam = 2;
                        else if((strcmp(arg1, "yellow") == 0))
                            whichTeam = 3;
                        else if((strcmp(arg1, "green") == 0))
                            whichTeam = 4;
                        else
                            whichTeam = atoi(arg1);

                        kickBots(MAX_BOTS, whichTeam);
                    }
                } else if(strcmp(cmd, "bot_chat") == 0 && arg1 != NULL) {
                    changeBotSetting("bot_chat", &botchat, arg1, 0, 1000, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "botskill_lower") == 0) {
                    changeBotSetting("botskill_lower", &botskill_lower, arg1, 1, 5, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "botskill_upper") == 0) {
                    changeBotSetting("botskill_upper", &botskill_upper, arg1, 1, 5, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "bot_skill_1_aim") == 0) {
                    changeBotSetting("bot_skill_1_aim", &bot_skill_1_aim, arg1, 0, 200, SETTING_SOURCE_SERVER_COMMAND);

                    BotUpdateSkillInaccuracy();
                    return;
                } else if(strcmp(cmd, "bot_aim_per_skill") == 0) {
                    changeBotSetting(
                        "bot_aim_per_skill", &bot_aim_per_skill, arg1, 5, 50, SETTING_SOURCE_SERVER_COMMAND);

                    BotUpdateSkillInaccuracy();
                    return;
                }

                else if(strcmp(cmd, "bot_can_build_teleporter") == 0 && arg1 != NULL) {
                    if(strcmp(arg1, "on") == 0) {
                        bot_can_build_teleporter = TRUE;
                        printf("bot_can_build_teleporter is ON\n");
                    } else if(strcmp(arg1, "off") == 0) {
                        bot_can_build_teleporter = FALSE;
                        printf("bot_can_build_teleporter is OFF\n");
                    }
                }

                else if(strcmp(cmd, "bot_can_use_teleporter") == 0 && arg1 != NULL) {
                    if(strcmp(arg1, "on") == 0) {
                        bot_can_use_teleporter = TRUE;
                        printf("bot_can_use_teleporter is ON\n");
                    } else if(strcmp(arg1, "off") == 0) {
                        bot_can_use_teleporter = FALSE;
                        printf("bot_can_use_teleporter is OFF\n");
                    }
                }

                else if(strcmp(cmd, "bot_xmas") == 0 && arg1 != NULL) {
                    if(strcmp(arg1, "on") == 0) {
                        bot_xmas = TRUE;

                        printf("bot xmas is ON\n");
                    } else if(strcmp(arg1, "off") == 0) {
                        bot_xmas = FALSE;

                        printf("bot xmas is OFF\n");
                    }
                } else if(strcmp(cmd, "bot_allow_moods") == 0) {
                    changeBotSetting("bot_allow_moods", &bot_allow_moods, arg1, 0, 1, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "bot_allow_humour") == 0) {
                    changeBotSetting("bot_allow_humour", &bot_allow_humour, arg1, 0, 1, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "bot_use_grenades") == 0) {
                    changeBotSetting("bot_use_grenades", &bot_use_grenades, arg1, 0, 2, SETTING_SOURCE_SERVER_COMMAND);
                } else if(strcmp(cmd, "dump") == 0) {
                    edict_t* pent = NULL;
                    while((pent = FIND_ENTITY_IN_SPHERE(pent, Vector(0, 0, 0), 8192)) != NULL && (!FNullEnt(pent))) {
                        UTIL_SavePent(pent);
                    }
                }
                // dedicated server input
            }
            // moved this line down one
            CVAR_SET_STRING("bot", "");
        }

        // { FILE *ufp=UTIL_OpenFoxbotLog(); fprintf(ufp,"time %f | check %f\n", gpGlobals->time, bot_check_time);
        // fclose(ufp); }

        // check if time to see if a bot needs to be created...
        if(bot_check_time < gpGlobals->time) {
            bot_check_time = gpGlobals->time + bot_create_interval;

            // min/max checking and team balance checking..
            playersPerTeam[0] = 0;
            playersPerTeam[1] = 0;
            playersPerTeam[2] = 0;
            playersPerTeam[3] = 0;
            if(max_bots > MAX_BOTS)
                max_bots = MAX_BOTS;
            if(max_bots > gpGlobals->maxClients)
                max_bots = gpGlobals->maxClients;
            if(max_bots < -1)
                max_bots = -1;
            if(min_bots > max_bots)
                min_bots = max_bots;
            if(min_bots < -1)
                min_bots = -1;
            if(min_bots == 0)
                min_bots = -1;

            // count the number of players, and players per team
            int count = 0;
            {
                char cl_name[128];

                for(i = 1; i <= gpGlobals->maxClients; i++) {
                    if(INDEXENT(i) == NULL)
                        continue;

                    cl_name[0] = '\0';
                    char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(INDEXENT(i));
                    strcpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));

                    // is this a valid connected player?
                    if(cl_name[0] != '\0') {
                        count++;
                        int team = (INDEXENT(i)->v.team) - 1;
                        if(team > -1 && team < 4)
                            ++playersPerTeam[team];
                    }
                }
            }

            // sort out is_team, just in case the map ents are fucked
            bool active_team_found = FALSE;
            for(i = 0; i < 4; i++) // look for an active team
            {
                if(is_team[i])
                    active_team_found = TRUE;
            }
            if(!active_team_found) // assume there are at least two teams
            {
                is_team[0] = TRUE;
                is_team[1] = TRUE;
                is_team[2] = FALSE;
                is_team[3] = FALSE;
            }
            if(is_team[2] == FALSE && is_team[3] == TRUE)
                is_team[3] = FALSE;

            // check if the teams need balancing
            TeamBalanceCheck();

            // random simulation of clients joining/leaving
            if(bot_total_varies)
                varyBotTotal();

            // if there are currently less than the maximum number of players
            // then add another bot using the default skill level...
            if((count < interested_bots || bot_total_varies == 0) && count < max_bots && max_bots != -1) {
                //	UTIL_BotLogPrintf("Bot created, time:%f, bot_check_time:%f, bot_create_interval:%f\n",
                //		gpGlobals->time, bot_check_time, bot_create_interval);

                BotCreate(NULL, NULL, NULL, NULL, NULL);
            }

            // do bot max_bot kick here...
            // if there are currently more than the minimum number of bots running
            // then kick one of the bots off the server...
            if(((count > interested_bots && bot_total_varies) || count > max_bots) && max_bots != -1) {
                // count the number of bots present
                int bot_count = 0;
                for(i = 0; i <= 31; i++) {
                    if(bots[i].is_used)
                        ++bot_count;
                }

                if(bot_count > min_bots) {
                    if(bot_total_varies)
                        kickRandomBot();
                    else
                        kickBots(1, -1);
                }
            }
        } else {
            // make sure the delay is not insanely long
            if(bot_check_time > gpGlobals->time + 100.0)
                bot_check_time = gpGlobals->time + 10.0;
        }

        previous_time = gpGlobals->time;
    }

    // this is where the behaviour config is interpreted..
    // i.e. new lev, load behaviour, and parse it

    if(strcmp(STRING(gpGlobals->mapname), prevmapname) != 0) {
        edict_t* pent = NULL;
        // while((pent = FIND_ENTITY_BY_STRING( pent, "classname","trigger_multiple" )) != NULL
        // && (!FNullEnt(pent)))
        while((pent = FIND_ENTITY_IN_SPHERE(pent, Vector(0, 0, 0), 8000)) != NULL && (!FNullEnt(pent))) {
            if(pent->v.absmin.x == -1 && pent->v.absmin.y == -1 && pent->v.absmin.z == -1) {
                if(pent->v.absmax.x == 1 && pent->v.absmax.y == 1 && pent->v.absmax.z == 1) {
                    fp = UTIL_OpenFoxbotLog();
                    if(fp != NULL) {
                        fprintf(
                            fp, "Fixing entity current map %s last map %s\n", STRING(gpGlobals->mapname), prevmapname);
                        fclose(fp);
                    }

                    // UTIL_SavePent(pent);
                    if(strcmp(STRING(pent->v.classname), "info_tfgoal") != 0) {
                        pent->v.absmin = pent->v.absmin + pent->v.mins;
                        pent->v.absmax = pent->v.absmax + pent->v.maxs;
                        if(mr_meta)
                            MDLL_Spawn(pent);
                        else
                            DispatchSpawn(pent);
                    } else // is info goal
                    {
                        int max = 0;
                        int t = (int)pent->v.mins.x;
                        if(t < 0)
                            t = -t;
                        if(t > max)
                            max = t;
                        t = (int)pent->v.mins.y;
                        if(t < 0)
                            t = -t;
                        if(t > max)
                            max = t;
                        t = (int)pent->v.mins.z;
                        if(t < 0)
                            t = -t;
                        if(t > max)
                            max = t;
                        t = (int)pent->v.maxs.x;
                        if(t < 0)
                            t = -t;
                        if(t > max)
                            max = t;
                        t = (int)pent->v.maxs.y;
                        if(t < 0)
                            t = -t;
                        if(t > max)
                            max = t;
                        t = (int)pent->v.maxs.z;
                        if(t < 0)
                            t = -t;
                        if(t > max)
                            max = t;
                        pent->v.absmin = pent->v.absmin - Vector(max, max, max);
                        pent->v.absmax = pent->v.absmax + Vector(max, max, max);
                        pent->v.nextthink = 0;
                        if(mr_meta)
                            MDLL_Spawn(pent);
                        else
                            DispatchSpawn(pent);
                        // UTIL_SavePent(pent);
                    }
                }
            }
            // UTIL_SavePent(pent);
            if(strcmp(STRING(pent->v.classname), "func_door") == 0 && pent->v.nextthink != -1)
                pent->v.nextthink = -1;
        }
        script_loaded = FALSE;
        script_parsed = FALSE;
        strcpy(prevmapname, STRING(gpGlobals->mapname));
        char filename[256];
        char mapname[64];

        int i;
        // reset known team data on map change
        for(i = 0; i < 4; i++) {
            attack[i] = FALSE;
            defend[i] = FALSE;
        }
        for(i = 0; i < 8; i++) {
            blue_av[i] = FALSE;
            red_av[i] = FALSE;
            green_av[i] = FALSE;
            yellow_av[i] = FALSE;
        }
        ResetBotHomeInfo();

        // clear up all the model icons here!
        // don't remove them as the engine does this its self on a map change
        // just clear the pointers..doh!
        /*for(i=0; i<128; i++)
           {
           if(!mdlb[i])
           if(mdlp[i]!=NULL)
           {
           //REMOVE_ENTITY(mdlp[i]);
           mdlp[i]=NULL;
           mdlb[i]=true;
           }
           }*/
        // ok, do other level data clearing here!

        struct msg_com_struct* prev = NULL;
        struct msg_com_struct* curr = NULL;
        for(i = 0; i < MSG_MAX; i++) {
            // assuming i only goes to 64 on next line..see msg_msg[64][msg_max]
            // was [0][i] before...may be a problem
            msg_msg[0][i] = '\0'; // clear the messages, for level changes

            msg_com[i].ifs[0] = '\0';
            // the idea behind this delete function is if the root.next isnt null,
            // then it finds the last item in list (the one with item.next =null)
            // and deletes it.. then repeats it all again.. if root.next etc
            while(msg_com[i].next != NULL) {
                curr = &msg_com[i];
                // UTIL_BotLogPrintf("StartFrame-del %d\n", curr->next);

                while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1) {
                    prev = curr;
                    curr = curr->next;
                }

                if(prev != NULL) {
                    delete prev->next;
                    prev->next = NULL; // make sure it doesnt go over
                }
            }
            msg_com[i].next = NULL; // make sure it dont crash..gr
            // UTIL_BotLogPrintf("StartFrame-del %d\n",msg_com[i].next);
        }
        // UTIL_BotLogPrintf("StartFrame-made it past clear data\n");

        // check if mapname_bot.cfg file exists...

        strcpy(mapname, STRING(gpGlobals->mapname));
        strcat(mapname, "_fb.cfg");

        UTIL_BuildFileName(filename, 255, "scripts", mapname);
        FILE* bfp = fopen(filename, "r");

        if((bfp != NULL) && mod_id == TFC_DLL) {
            script_loaded = TRUE;
            char msg[255];
            sprintf(msg, "\nExecuting FoXBot TFC script file:%s\n\n", filename);
            ALERT(at_console, msg);

            int ch = fgetc(bfp);
            int i;
            char buffer[14097];
            for(i = 0; (i < 14096) && (feof(bfp) == 0); i++) {
                buffer[i] = (char)ch;
                if(buffer[i] == '\t')
                    buffer[i] = ' ';
                ch = fgetc(bfp);
            }
            buffer[i] = '\0';

            // weve read it into buffer, now we need to check the syntax..
            // ooo lexical analysis, hehe
            int braces = 0;           // check for an even number of braces i.e. {..}
            bool commentline = FALSE; // used to ignore comment lines
            int start = 0;            // used to check if were in a start section or not
            int havestart = 0;
            int msgsection = 0; // used to check if were in a message section section or not
            int ifsec = 0;      // used to check if were in an if statement
            char* buf = buffer;
            // ALERT( at_console, buf);
            // ALERT( at_console, "\n");
            bool random_shit_error = FALSE;
            for(i = 0; (i < 14096) && (buffer[i] != '\0') && (!random_shit_error); i++) {
                // first off... we need to ignore comment lines!
                if(strncmp(buf, "//", 2) == 0) {
                    commentline = TRUE;
                    i++;
                    buf = _strinc(buf);
                }
                if(buffer[i] == '\n')
                    commentline = FALSE;
                if(commentline == false) {
                    // need to check for section definition (on_...)
                    if(strncmp(buf, "on_start", 8) == 0) {
                        if(start > 0)
                            start = 99; // check for nested start defs
                        else
                            start = 1;
                        if(havestart > 0)
                            havestart = 99; // check for more than one on_start section
                        else
                            havestart = 1;
                        for(int j = 0; j < 8; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // try and move to end of on start
                    }
                    if(strncmp(buf, "on_msg", 6) == 0) {
                        if(msgsection > 0)
                            msgsection = 99; // check for nested msg defs
                        else
                            msgsection = 1;
                        for(int j = 0; j < 6; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // try and move to end of on_msg
                        // now check input var!
                        if(buffer[i] != '(')
                            msgsection = 99;
                        // make sure it starts wif a (
                        else {
                            i++;
                            buf = _strinc(buf);
                            if(buffer[i] == ')')
                                msgsection = 99;
                            // make sure message isnt empty
                            else {
                                // if it isn't empty, move to end (ignore msg)
                                while(buffer[i] != ')') {
                                    i++;
                                    buf = _strinc(buf);
                                }
                            }
                        }
                    }
                    // attack
                    else if(strncmp(buf, "blue_attack", 11) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 11; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[0] = 90;
                        } // move to end
                    } else if(strncmp(buf, "red_attack", 10) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 10; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[1] = 90;
                        } // move to end
                    } else if(strncmp(buf, "yellow_attack", 13) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 13; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[3] = 90;
                        } // move to end
                    } else if(strncmp(buf, "green_attack", 12) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 12; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[3] = 90;
                        } // move to end
                    }

                    // defend
                    else if(strncmp(buf, "blue_defend", 11) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 11; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[0] = 15;
                        } // move to end
                    } else if(strncmp(buf, "red_defend", 10) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 10; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[1] = 15;
                        } // move to end
                    } else if(strncmp(buf, "yellow_defend", 13) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 13; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[2] = 15;
                        } // move to end
                    } else if(strncmp(buf, "green_defend", 12) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 12; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[3] = 15;
                        } // move to end
                    } else if(strncmp(buf, "blue_normal", 11) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 11; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[0] = 50;
                        } // move to end
                    } else if(strncmp(buf, "red_normal", 10) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 10; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[1] = 50;
                        } // move to end
                    } else if(strncmp(buf, "yellow_normal", 13) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 13; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[2] = 50;
                        } // move to end
                    } else if(strncmp(buf, "green_normal", 12) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 12; j++) {
                            i++;
                            buf = _strinc(buf);
                            RoleStatus[3] = 50;
                        } // move to end
                    }

                    // point<n> availabe_only (exclusive)
                    else if(strncmp(buf, "blue_available_only_point", 25) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 25; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "red_available_only_point", 24) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 24; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "green_available_only_point", 26) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 26; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "yellow_available_only_point", 27) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 27; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    }

                    // point<n> availabe (set to true)
                    else if(strncmp(buf, "blue_available_point", 20) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 20; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "red_available_point", 19) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 19; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "green_available_point", 21) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 21; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "yellow_available_point", 22) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 22; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    }

                    // point<n> not_availabe (set to false)
                    else if(strncmp(buf, "blue_notavailable_point", 23) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 23; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "red_notavailable_point", 22) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 22; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "green_notavailable_point", 24) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;
                        for(int j = 0; j < 24; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "yellow_notavailable_point", 25) == 0) {
                        // this can only be in a section
                        if(start == 0 && msgsection == 0)
                            random_shit_error = TRUE;

                        for(int j = 0; j < 25; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    }

                    // is point<n> availabe?
                    else if(strncmp(buf, "if_blue_point", 13) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 13; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "if_red_point", 12) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 12; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "if_green_point", 14) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 14; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "if_yellow_point", 15) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 15; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    }

                    // is point<n> NOT availabe?
                    else if(strncmp(buf, "ifn_blue_point", 14) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 14; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "ifn_red_point", 13) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 13; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "ifn_green_point", 15) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 15; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    } else if(strncmp(buf, "ifn_yellow_point", 16) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        for(int j = 0; j < 16; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                            buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                            random_shit_error = TRUE;
                        else {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                    }

                    //
                    // multipoint ifs
                    //
                    else if(strncmp(buf, "if_blue_mpoint", 14) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        int j;
                        for(j = 0; j < 14; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        // check for 8 if vals
                        for(j = 0; j < 8; j++) {
                            if(buffer[i] != '1' && buffer[i] != '0')
                                random_shit_error = TRUE;
                            else {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        }
                    } else if(strncmp(buf, "if_red_mpoint", 13) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        int j;
                        for(j = 0; j < 13; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        // check for 8 if vals
                        for(j = 0; j < 8; j++) {
                            if(buffer[i] != '1' && buffer[i] != '0')
                                random_shit_error = TRUE;
                            else {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        }
                    } else if(strncmp(buf, "if_green_mpoint", 15) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        int j;
                        for(j = 0; j < 15; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        // check for 8 if vals
                        for(j = 0; j < 8; j++) {
                            if(buffer[i] != '1' && buffer[i] != '0')
                                random_shit_error = TRUE;
                            else {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        }
                    } else if(strncmp(buf, "if_yellow_mpoint", 16) == 0) {
                        // this can only be in a section
                        if(msgsection == 0)
                            random_shit_error = TRUE;
                        if(ifsec > 0)
                            ifsec = 99; // check for nested if defs
                        else
                            ifsec = 1;
                        int j;
                        for(j = 0; j < 16; j++) {
                            i++;
                            buf = _strinc(buf);
                        } // move to end
                        // check for 8 if vals
                        for(j = 0; j < 8; j++) {
                            if(buffer[i] != '1' && buffer[i] != '0')
                                random_shit_error = TRUE;
                            else {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        }
                    }
                    // end of multipoint ifs

                    // ignore endlines
                    // and comments
                    else if((buffer[i] != '/' && buffer[i] != '{' && buffer[i] != '}' && buffer[i] != ' ' &&
                                buffer[i] != '\n') &&
                        commentline == false && random_shit_error == false) {
                        random_shit_error = TRUE;
                        ALERT(at_console, "\\/\\/\\/\\/\\/\\/\n");
                        ALERT(at_console, buf);
                        ALERT(at_console, "\n");
                    }

                    // do your magic lexical analysis here..
                    // first braces
                    switch(buffer[i]) {
                    case '{':
                        braces++;
                        if(start == 1)
                            start = 2;
                        else if(start > 0)
                            start = 99;
                        // need to check that more braces aernt put in than nesesarry
                        // i.e. start=2 then generate error,
                        // cus only have variable setting in start section
                        if(ifsec == 1)
                            ifsec = 2;
                        else if(ifsec > 0)
                            ifsec = 99;
                        if(msgsection == 1)
                            msgsection = 2;
                        else if(msgsection > 0 && ifsec == 0)
                            msgsection = 99;
                        break;
                    case '}':
                        braces--;
                        if(start == 2)
                            start = 0;
                        if(msgsection == 2 && ifsec == 0)
                            msgsection = 0;
                        if(ifsec == 2)
                            ifsec = 0; // cancel ifs before ending message section
                        break;
                    };
                }
                if(!random_shit_error)
                    buf = _strinc(buf); // like i++ but for stringcmp stuff
            }
            bool syntax_error;
            syntax_error = FALSE;
            // syntax errors
            if(braces != 0) {
                ALERT(at_console, "Syntax error, wrong number of braces\n");
                syntax_error = TRUE;
            }
            if(start != 0) {
                ALERT(at_console, "Syntax error, on_start error\n");
                syntax_error = TRUE;
            }
            if(msgsection != 0) {
                ALERT(at_console, "Syntax error, on_msg error\n");
                syntax_error = TRUE;
            }
            if(ifsec != 0) {
                ALERT(at_console, "Syntax error, if error\n");
                syntax_error = TRUE;
            }
            if(havestart > 1) {
                ALERT(at_console, "Syntax error, more than 1 on_start\n");
                syntax_error = TRUE;
            }
            if(random_shit_error) {
                ALERT(at_console, "Syntax error, unrecognised command\n");
                syntax_error = TRUE;

                FILE* fp = UTIL_OpenFoxbotLog();
                if(fp != NULL) {
                    fprintf(fp, "Syntax error, unrecognised command\n%s\n", buf);
                    fclose(fp);
                }
            }

            // warnings
            if(havestart == 0)
                ALERT(at_console, "Warning, no on_start section\n");
            // after all the lexical stuff, if we dont have any errors
            // pass the text into the command data types for use
            if(syntax_error == false) {
                script_parsed = TRUE;
                // pass everything to data array
                //	bool pass_debug = TRUE;
                ALERT(at_console, "Passing data to behaviour arrays\n\n");
                braces = 0;          // check for an even number of braces i.e. {..}
                commentline = FALSE; // used to ignore comment lines
                start = 0;           // used to check if were in a start section or not
                havestart = 0;
                msgsection = 0; // used to check if were in a message section section or not
                ifsec = 0;      // used to check if were in an if statement
                buf = buffer;
                random_shit_error = FALSE;
                int pnt;
                char msgtext[64];
                int cnt;
                int current_msg;
                current_msg = -1;
                for(i = 0; (i < 14096) && (buffer[i] != '\0'); i++) {
                    // first off.. need to ignore comment lines!
                    if(strncmp(buf, "//", 2) == 0) {
                        commentline = TRUE;
                        i++;
                        buf = _strinc(buf);
                    }
                    if(buffer[i] == '\n')
                        commentline = FALSE;
                    if(commentline == false) {
                        // need to check for section definition (on_...)
                        if(strncmp(buf, "on_start", 8) == 0) {
                            if(start > 0)
                                start = 99; // check for nested start defs
                            else
                                start = 1;
                            if(havestart > 0)
                                havestart = 99; // check for more than one on_start section
                            else
                                havestart = 1;
                            for(int j = 0; j < 8; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // try and move to end of on start
                        }
                        if(strncmp(buf, "on_msg", 6) == 0) {
                            current_msg++;
                            if(msgsection > 0)
                                msgsection = 99; // check for nested msg defs
                            else
                                msgsection = 1;
                            for(int j = 0; j < 6; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // try and move to end of on_msg
                            // now check input var!
                            if(buffer[i] != '(')
                                msgsection = 99;
                            // make sure it starts wif a (
                            else {
                                i++;
                                buf = _strinc(buf);
                                if(buffer[i] == ')')
                                    msgsection = 99;
                                // make sure message isnt empty
                                else {
                                    cnt = 0;
                                    // if it isn't empty, move to end (ignore msg)
                                    while(buffer[i] != ')') {
                                        msgtext[cnt] = buffer[i];
                                        cnt++;
                                        i++;
                                        buf = _strinc(buf);
                                    }
                                    msgtext[cnt] = '\0'; // terminate string
                                    strcpy(msg_msg[current_msg], msgtext);
                                    // ALERT( at_console, msg_msg[current_msg]);
                                    // now we have the message, we should probably clear out
                                    // all the available data
                                    for(int i = 0; i < 8; i++) {
                                        msg_com[current_msg].blue_av[i] = -1;
                                        msg_com[current_msg].red_av[i] = -1;
                                        msg_com[current_msg].yellow_av[i] = -1;
                                        msg_com[current_msg].green_av[i] = -1;
                                    }
                                    // clear the next pointer to stop it craching out.
                                    msg_com[current_msg].next = NULL;
                                    curr = &msg_com[current_msg];
                                }
                            }
                        }
                        // attack
                        else if(strncmp(buf, "blue_attack", 11) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 11; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            attack[0] = TRUE;
                        } else if(strncmp(buf, "red_attack", 10) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 10; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            attack[1] = TRUE;
                        } else if(strncmp(buf, "green_attack", 12) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 12; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            attack[2] = TRUE;
                        } else if(strncmp(buf, "yellow_attack", 13) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 13; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            attack[3] = TRUE;
                        }

                        // defend
                        else if(strncmp(buf, "blue_defend", 11) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 11; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            defend[0] = TRUE;
                        } else if(strncmp(buf, "red_defend", 10) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 10; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            defend[1] = TRUE;
                        } else if(strncmp(buf, "green_defend", 12) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 12; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            defend[2] = TRUE;
                        } else if(strncmp(buf, "yellow_defend", 13) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 13; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            defend[3] = TRUE;
                        }
                        // normal (defend+attack)??
                        // not used yet..

                        else if(strncmp(buf, "blue_normal", 11) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 11; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        } else if(strncmp(buf, "red_normal", 10) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 10; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        } else if(strncmp(buf, "green_normal", 12) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 12; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        } else if(strncmp(buf, "yellow_normal", 13) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 13; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                        }

                        // point<n> availabe_only (exclusive)
                        else if(strncmp(buf, "blue_available_only_point", 25) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 25; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                for(int j = 0; j < 8; j++) {
                                    blue_av[j] = FALSE;
                                }
                                blue_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                for(int j = 0; j < 8; j++) {
                                    msg_com[current_msg].blue_av[j] = 0; // false
                                }
                                msg_com[current_msg].blue_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                for(int j = 0; j < 8; j++) {
                                    curr->blue_av[j] = 0; // false
                                }
                                curr->blue_av[pnt] = 1; // true
                            }
                        } else if(strncmp(buf, "red_available_only_point", 24) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 24; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                for(int j = 0; j < 8; j++) {
                                    red_av[j] = FALSE;
                                }
                                red_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                for(int j = 0; j < 8; j++) {
                                    msg_com[current_msg].red_av[j] = 0; // false
                                }
                                msg_com[current_msg].red_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                for(int j = 0; j < 8; j++) {
                                    curr->red_av[j] = 0; // false
                                }
                                curr->red_av[pnt] = 1; // true
                            }
                        } else if(strncmp(buf, "green_available_only_point", 26) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 26; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                for(int j = 0; j < 8; j++) {
                                    green_av[j] = FALSE;
                                }
                                green_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                for(int j = 0; j < 8; j++) {
                                    msg_com[current_msg].green_av[j] = 0; // false
                                }
                                msg_com[current_msg].green_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                for(int j = 0; j < 8; j++) {
                                    curr->green_av[j] = 0; // false
                                }
                                curr->green_av[pnt] = 1; // true
                            }
                        } else if(strncmp(buf, "yellow_available_only_point", 27) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 27; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                for(int j = 0; j < 8; j++) {
                                    yellow_av[j] = FALSE;
                                }
                                yellow_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                for(int j = 0; j < 8; j++) {
                                    msg_com[current_msg].yellow_av[j] = 0; // false
                                }
                                msg_com[current_msg].yellow_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                for(int j = 0; j < 8; j++) {
                                    curr->yellow_av[j] = 0; // false
                                }
                                curr->yellow_av[pnt] = 1; // true
                            }
                        }

                        // point<n> availabe (set to true)
                        else if(strncmp(buf, "blue_available_point", 20) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 20; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                blue_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].blue_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->blue_av[pnt] = 1; // true
                            }
                        } else if(strncmp(buf, "red_available_point", 19) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 19; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                red_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].red_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->red_av[pnt] = 1; // true
                            }
                        } else if(strncmp(buf, "green_available_point", 21) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 21; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                green_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].green_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->green_av[pnt] = 1; // true
                            }
                        } else if(strncmp(buf, "yellow_available_point", 22) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 22; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                yellow_av[pnt] = TRUE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].yellow_av[pnt] = 1; // true
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->yellow_av[pnt] = 1; // true
                            }
                        }

                        // point<n> not_availabe (set to false)
                        else if(strncmp(buf, "blue_notavailable_point", 23) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 23; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                blue_av[pnt] = FALSE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].blue_av[pnt] = 0; // false
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->blue_av[pnt] = 0; // false
                            }
                        } else if(strncmp(buf, "red_notavailable_point", 22) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 22; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                red_av[pnt] = FALSE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].red_av[pnt] = 0; // false
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->red_av[pnt] = 0; // false
                            }
                        } else if(strncmp(buf, "green_notavailable_point", 24) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 24; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                green_av[pnt] = FALSE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].green_av[pnt] = 0; // false
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->green_av[pnt] = 0; // false
                            }
                        } else if(strncmp(buf, "yellow_notavailable_point", 25) == 0) {
                            // this can only be in a section
                            if(start == 0 && msgsection == 0)
                                random_shit_error = TRUE;
                            for(int j = 0; j < 25; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                pnt--;
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(start == 2) {
                                yellow_av[pnt] = FALSE;
                            }
                            if(msgsection == 2 && ifsec == 0) {
                                msg_com[current_msg].yellow_av[pnt] = 0; // false
                            }
                            if(msgsection == 2 && ifsec == 2) {
                                curr->yellow_av[pnt] = 0; // false
                            }
                        }

                        // is point<n> availabe?
                        else if(strncmp(buf, "if_blue_point", 13) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 13; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "b_p_%d", pnt);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "if_red_point", 12) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 12; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "r_p_%d", pnt);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "if_green_point", 14) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 14; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "g_p_%d", pnt);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "if_yellow_point", 15) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 15; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "y_p_%d", pnt);
                            strcpy(curr->ifs, msg);
                        }

                        // is point<n> NOT availabe?
                        else if(strncmp(buf, "ifn_blue_point", 14) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 14; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "b_pn_%d", pnt);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "ifn_red_point", 13) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 13; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "r_pn_%d", pnt);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "ifn_green_point", 15) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 15; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "g_pn_%d", pnt);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "ifn_yellow_point", 16) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            for(int j = 0; j < 16; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            if(buffer[i] != '1' && buffer[i] != '2' && buffer[i] != '3' && buffer[i] != '4' &&
                                buffer[i] != '5' && buffer[i] != '6' && buffer[i] != '7' && buffer[i] != '8')
                                random_shit_error = TRUE;
                            else {
                                pnt = atoi(&buffer[i]); // get the var
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            sprintf(msg, "y_pn_%d", pnt);
                            strcpy(curr->ifs, msg);
                        }

                        //
                        // multipoint ifs
                        //
                        else if(strncmp(buf, "if_blue_mpoint", 14) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            int j;
                            for(j = 0; j < 14; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            // check for 8 if vals
                            char pnts[9];
                            for(j = 0; j < 8; j++) {
                                if(buffer[i] != '1' && buffer[i] != '0')
                                    random_shit_error = TRUE;
                                else {
                                    pnts[j] = buffer[i];
                                    i++;
                                    buf = _strinc(buf);
                                } // move to end
                            }
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            pnts[8] = '\0';
                            sprintf(msg, "b_mp_%s", pnts);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "if_red_mpoint", 13) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            int j;
                            for(j = 0; j < 13; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            // check for 8 if vals
                            char pnts[9];
                            for(j = 0; j < 8; j++) {
                                if(buffer[i] != '1' && buffer[i] != '0')
                                    random_shit_error = TRUE;
                                else {
                                    pnts[j] = buffer[i];
                                    i++;
                                    buf = _strinc(buf);
                                } // move to end
                            }
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            pnts[8] = '\0';
                            sprintf(msg, "r_mp_%s", pnts);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "if_green_mpoint", 15) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            int j;
                            for(j = 0; j < 15; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            // check for 8 if vals
                            char pnts[9];
                            for(j = 0; j < 8; j++) {
                                if(buffer[i] != '1' && buffer[i] != '0')
                                    random_shit_error = TRUE;
                                else {
                                    pnts[j] = buffer[i];
                                    i++;
                                    buf = _strinc(buf);
                                } // move to end
                            }
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            pnts[8] = '\0';
                            sprintf(msg, "g_mp_%s", pnts);
                            strcpy(curr->ifs, msg);
                        } else if(strncmp(buf, "if_yellow_mpoint", 16) == 0) {
                            // this can only be in a section
                            if(msgsection == 0)
                                random_shit_error = TRUE;
                            if(ifsec > 0)
                                ifsec = 99; // check for nested if defs
                            else
                                ifsec = 1;
                            int j;
                            for(j = 0; j < 16; j++) {
                                i++;
                                buf = _strinc(buf);
                            } // move to end
                            // check for 8 if vals
                            char pnts[9];
                            for(j = 0; j < 8; j++) {
                                if(buffer[i] != '1' && buffer[i] != '0')
                                    random_shit_error = TRUE;
                                else {
                                    pnts[j] = buffer[i];
                                    i++;
                                    buf = _strinc(buf);
                                } // move to end
                            }
                            while(curr->next != NULL && curr->next != 0 && (int)curr->next != -1)
                                curr = curr->next; // get to null

                            curr->next = new msg_com_struct;
                            curr = curr->next;
                            // clear next pointer..
                            curr->next = 0;
                            for(int i = 0; i < 8; i++) {
                                curr->blue_av[i] = -1;
                                curr->red_av[i] = -1;
                                curr->yellow_av[i] = -1;
                                curr->green_av[i] = -1;
                            }
                            pnts[8] = '\0';
                            sprintf(msg, "y_mp_%s", pnts);
                            strcpy(curr->ifs, msg);
                        }
                        // end of multipoint ifs

                        // ignore endlines
                        // and comments
                        else if((buffer[i] != '/' && buffer[i] != '{' && buffer[i] != '}' && buffer[i] != ' ' &&
                                    buffer[i] != '\n') &&
                            commentline == false && random_shit_error == false) {
                            random_shit_error = TRUE;
                            ALERT(at_console, "\\/\\/\\/\\/\\/\\/\n");
                            ALERT(at_console, buf);
                            ALERT(at_console, "\n");
                        }

                        // do your magic lexical analysis here..
                        // first braces
                        switch(buffer[i]) {
                        case '{':
                            braces++;
                            if(start == 1)
                                start = 2;
                            else if(start > 0)
                                start = 99;
                            // need to check that more braces aernt put in than nesesarry
                            // i.e. start=2 then generate error,
                            // cus only have variable setting in start section
                            if(ifsec == 1)
                                ifsec = 2;
                            else if(ifsec > 0)
                                ifsec = 99;
                            if(msgsection == 1)
                                msgsection = 2;
                            else if(msgsection > 0 && ifsec == 0)
                                msgsection = 99;
                            break;
                        case '}':
                            braces--;
                            if(start == 2)
                                start = 0;
                            if(ifsec == 2)
                                ifsec = 0;
                            if(msgsection == 2)
                                msgsection = 0;
                            break;
                        };
                    }
                    buf = _strinc(buf); // like i++ but for stringcmp stuff
                }
            } else {
                ALERT(at_console, "\nScript will not be used until there are no syntax errors\n\n");
            }
        }

        if(bfp != NULL) {
            fclose(bfp);
            bfp = NULL;
        }

        //{ fp = UTIL_OpenFoxbotLog(); fprintf(fp, "StartFrame-my shiznit (end)\n"); fclose(fp); }
        // strcpy(prevmapname,STRING(gpGlobals->mapname));
    }

    if(!mr_meta)
        (*other_gFunctionTable.pfnStartFrame)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

void ParmsNewLevel(void)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnParmsNewLevel)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

void ParmsChangeLevel(void)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnParmsChangeLevel)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

const char* GetGameDescription(void)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnGetGameDescription)();
    else {
        SET_META_RESULT(MRES_HANDLED);
        return ("");
    }
}

void PlayerCustomization(edict_t* pEntity, customization_t* pCust)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "PlayerCustomization: %p\n", (void*)pEntity);
        fclose(fp);
    }

    if(!mr_meta)
        (*other_gFunctionTable.pfnPlayerCustomization)(pEntity, pCust);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void SpectatorConnect(edict_t* pEntity)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSpectatorConnect)(pEntity);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void SpectatorDisconnect(edict_t* pEntity)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSpectatorDisconnect)(pEntity);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void SpectatorThink(edict_t* pEntity)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSpectatorThink)(pEntity);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void Sys_Error(const char* error_string)
{
    // int y = 1, x = 1;

    // x = x - 1;  // x is zero
    // y = y / x;  // cause an divide by zero exception

    // if(debug_engine)
    sz_error_check[251] = '\0';
    {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL)
            fprintf(fp, "SytemError: %s %s\n", error_string, sz_error_check);

        int i;
        for(i = 0; i < 32; i++) {
            if(clients[i] != NULL && fp != NULL)
                fprintf(fp, "%p %d\n", (void*)clients[i], i);
        }

        fclose(fp);

        for(i = 0; i <= 32; i++) {
            if(INDEXENT(i) != NULL)
                UTIL_SavePent(INDEXENT(i));
        }
    }
    if(!mr_meta)
        (*other_gFunctionTable.pfnSys_Error)(error_string);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void PM_Move(struct playermove_s* ppmove, int server)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnPM_Move)(ppmove, server);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void PM_Init(struct playermove_s* ppmove)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnPM_Init)(ppmove);
    else
        SET_META_RESULT(MRES_HANDLED);
}

char PM_FindTextureType(char* name)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnPM_FindTextureType)(name);
    else {
        SET_META_RESULT(MRES_HANDLED);
        return ('\0');
    }
}

void SetupVisibility(edict_t* pViewEntity, edict_t* pClient, unsigned char** pvs, unsigned char** pas)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnSetupVisibility)(pViewEntity, pClient, pvs, pas);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void UpdateClientData(const struct edict_s* ent, int sendweapons, struct clientdata_s* cd)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnUpdateClientData)(ent, sendweapons, cd);
    else {
        gpGamedllFuncs->dllapi_table->pfnUpdateClientData(ent, sendweapons, cd);
        SET_META_RESULT(MRES_SUPERCEDE);
    }
}

int AddToFullPack(struct entity_state_s* state,
    int e,
    edict_t* ent,
    edict_t* host,
    int hostflags,
    int player,
    unsigned char* pSet)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnAddToFullPack)(state, e, ent, host, hostflags, player, pSet);
    else {
        SET_META_RESULT(MRES_HANDLED);
        return (0);
    }
}

void CreateBaseline(int player,
    int eindex,
    struct entity_state_s* baseline,
    struct edict_s* entity,
    int playermodelindex,
    vec3_t player_mins,
    vec3_t player_maxs)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnCreateBaseline)(
            player, eindex, baseline, entity, playermodelindex, player_mins, player_maxs);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void RegisterEncoders(void)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnRegisterEncoders)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

int GetWeaponData(struct edict_s* player, struct weapon_data_s* info)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnGetWeaponData)(player, info);
    else {
        SET_META_RESULT(MRES_HANDLED);
        return (1);
    }
}

void CmdStart(const edict_t* player, const struct usercmd_s* cmd, unsigned int random_seed)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnCmdStart)(player, cmd, random_seed);
    else
        SET_META_RESULT(MRES_HANDLED);
}

void CmdEnd(const edict_t* player)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnCmdEnd)(player);
    else
        SET_META_RESULT(MRES_HANDLED);
}

int ConnectionlessPacket(const struct netadr_s* net_from,
    const char* args,
    char* response_buffer,
    int* response_buffer_size)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnConnectionlessPacket)(net_from, args, response_buffer, response_buffer_size);
    else {
        SET_META_RESULT(MRES_HANDLED);
        return (0);
    }
}

int GetHullBounds(int hullnumber, float* mins, float* maxs)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnGetHullBounds)(hullnumber, mins, maxs);
    else {
        SET_META_RESULT(MRES_HANDLED);
        return (0);
    }
}

void CreateInstancedBaselines(void)
{
    if(!mr_meta)
        (*other_gFunctionTable.pfnCreateInstancedBaselines)();
    else
        SET_META_RESULT(MRES_HANDLED);
}

int InconsistentFile(const edict_t* player, const char* filename, char* disconnect_message)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "InconsistentFile: %p filename=%s\n", (void*)player, filename);
        fclose(fp);
    }

    if(!mr_meta)
        return (*other_gFunctionTable.pfnInconsistentFile)(player, filename, disconnect_message);
    else {
        RETURN_META_VALUE(MRES_HANDLED, 0);
        return (0);
    }
}

int AllowLagCompensation(void)
{
    if(!mr_meta)
        return (*other_gFunctionTable.pfnAllowLagCompensation)();
    else {
        SET_META_RESULT(MRES_HANDLED);
        return (1);
    }
}

DLL_FUNCTIONS gFunctionTable = {
    GameDLLInit,               // pfnGameInit
    DispatchSpawn,             // pfnSpawn
    DispatchThink,             // pfnThink
    DispatchUse,               // pfnUse
    DispatchTouch,             // pfnTouch
    DispatchBlocked,           // pfnBlocked
    DispatchKeyValue,          // pfnKeyValue
    DispatchSave,              // pfnSave
    DispatchRestore,           // pfnRestore
    DispatchObjectCollsionBox, // pfnAbsBox

    SaveWriteFields, // pfnSaveWriteFields
    SaveReadFields,  // pfnSaveReadFields

    SaveGlobalState,    // pfnSaveGlobalState
    RestoreGlobalState, // pfnRestoreGlobalState
    ResetGlobalState,   // pfnResetGlobalState

    ClientConnect,         // pfnClientConnect
    ClientDisconnect,      // pfnClientDisconnect
    ClientKill,            // pfnClientKill
    ClientPutInServer,     // pfnClientPutInServer
    ClientCommand,         // pfnClientCommand
    ClientUserInfoChanged, // pfnClientUserInfoChanged
    ServerActivate,        // pfnServerActivate
    ServerDeactivate,      // pfnServerDeactivate

    PlayerPreThink,  // pfnPlayerPreThink
    PlayerPostThink, // pfnPlayerPostThink

    StartFrame,       // pfnStartFrame
    ParmsNewLevel,    // pfnParmsNewLevel
    ParmsChangeLevel, // pfnParmsChangeLevel

    GetGameDescription,  // pfnGetGameDescription    Returns string describing current .dll game.
    PlayerCustomization, // pfnPlayerCustomization   Notifies .dll of new customization for player.

    SpectatorConnect,    // pfnSpectatorConnect      Called when spectator joins server
    SpectatorDisconnect, // pfnSpectatorDisconnect   Called when spectator leaves the server
    SpectatorThink,      // pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

    Sys_Error, // pfnSys_Error          Called when engine has encountered an error

    PM_Move,            // pfnPM_Move
    PM_Init,            // pfnPM_Init            Server version of player movement initialization
    PM_FindTextureType, // pfnPM_FindTextureType

    SetupVisibility,      // pfnSetupVisibility        Set up PVS and PAS for networking for this client
    UpdateClientData,     // pfnUpdateClientData       Set up data sent only to specific client
    AddToFullPack,        // pfnAddToFullPack
    CreateBaseline,       // pfnCreateBaseline        Tweak entity baseline for network encoding, allows setup of player
                          // baselines, too.
    RegisterEncoders,     // pfnRegisterEncoders      Callbacks for network encoding
    GetWeaponData,        // pfnGetWeaponData
    CmdStart,             // pfnCmdStart
    CmdEnd,               // pfnCmdEnd
    ConnectionlessPacket, // pfnConnectionlessPacket
    GetHullBounds,        // pfnGetHullBounds
    CreateInstancedBaselines, // pfnCreateInstancedBaselines
    InconsistentFile,         // pfnInconsistentFile
    AllowLagCompensation,     // pfnAllowLagCompensation
};

/*#ifdef __BORLANDC__
   int EXPORT GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
 #else
   extern "C" EXPORT int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
 #endif*/

C_DLLEXPORT int GetEntityAPI(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion)
{
    // check if engine's pointer is valid and version is correct...

    if(!pFunctionTable || interfaceVersion != INTERFACE_VERSION)
        return FALSE;

    // pass engine callback function table to engine...
    memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));

    if(!mr_meta) {
        // pass other DLLs engine callbacks to function table...
        if(!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION)) {
            return FALSE; // error initializing function table!!!
        }
    }

    return TRUE;
}

/*#ifdef __BORLANDC__
   int EXPORT GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
 #else
   extern "C" EXPORT int GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
 #endif*/
C_DLLEXPORT int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
    if(other_GetNewDLLFunctions == NULL)
        return FALSE;

    if(!mr_meta) {
        // pass other DLLs engine callbacks to function table...
        if(!(*other_GetNewDLLFunctions)(pFunctionTable, interfaceVersion)) {
            return FALSE; // error initializing function table!!!
        }
    }

    return TRUE;
}

/*void FakeClientCommand (edict_t *pFakeClient, const char *fmt, ...)
   {
   // the purpose of this function is to provide fakeclients (bots) with the same client
   // command-scripting advantages (putting multiple commands in one line between semicolons)
   // as real players. It is an improved version of botman's FakeClientCommand, in which you
   // supply directly the whole string as if you were typing it in the bot's "console". It
   // is supposed to work exactly like the pfnClientCommand (server-sided client command).
   va_list argptr;
   static char command[256];
   int length, fieldstart, fieldstop, i, index, stringindex = 0;

   if(pFakeClient==NULL)
   return;
   // concatenate all the arguments in one string
   va_start (argptr, fmt);
   vsprintf (command, fmt, argptr);
   va_end (argptr);

   if((command == NULL) || (*command == 0))
   return; // if nothing in the command buffer, return

   isFakeClientCommand = TRUE; // set the "fakeclient command" flag
   length = strlen (command); // get the total length of the command string
   // process all individual commands (separated by a semicolon) one each a time
   while(stringindex < length)
   {
   fieldstart = stringindex; // save field start position (first character)
   while((stringindex < length) && (command[stringindex] != ';'))
   stringindex++; // reach end of field
   if(command[stringindex - 1] == '\n')
   fieldstop = stringindex - 2; // discard any trailing '\n' if needed
   else
   fieldstop = stringindex - 1; // save field stop position (last character before semicolon or end)
   for(i = fieldstart; i <= fieldstop; i++)
   g_argv[i - fieldstart] = command[i]; // store the field value in the g_argv global string
   g_argv[i - fieldstart] = 0; // terminate the string
   stringindex++; // move the overall string index one step further to bypass the semicolon
   index = 0;
   fake_arg_count = 0; // let's now parse that command and count the different arguments
   // count the number of arguments
   while(index < i - fieldstart)
   {
   while((index < i - fieldstart) && (g_argv[index] == ' '))
   index++; // ignore spaces
   // is this field a group of words between quotes or a single word ?
   if(g_argv[index] == '"')
   {
   index++; // move one step further to bypass the quote
   while((index < i - fieldstart) && (g_argv[index] != '"'))
   index++; // reach end of field
   index++; // move one step further to bypass the quote
   }
   else
   while((index < i - fieldstart) && (g_argv[index] != ' '))
   index++; // this is a single word, so reach the end of field
   fake_arg_count++; // we have processed one argument more
   }
   }
   if(!mr_meta)
   ClientCommand (pFakeClient); // tell now the MOD DLL to execute this ClientCommand...
   else
   MDLL_ClientCommand(pFakeClient);
   g_argv[0] = 0; // when it's done, reset the g_argv field
   isFakeClientCommand = FALSE; // reset the "fakeclient command" flag
   fake_arg_count = 0; // and the argument count
   }*/

void FakeClientCommand(edict_t* pBot, char* arg1, char* arg2, char* arg3)
{
    int length, i;
    i = 0;
    while(i < 256) {
        g_argv[i] = '\0';
        i++;
    }

    if((arg1 == NULL) || (*arg1 == 0))
        return;

    if(strncmp(arg1, "kill", 4) == 0) {
        if(!mr_meta)
            ClientKill(pBot);
        else
            MDLL_ClientKill(pBot);
        return;
    }

    if((arg2 == NULL) || (*arg2 == 0)) {
        length = _snprintf(&g_argv[0], 250, "%s", arg1);
        fake_arg_count = 1;
    } else if((arg3 == NULL) || (*arg3 == 0)) {
        length = _snprintf(&g_argv[0], 250, "%s %s", arg1, arg2);
        fake_arg_count = 2;
    } else {
        length = _snprintf(&g_argv[0], 250, "%s %s %s", arg1, arg2, arg3);
        fake_arg_count = 3;
    }
    isFakeClientCommand = 1;
    g_argv[length] = '\0'; // null terminate just in case

    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "FakeClientCommand=%s %p\n", g_argv, (void*)pBot);
        fclose(fp);
    }

    // allow the MOD DLL to execute the ClientCommand...
    if(mr_meta) {
        MDLL_ClientCommand(pBot);
    } else
        ClientCommand(pBot);

    isFakeClientCommand = 0;
}

/*const char *pfnCmd_Args (void) {
   // this function returns a pointer to the whole current client command string. Since bots
   // have no client DLL and we may want a bot to execute a client command, we had to implement
   // a g_argv string in the bot DLL for holding the bots' commands, and also keep track of the
   // argument count. Hence this hook not to let the engine ask an unexistent client DLL for a
   // command we are holding here. Of course, real clients commands are still retrieved the
   // normal way, by asking the engine.
   // is this a bot issuing that client command ?
   if(isFakeClientCommand)
   {
   // is it a "say" or "say_team" client command ?
   if(strncmp ("say ", g_argv, 4) == 0)
   return (&g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine)


   else if(strncmp ("say_team ", g_argv, 9) == 0)
   return (&g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL

   engine)
   return (&g_argv[0]); // else return the whole bot client command string we know
   }
   return ((*g_engfuncs.pfnCmd_Args) ()); // ask the client command string to the

   engine
   } */

const char* Cmd_Args(void)
{
    if(!mr_meta) {
        if(isFakeClientCommand) {
            if(debug_engine) {
                fp = UTIL_OpenFoxbotLog();
                fprintf(fp, "fake cmd_args%s\n", &g_argv[0]);
                fclose(fp);
            }

            // is it a "say" or "say_team" client command ?
            if(strncmp("say ", g_argv, 4) == 0)
                return (&g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine)
            // sprintf(g_argv,"%s",&g_argv[0] + 4);
            else if(strncmp("say_team ", g_argv, 9) == 0)
                return (&g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine)
            // sprintf(g_argv,"%s",&g_argv[0] + 9);

            return &g_argv[0];
        } else {
            return (*g_engfuncs.pfnCmd_Args)();
        }
    } else {
        if(isFakeClientCommand) {
            if(debug_engine) {
                fp = UTIL_OpenFoxbotLog();
                fprintf(fp, "fake cmd_args%s\n", &g_argv[0]);
                fclose(fp);
            }

            // is it a "say" or "say_team" client command ?
            if(strncmp("say ", g_argv, 4) == 0)
                RETURN_META_VALUE(
                    MRES_SUPERCEDE, &g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine)
            else if(strncmp("say_team ", g_argv, 9) == 0)
                RETURN_META_VALUE(
                    MRES_SUPERCEDE, &g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine)
            else
                /*if(strncmp ("say ", g_argv, 4) == 0)
                   //return (&g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine)
                   sprintf(g_argv,"%s",&g_argv[0] + 4);
                   else if(strncmp ("say_team ", g_argv, 9) == 0)
                   //return (&g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine)
                   sprintf(g_argv,"%s",&g_argv[0] + 9);*/
                RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0]);
        } else {
            RETURN_META_VALUE(MRES_IGNORED, NULL);
        }
    }
}

const char* GetArg(const char* command, int arg_number)
{
    // the purpose of this function is to provide fakeclients (bots) with
    // the same Cmd_Argv convenience the engine provides to real clients.
    // This way the handling of real client commands and bot client
    // commands is exactly the same, just have a look in engine.cpp for
    // the hooking of pfnCmd_Argc, pfnCmd_Args and pfnCmd_Argv, which
    // redirects the call either to the actual engine functions
    // (when the caller is a real client), either on our function here,
    // which does the same thing, when the caller is a bot.
    int length, i, index = 0, arg_count = 0, fieldstart, fieldstop;
    arg[0] = 0;               // reset arg
    length = strlen(command); // get length of command
    // while we have not reached end of line
    while((index < length) && (arg_count <= arg_number)) {
        while((index < length) && (command[index] == ' '))
            index++; // ignore spaces
        // is this field multi-word between quotes or single word ?
        if(command[index] == '"') {
            index++;            // move one step further to bypass the quote
            fieldstart = index; // save field start position
            while((index < length) && (command[index] != '"'))
                index++;           // reach end of field
            fieldstop = index - 1; // save field stop position
            index++;               // move one step further to bypass the quote
        } else {
            fieldstart = index; // save field start position
            while((index < length) && (command[index] != ' '))
                index++;           // reach end of field
            fieldstop = index - 1; // save field stop position
        }
        // is this argument we just processed the wanted one ?
        if(arg_count == arg_number) {
            for(i = fieldstart; i <= fieldstop; i++)
                arg[i - fieldstart] = command[i]; // store the field value in a string
            arg[i - fieldstart] = 0;              // terminate the string
        }
        arg_count++; // we have processed one argument more
    }
    return (&arg[0]); // returns the wanted argument
}

const char* Cmd_Argv(int argc)
{
    // this function returns a pointer to a certain argument of the
    // current client command. Since bots have no client DLL and we may
    // want a bot to execute a client command, we had to implement a
    // g_argv string in the bot DLL for holding the bots' commands,
    // and also keep track of the argument count.
    // Hence this hook not to let the engine ask an unexistent client
    // DLL for a command we are holding here. Of course, real clients
    // commands are still retrieved the normal way, by asking the engine.
    // is this a bot issuing that client command ?

    // return ((*g_engfuncs.pfnCmd_Argv) (argc)); // ask the argument number "argc" to the engine

    if(isFakeClientCommand) {
        if(!mr_meta)
            return (GetArg(g_argv, argc)); // if so, then return the wanted argument we know
        else
            RETURN_META_VALUE(MRES_SUPERCEDE, GetArg(g_argv, argc));
    } else {
        if(!mr_meta) {
            return (*g_engfuncs.pfnCmd_Argv)(argc);
        } else {
            RETURN_META_VALUE(MRES_IGNORED, NULL);
        }
    }
}

int Cmd_Argc(void)
{
    if(!mr_meta) {
        if(debug_engine)
            UTIL_BotLogPrintf("fake cmd_argc %d\n", fake_arg_count);

        if(isFakeClientCommand) {
            return fake_arg_count;
        } else {
            return (*g_engfuncs.pfnCmd_Argc)();
        }
    } else {
        if(isFakeClientCommand) {
            RETURN_META_VALUE(MRES_SUPERCEDE, fake_arg_count);
        } else {
            RETURN_META_VALUE(MRES_IGNORED, 0);
        }
    }
}

// meta mod post functions

void DispatchKeyValue_Post(edict_t* pentKeyvalue, KeyValueData* pkvd)
{
    static edict_t* temp_pent;
    static int flag_index;

    // fp=UTIL_OpenFoxbotLog(); fprintf(fp, "DispatchKeyValue: %x %s=%s\n",pentKeyvalue,pkvd->szKeyName,pkvd->szValue);
    // fclose(fp);

    if(mod_id == TFC_DLL) {
        if(pentKeyvalue == pent_info_tfdetect) {
            if(strcmp(pkvd->szKeyName, "ammo_medikit") == 0) // max BLUE players
                max_team_players[0] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "ammo_detpack") == 0) // max RED players
                max_team_players[1] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_medikit") == 0) // max YELLOW players
                max_team_players[2] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_detpack") == 0) // max GREEN players
                max_team_players[3] = atoi(pkvd->szValue);

            else if(strcmp(pkvd->szKeyName, "maxammo_shells") == 0) // BLUE class limits
                team_class_limits[0] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_nails") == 0) // RED class limits
                team_class_limits[1] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_rockets") == 0) // YELLOW class limits
                team_class_limits[2] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "maxammo_cells") == 0) // GREEN class limits
                team_class_limits[3] = atoi(pkvd->szValue);

            else if(strcmp(pkvd->szKeyName, "team1_allies") == 0) // BLUE allies
                team_allies[0] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "team2_allies") == 0) // RED allies
                team_allies[1] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "team3_allies") == 0) // YELLOW allies
                team_allies[2] = atoi(pkvd->szValue);
            else if(strcmp(pkvd->szKeyName, "team4_allies") == 0) // GREEN allies
                team_allies[3] = atoi(pkvd->szValue);
        } else if(pent_info_tfdetect == NULL) {
            if((strcmp(pkvd->szKeyName, "classname") == 0) && (strcmp(pkvd->szValue, "info_tfdetect") == 0)) {
                pent_info_tfdetect = pentKeyvalue;
            }
        }

        if(pentKeyvalue == pent_item_tfgoal) {
            if(strcmp(pkvd->szKeyName, "team_no") == 0)
                flags[flag_index].team_no = atoi(pkvd->szValue);

            if((strcmp(pkvd->szKeyName, "mdl") == 0) && ((strcmp(pkvd->szValue, "models/flag.mdl") == 0) ||
                                                            (strcmp(pkvd->szValue, "models/keycard.mdl") == 0) ||
                                                            (strcmp(pkvd->szValue, "models/ball.mdl") == 0))) {
                flags[flag_index].mdl_match = TRUE;
                num_flags++;
            }
        } else if(pent_item_tfgoal == NULL) {
            if((strcmp(pkvd->szKeyName, "classname") == 0) && (strcmp(pkvd->szValue, "item_tfgoal") == 0)) {
                if(num_flags < MAX_FLAGS) {
                    pent_item_tfgoal = pentKeyvalue;

                    flags[num_flags].mdl_match = FALSE;
                    flags[num_flags].team_no = 0; // any team unless specified
                    flags[num_flags].edict = pentKeyvalue;

                    // in case the mdl comes before team_no
                    flag_index = num_flags;
                }
            }
        } else {
            pent_item_tfgoal = NULL; // reset for non-flag item_tfgoal's
        }

        if((strcmp(pkvd->szKeyName, "classname") == 0) &&
            ((strcmp(pkvd->szValue, "info_player_teamspawn") == 0) ||
                (strcmp(pkvd->szValue, "info_tf_teamcheck") == 0) || (strcmp(pkvd->szValue, "i_p_t") == 0))) {
            temp_pent = pentKeyvalue;
        } else if(pentKeyvalue == temp_pent) {
            if(strcmp(pkvd->szKeyName, "team_no") == 0) {
                // int value = atoi(pkvd->szValue);

                // is_team[value-1]=true;
                // if(value > max_teams)
                // max_teams = value;
            }
        }
    } /*else if(mod_id == GEARBOX_DLL) {
        if(pent_info_ctfdetect == NULL) {
            if((strcmp(pkvd->szKeyName, "classname") == 0) && (strcmp(pkvd->szValue, "info_ctfdetect") == 0)) {
                pent_info_ctfdetect = pentKeyvalue;
            }
        }
    }*/

    /*if(!mr_meta)
       (*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
       else*/
    SET_META_RESULT(MRES_HANDLED);
}

static DLL_FUNCTIONS gFunctionTable_Post = {
    NULL, //! pfnGameInit()				Initialize the game (one-time call after loading of game .dll)
    NULL, //! pfnSpawn()
    NULL, //! pfnThink()
    NULL, //! pfnUse()
    NULL, //! pfnTouch()
    NULL, //! pfnBlocked()
    NULL, // DispatchKeyValue_Post,			//! pfnKeyValue()
    NULL, //! pfnSave()
    NULL, //! pfnRestore()
    NULL, //! pfnSetAbsBox()

    NULL, //! pfnSaveWriteFields()
    NULL, //! pfnSaveReadFields()

    NULL, //! pfnSaveGlobalState()
    NULL, //! pfnRestoreGlobalState()
    NULL, //! pfnResetGlobalState()

    ClientConnect_Post, //! pfnClientConnect()			(wd) Client has connected
    NULL,               //! pfnClientDisconnect()		(wd) Player has left the game
    NULL,               //! pfnClientKill()				(wd) Player has typed "kill"
    NULL,               //! pfnClientPutInServer()		(wd) Client is entering the game
    NULL,               //! pfnClientCommand()			(wd) Player has sent a command (typed, or from a bind)
    NULL,               //! pfnClientUserInfoChanged()	(wd) Client has updated their setinfo structure
    NULL,               //! pfnServerActivate()			(wd) Server is starting a new map
    NULL,               //! pfnServerDeactivate()		(wd) Server is leaving the map (shutdown, or changelevel); SDK2

    NULL, //! pfnPlayerPreThink()
    NULL, //! pfnPlayerPostThink()

    NULL, //! pfnStartFrame()
    NULL, //! pfnParmsNewLevel()
    NULL, //! pfnParmsChangeLevel()

    NULL, //! pfnGetGameDescription()		Returns string describing current .dll.  E.g. "TeamFotrress 2",
          //! "Half-Life"
    NULL, //! pfnPlayerCustomization()	Notifies .dll of new customization for player.

    NULL, //! pfnSpectatorConnect()		Called when spectator joins server
    NULL, //! pfnSpectatorDisconnect()	Called when spectator leaves the server
    NULL, //! pfnSpectatorThink()			Called when spectator sends a command packet (usercmd_t)

    NULL, //! pfnSys_Error()				Notify game .dll that engine is going to shut down.
          //! Allows mod authors to set a breakpoint.  SDK2

    NULL, //! pfnPM_Move()				(wd) SDK2
    NULL, //! pfnPM_Init()				Server version of player movement initialization; (wd) SDK2
    NULL, //! pfnPM_FindTextureType()		(wd) SDK2

    NULL, //! pfnSetupVisibility()		Set up PVS and PAS for networking for this client; (wd) SDK2
    NULL, //! pfnUpdateClientData()		Set up data sent only to specific client; (wd) SDK2
    NULL, //! pfnAddToFullPack()			(wd) SDK2
    NULL, //! pfnCreateBaseline()			Tweak entity baseline for network encoding, allows setup of
          //! player baselines, too.; (wd) SDK2
    NULL, //! pfnRegisterEncoders()		Callbacks for network encoding; (wd) SDK2
    NULL, //! pfnGetWeaponData()			(wd) SDK2
    NULL, //! pfnCmdStart()				(wd) SDK2
    NULL, //! pfnCmdEnd()					(wd) SDK2
    NULL, //! pfnConnectionlessPacket()	(wd) SDK2
    NULL, //! pfnGetHullBounds()			(wd) SDK2
    NULL, //! pfnCreateInstancedBaselines()	(wd) SDK2
    NULL, //! pfnInconsistentFile()		(wd) SDK2
    NULL, //! pfnAllowLagCompensation()	(wd) SDK2
};

C_DLLEXPORT int GetEntityAPI_Post(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion)
{
    if(!pFunctionTable) {
        return (FALSE);
    } else if(interfaceVersion != INTERFACE_VERSION) {
        return (FALSE);
    }
    memcpy(pFunctionTable, &gFunctionTable_Post, sizeof(DLL_FUNCTIONS));
    return (TRUE);
}

static void ProcessBotCfgFile(void)
{
    int ch;
    char cmd_line[512];
    int cmd_index;
    static char server_cmd[514];
    char *cmd, *arg1, *arg2, *arg3, *arg4;
    char msg[255];

    if(bot_cfg_pause_time > gpGlobals->time)
        return;

    if(bot_cfg_fp == NULL) {
        if(cfg_file == 1)
            need_to_open_cfg2 = TRUE;
        if(cfg_file == 2) {
            cfg_file = 0;
            bot_cfg_pause_time = 0.0; // stop it checking the file again :)
        }
        return;
    }

    cmd_index = 0;
    cmd_line[cmd_index] = 0;
    cmd_line[510] = 0;

    ch = fgetc(bot_cfg_fp);

    // skip any leading blanks
    while(ch == ' ')
        ch = fgetc(bot_cfg_fp);

    while((ch != EOF) && (ch != '\r') && (ch != '\n') && (cmd_index < 510)) {
        if(ch == '\t') // convert tabs to spaces
            ch = ' ';

        //{fp=UTIL_OpenFoxbotLog(); fprintf(fp,"cfg %d %i\n",cmd_index,ch); fclose(fp); }
        cmd_line[cmd_index] = static_cast<char>(ch);

        ch = fgetc(bot_cfg_fp);

        // skip multiple spaces in input file
        while((cmd_line[cmd_index] == ' ') && (ch == ' '))
            ch = fgetc(bot_cfg_fp);

        cmd_index++;
    }

    if(ch == '\r') // is it a carriage return?
    {
        ch = fgetc(bot_cfg_fp); // skip the linefeed
    }

    // if reached end of file, then close it
    if(ch == EOF) {
        fclose(bot_cfg_fp);

        bot_cfg_fp = NULL;
        /*if(debug_engine)
                {fp=UTIL_OpenFoxbotLog(); fprintf(fp,"close cfg\n"); fclose(fp);}*/

        // dam reset
        // bot_cfg_pause_time = 0.0;

        // sort new cfg shiznit out
        if(cfg_file == 1)
            need_to_open_cfg2 = TRUE;
        if(cfg_file == 2) {
            cfg_file = 3; // 0 is for if it was null when we passed in
            bot_cfg_pause_time = 0.0;
        }
    }

    cmd_line[cmd_index] = 0; // terminate the command line

    if((cmd_line[0] == '#') || (cmd_line[0] == 0))
        return; // return if comment or blank line

    // DREVIL
    if(strncmp("echo", cmd_line, 4) == 0)
        return;

    // copy the command line to a server command buffer...
    strcpy(server_cmd, cmd_line);
    strcat(server_cmd, "\n");

    cmd_index = 0;
    cmd = cmd_line;
    arg1 = arg2 = arg3 = arg4 = NULL;

    // skip to blank or end of string...
    while((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0) && (cmd_index < 510))
        cmd_index++;
    if(cmd_line[cmd_index] == ' ' && (cmd_index < 510)) {
        cmd_line[cmd_index++] = 0;
        arg1 = &cmd_line[cmd_index];

        // skip to blank or end of string...
        while((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0) && (cmd_index < 510))
            cmd_index++;
        if(cmd_line[cmd_index] == ' ' && (cmd_index < 510)) {
            cmd_line[cmd_index++] = 0;
            arg2 = &cmd_line[cmd_index];

            while((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0) && (cmd_index < 510))
                cmd_index++;
            if(cmd_line[cmd_index] == ' ' && (cmd_index < 510)) {
                cmd_line[cmd_index++] = 0;
                arg3 = &cmd_line[cmd_index];

                while((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0) && (cmd_index < 510))
                    cmd_index++;
                if(cmd_line[cmd_index] == ' ' && (cmd_index < 510)) {
                    cmd_line[cmd_index++] = 0;
                    arg4 = &cmd_line[cmd_index];
                }
            }
        }
    }

    if(strcmp(cmd, "addbot") == 0) {
        if(IS_DEDICATED_SERVER()) {
            printf("[Config] add bot (%s,%s,%s,%s)\n", arg1, arg2, arg3, arg4);
        } else {
            sprintf(msg, "[Config] add bot (%s,%s,%s,%s)\n", arg1, arg2, arg3, arg4);
            ALERT(at_console, msg);
        }
        BotCreate(NULL, arg1, arg2, arg3, arg4);

        // have to delay here or engine gives "Tried to write to
        // uninitialized sizebuf_t" error and crashes...
        bot_cfg_pause_time = gpGlobals->time + 2.0f;
        bot_check_time = gpGlobals->time + 5.0f;

        return;
    }

    if(strcmp(cmd, "bot_chat") == 0) {
        changeBotSetting("bot_chat", &botchat, arg1, 0, 1000, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "botskill_lower") == 0) {
        changeBotSetting("botskill_lower", &botskill_lower, arg1, 1, 5, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "botskill_upper") == 0) {
        changeBotSetting("botskill_upper", &botskill_upper, arg1, 1, 5, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "bot_skill_1_aim") == 0) {
        changeBotSetting("bot_skill_1_aim", &bot_skill_1_aim, arg1, 0, 200, SETTING_SOURCE_CONFIG_FILE);

        BotUpdateSkillInaccuracy();
        return;
    }

    if(strcmp(cmd, "bot_aim_per_skill") == 0) {
        changeBotSetting("bot_aim_per_skill", &bot_aim_per_skill, arg1, 5, 50, SETTING_SOURCE_CONFIG_FILE);

        BotUpdateSkillInaccuracy();
        return;
    }

    if(strcmp(cmd, "observer") == 0) {
        int temp = atoi(arg1);

        if(temp)
            b_observer_mode = TRUE;
        else
            b_observer_mode = FALSE;

        return;
    }

    if(strcmp(cmd, "bot_allow_moods") == 0) {
        changeBotSetting("bot_allow_moods", &bot_allow_moods, arg1, 0, 1, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "bot_allow_humour") == 0) {
        changeBotSetting("bot_allow_humour", &bot_allow_humour, arg1, 0, 1, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "bot_use_grenades") == 0) {
        changeBotSetting("bot_use_grenades", &bot_use_grenades, arg1, 0, 2, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "min_bots") == 0) {
        changeBotSetting("min_bots", &min_bots, arg1, -1, 31, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "max_bots") == 0) {
        changeBotSetting("max_bots", &max_bots, arg1, -1, MAX_BOTS, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "bot_total_varies") == 0) {
        changeBotSetting("bot_total_varies", &bot_total_varies, arg1, 0, 3, SETTING_SOURCE_CONFIG_FILE);
        return;
    }

    if(strcmp(cmd, "bot_team_balance") == 0) {
        if((arg1 != NULL)) {
            int temp = atoi(arg1);
            if(temp)
                bot_team_balance = TRUE;
            else
                bot_team_balance = FALSE;
        }

        if(IS_DEDICATED_SERVER()) {
            if(bot_team_balance)
                printf("[Config] bot_team_balance (1) On\n");
            else
                printf("[Config] bot_team_balance (0) Off\n");
        } else {
            if(bot_team_balance)
                ALERT(at_console, "[Config] bot_team_balance (1) On\n");
            else
                ALERT(at_console, "[Config] bot_team_balance (0) Off\n");
        }
        return;
    }
    if(strcmp(cmd, "bot_bot_balance") == 0) {
        if((arg1 != NULL)) {
            int temp = atoi(arg1);
            if(temp)
                bot_bot_balance = TRUE;
            else
                bot_bot_balance = FALSE;
        }

        if(IS_DEDICATED_SERVER()) {
            if(bot_bot_balance)
                printf("[Config] bot_bot_balance (1) On\n");
            else
                printf("[Config] bot_bot_balance (0) Off\n");
        } else {
            if(bot_bot_balance)
                ALERT(at_console, "[Config] bot_bot_balance (1) On\n");
            else
                ALERT(at_console, "[Config] bot_bot_balance (0) Off\n");
        }
        return;
    }

    if(strcmp(cmd, "bot_xmas") == 0) {
        int temp = atoi(arg1);
        bot_xmas = TRUE;
        if(temp == 0) {
            bot_xmas = FALSE;
            if(IS_DEDICATED_SERVER())
                printf("[Config] bot xmas (0) off\n");
            else {
                sprintf(msg, "[Config] bot xmas (0) off\n");
                ALERT(at_console, msg);
            }
        } else {
            if(IS_DEDICATED_SERVER())
                printf("[Config] bot xmas (1) on\n");
            else {
                sprintf(msg, "[Config] bot xmas (1) on\n");
                ALERT(at_console, msg);
            }
        }
        return;
    }

    if(strcmp(cmd, "bot_can_build_teleporter") == 0) {
        if(strcmp(arg1, "on") == 0) {
            bot_can_build_teleporter = TRUE;
            if(IS_DEDICATED_SERVER())
                printf("[Config] bot_can_build_teleporter on\n");
            else {
                sprintf(msg, "[Config] bot_can_build_teleporter on\n");
                ALERT(at_console, msg);
            }
        } else if(strcmp(arg1, "off") == 0) {
            bot_can_build_teleporter = FALSE;
            if(IS_DEDICATED_SERVER())
                printf("[Config] bot_can_build_teleporter off\n");
            else {
                sprintf(msg, "[Config] bot_can_build_teleporter off\n");
                ALERT(at_console, msg);
            }
        }
        return;
    }

    if(strcmp(cmd, "bot_can_use_teleporter") == 0) {
        if(strcmp(arg1, "on") == 0) {
            bot_can_use_teleporter = TRUE;
            if(IS_DEDICATED_SERVER())
                printf("[Config] bot_can_use_teleporter on\n");
            else {
                sprintf(msg, "[Config] bot_can_use_teleporter on\n");
                ALERT(at_console, msg);
            }
        } else if(strcmp(arg1, "off") == 0) {
            bot_can_use_teleporter = FALSE;
            if(IS_DEDICATED_SERVER())
                printf("[Config] bot_can_use_teleporter off\n");
            else {
                sprintf(msg, "[Config] bot_can_use_teleporter off\n");
                ALERT(at_console, msg);
            }
        }
        return;
    }

    if(strcmp(cmd, "pause") == 0) {
        bot_cfg_pause_time = gpGlobals->time + atoi(arg1);
        bot_check_time = bot_cfg_pause_time; // bot_check_time governs bot spawn time too
        if(IS_DEDICATED_SERVER())
            printf("[Config] pause has been set to %s\n", arg1);
        else {
            sprintf(msg, "[Config] pause has been set to %s\n", arg1);
            ALERT(at_console, msg);
        }
        return;
    }

    if(strcmp(cmd, "bot_create_interval") == 0) {
        bot_create_interval = static_cast<float>(atoi(arg1));
        if(bot_create_interval < 1.0 || bot_create_interval > 8.0)
            bot_create_interval = 3.0;

        if(IS_DEDICATED_SERVER())
            printf("[Config] bot_create_interval has been set to %s\n", arg1);
        else {
            sprintf(msg, "[Config] bot_create_interval has been set to %s\n", arg1);
            ALERT(at_console, msg);
        }
        return;
    }

    if(strcmp(cmd, "defensive_chatter") == 0) {
        int temp = atoi(arg1);
        if(temp == 0)
            defensive_chatter = FALSE;
        else
            defensive_chatter = TRUE;

        if(IS_DEDICATED_SERVER()) {
            printf("[Config] defensive chatter is %s\n", arg1);
        } else {
            sprintf(msg, "[Config] defensive chatter is %s\n", arg1);
            ALERT(at_console, msg);
        }
        return;
    }
    if(strcmp(cmd, "offensive_chatter") == 0) {
        int temp = atoi(arg1);
        if(temp == 0)
            offensive_chatter = FALSE;
        else
            offensive_chatter = TRUE;

        if(IS_DEDICATED_SERVER()) {
            printf("[Config] offensive chatter is %s\n", arg1);
        } else {
            sprintf(msg, "[Config] offensive chatter is %s\n", arg1);
            ALERT(at_console, msg);
        }
        return;
    }
    /*	if(strcmp(cmd, "frag_commander") == 0)
            {
                    int temp = atoi( arg1 );
                    if(temp == 0)
                            frag_commander = FALSE;
                    else
                            frag_commander = TRUE;

                    if(IS_DEDICATED_SERVER())
                    {
                            printf("[Config] Score-based commander is %s\n",arg1);
                    }
                    else
                    {
                            sprintf(msg,"[Config] Score-based commander is %s\n",arg1);
                            ALERT( at_console, msg);
                    }
                    return;
            }*/

    sprintf(msg, "executing server command: %s\n", server_cmd);
    ALERT(at_console, msg);

    if(IS_DEDICATED_SERVER())
        printf(msg);

    SERVER_COMMAND(server_cmd);
}

void UTIL_SavePent(edict_t* pent)
{
    FILE* fp = UTIL_OpenFoxbotLog();
    if(fp == NULL)
        return;

    fprintf(fp, "*edict_t %p\n", (void*)pent);
    fprintf(fp, "classname %s\n", STRING(pent->v.classname));
    fprintf(fp, "globalname %s\n", STRING(pent->v.globalname));
    fprintf(fp, "origin %f %f %f\n", pent->v.origin.x, pent->v.origin.y, pent->v.origin.z);
    fprintf(fp, "oldorigin %f %f %f\n", pent->v.oldorigin.x, pent->v.oldorigin.y, pent->v.oldorigin.z);
    fprintf(fp, "velocity %f %f %f\n", pent->v.velocity.x, pent->v.velocity.y, pent->v.velocity.z);
    fprintf(fp, "basevelocity %f %f %f\n", pent->v.basevelocity.x, pent->v.basevelocity.y, pent->v.basevelocity.z);
    fprintf(
        fp, "clbasevelocity %f %f %f\n", pent->v.clbasevelocity.x, pent->v.clbasevelocity.y, pent->v.clbasevelocity.z);
    fprintf(fp, "movedir %f %f %f\n", pent->v.movedir.x, pent->v.movedir.y, pent->v.movedir.z);
    fprintf(fp, "angles %f %f %f\n", pent->v.angles.x, pent->v.angles.y, pent->v.angles.z);
    fprintf(fp, "avelocity %f %f %f\n", pent->v.avelocity.x, pent->v.avelocity.y, pent->v.avelocity.z);
    fprintf(fp, "punchangle %f %f %f\n", pent->v.punchangle.x, pent->v.punchangle.y, pent->v.punchangle.z);
    fprintf(fp, "v_angles %f %f %f\n", pent->v.v_angle.x, pent->v.v_angle.y, pent->v.v_angle.z);
    fprintf(fp, "endpos %f %f %f\n", pent->v.endpos.x, pent->v.endpos.y, pent->v.endpos.z);
    fprintf(fp, "startpos %f %f %f\n", pent->v.startpos.x, pent->v.startpos.y, pent->v.startpos.z);
    fprintf(fp, "impacttime %f\n", pent->v.impacttime);
    fprintf(fp, "starttime %f\n", pent->v.starttime);
    fprintf(fp, "fixangle %d\n", pent->v.fixangle);
    fprintf(fp, "idealpitch %f\n", pent->v.idealpitch);
    fprintf(fp, "pitch_speed %f\n", pent->v.pitch_speed);
    fprintf(fp, "ideal_yaw %f\n", pent->v.ideal_yaw);
    fprintf(fp, "yaw_speed %f\n", pent->v.yaw_speed);
    fprintf(fp, "modelindex %d\n", pent->v.modelindex);
    fprintf(fp, "model %s\n", STRING(pent->v.model));
    fprintf(fp, "viewmodel %d\n", pent->v.viewmodel);
    fprintf(fp, "weaponmodel %d\n", pent->v.weaponmodel);
    fprintf(fp, "absmin %f %f %f\n", pent->v.absmin.x, pent->v.absmin.y, pent->v.absmin.z);
    fprintf(fp, "absmax %f %f %f\n", pent->v.absmax.x, pent->v.absmax.y, pent->v.absmax.z);
    fprintf(fp, "mins %f %f %f\n", pent->v.mins.x, pent->v.mins.y, pent->v.mins.z);
    fprintf(fp, "maxs %f %f %f\n", pent->v.maxs.x, pent->v.maxs.y, pent->v.maxs.z);
    fprintf(fp, "size %f %f %f\n", pent->v.size.x, pent->v.size.y, pent->v.size.z);
    fprintf(fp, "ltime %f\n", pent->v.ltime);
    fprintf(fp, "nextthink %f\n", pent->v.nextthink);
    fprintf(fp, "movetype %d\n", pent->v.movetype);
    fprintf(fp, "solid %d\n", pent->v.solid);
    fprintf(fp, "skin %d\n", pent->v.skin);
    fprintf(fp, "body %d\n", pent->v.body);
    fprintf(fp, "effects %d\n", pent->v.effects);
    fprintf(fp, "gravity %f\n", pent->v.gravity);
    fprintf(fp, "friction %f\n", pent->v.friction);
    fprintf(fp, "light_level %d %d\n", pent->v.light_level, GETENTITYILLUM(pent));
    if((pent->v.pContainingEntity) != NULL)
        fprintf(fp, "cont light_level %d\n", GETENTITYILLUM(pent->v.pContainingEntity));
    fprintf(fp, "health %f\n", pent->v.health);
    fprintf(fp, "frags %f\n", pent->v.frags);
    fprintf(fp, "weapons %d\n", pent->v.weapons);
    fprintf(fp, "takedamage %f\n", pent->v.takedamage);
    fprintf(fp, "deadflag %d\n", pent->v.deadflag);
    fprintf(fp, "view_ofs %f %f %f\n", pent->v.view_ofs.x, pent->v.view_ofs.y, pent->v.view_ofs.z);
    fprintf(fp, "button %d\n", pent->v.button);
    fprintf(fp, "impulse %d\n", pent->v.impulse);
    fprintf(fp, "*chain %p\n", (void*)pent->v.chain);
    fprintf(fp, "*dmg_inflictor %p\n", (void*)pent->v.dmg_inflictor);
    fprintf(fp, "*enemy %p\n", (void*)pent->v.enemy);
    fprintf(fp, "*aiment %p\n", (void*)pent->v.aiment);
    fprintf(fp, "*owner %p\n", (void*)pent->v.owner);
    fprintf(fp, "*grounentity %p\n", (void*)pent->v.groundentity);
    fprintf(fp, "spawnflags %d\n", pent->v.spawnflags);
    fprintf(fp, "flags %d\n", pent->v.flags);
    fprintf(fp, "colormap %d\n", pent->v.colormap);
    fprintf(fp, "team %d\n", pent->v.team);
    fprintf(fp, "max_health %f\n", pent->v.max_health);
    fprintf(fp, "teleport_time %f\n", pent->v.teleport_time);
    fprintf(fp, "armortype %f\n", pent->v.armortype);
    fprintf(fp, "armorvalue %f\n", pent->v.armorvalue);
    fprintf(fp, "waterlevel %d\n", pent->v.waterlevel);
    fprintf(fp, "watertype %d\n", pent->v.watertype);
    fprintf(fp, "target %s\n", STRING(pent->v.target));
    fprintf(fp, "targetname %s\n", STRING(pent->v.targetname));
    fprintf(fp, "netname %s\n", STRING(pent->v.netname));
    fprintf(fp, "message %s\n", STRING(pent->v.message));
    fprintf(fp, "dmg_take %f\n", pent->v.dmg_take);
    fprintf(fp, "dmg_save %f\n", pent->v.dmg_save);
    fprintf(fp, "dmg %f\n", pent->v.dmg);
    fprintf(fp, "dmgtime %f\n", pent->v.dmgtime);
    fprintf(fp, "noise %s\n", STRING(pent->v.noise));
    fprintf(fp, "noise1 %s\n", STRING(pent->v.noise1));
    fprintf(fp, "noise2 %s\n", STRING(pent->v.noise2));
    fprintf(fp, "noise3 %s\n", STRING(pent->v.noise3));
    fprintf(fp, "speed %f\n", pent->v.speed);
    fprintf(fp, "air_finished %f\n", pent->v.air_finished);
    fprintf(fp, "pain_finished %f\n", pent->v.pain_finished);
    fprintf(fp, "pContainingEntity %p\n", (void*)(pent->v.pContainingEntity));
    fprintf(fp, "playerclass %d\n", pent->v.playerclass);
    fprintf(fp, "maxspeed %f\n", pent->v.maxspeed);
    fprintf(fp, "fov %f\n", pent->v.fov);
    fprintf(fp, "weaponanim %d\n", pent->v.weaponanim);
    fprintf(fp, "pushmsec %d\n", pent->v.pushmsec);
    fprintf(fp, "bInDuck %d\n", pent->v.bInDuck);
    fprintf(fp, "flTimeStepSound %d\n", pent->v.flTimeStepSound);
    fprintf(fp, "flSwimTime %d\n", pent->v.flSwimTime);
    fprintf(fp, "flDuckTime %d\n", pent->v.flDuckTime);
    fprintf(fp, "iStepLeft %d\n", pent->v.iStepLeft);
    fprintf(fp, "flFallVelocity %f\n", pent->v.flFallVelocity);
    fprintf(fp, "gamestate %d\n", pent->v.gamestate);
    fprintf(fp, "oldbuttons %d\n", pent->v.oldbuttons);
    fprintf(fp, "groupinfo %d\n", pent->v.groupinfo);
    fprintf(fp, "iuser1 %d\n", pent->v.iuser1);
    fprintf(fp, "iuser2 %d\n", pent->v.iuser2);
    fprintf(fp, "iuser3 %d\n", pent->v.iuser3);
    fprintf(fp, "iuser4 %d\n", pent->v.iuser4);
    fprintf(fp, "fuser1 %f\n", pent->v.fuser1);
    fprintf(fp, "fuser2 %f\n", pent->v.fuser2);
    fprintf(fp, "fuser3 %f\n", pent->v.fuser3);
    fprintf(fp, "fuser4 %f\n", pent->v.fuser4);
    fprintf(fp, "vuser1 %f %f %f\n", pent->v.vuser1.x, pent->v.vuser1.y, pent->v.vuser1.z);
    fprintf(fp, "vuser2 %f %f %f\n", pent->v.vuser2.x, pent->v.vuser2.y, pent->v.vuser2.z);
    fprintf(fp, "vuser3 %f %f %f\n", pent->v.vuser3.x, pent->v.vuser3.y, pent->v.vuser3.z);
    fprintf(fp, "vuser4 %f %f %f\n", pent->v.vuser4.x, pent->v.vuser4.y, pent->v.vuser4.z);
    fprintf(fp, "euser1 %p\n", (void*)(pent->v.euser1));
    fprintf(fp, "euser2 %p\n", (void*)(pent->v.euser2));
    fprintf(fp, "euser3 %p\n", (void*)(pent->v.euser3));
    fprintf(fp, "euser4 %p\n", (void*)(pent->v.euser4));

    fprintf(fp, "-info buffer %s\n", g_engfuncs.pfnGetInfoKeyBuffer(pent));
    fclose(fp);
}

static void DisplayBotInfo()
{
    char msg[255];
    char msg2[512];

    if(IS_DEDICATED_SERVER()) {
        // tell the console all the bot vars

        _snprintf(msg2, 511, "--FoxBot Loaded--\n--Visit 'www.apg-clan.org' for updates and info--\n");
        msg2[511] = '\0'; // just in case
        printf(msg2);

        /*	sprintf(msg,"--* foxbot v%d.%d build# %d *--\n",
                        VER_MAJOR,VER_MINOR,VER_BUILD);*/

        sprintf(msg, "--* foxbot v%d.%d *--\n", VER_MAJOR, VER_MINOR);
        printf(msg);

        strncat(msg2, msg, 511 - strlen(msg2));
        sprintf(msg, "\n--FoxBot info--\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        // waypoints
        if(num_waypoints > 0)
            sprintf(msg, "Waypoints loaded\n");
        else
            sprintf(msg, "Waypoints NOT loaded\n--Warning bots will not navigate correctly!--\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        // area file
        if(num_areas > 0)
            sprintf(msg, "Areas loaded\n");
        else
            sprintf(msg, "Areas not loaded\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        // scripts...loaded/passed?
        if(script_loaded) {
            if(script_parsed)
                sprintf(msg, "Script loaded and parsed\n");
            else
                sprintf(msg,
                    "Script loaded and NOT parsed\n--Warning script file has an error in it, will NOT be used!--\n");
        } else
            sprintf(msg, "No script file loaded\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // now bots vars
        sprintf(msg, "\n--FoxBot vars--\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // bot skill levels
        sprintf(msg, "botskill_lower %d\nbotskill_upper %d\n", botskill_lower, botskill_upper);
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        sprintf(msg, "max_bots %d\nmin_bots %d\n", max_bots, min_bots);
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // bot chat
        sprintf(msg, "Bot chat %d\n", botchat);
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        if(bot_team_balance)
            sprintf(msg, "Bot auto team balance On\n");
        else
            sprintf(msg, "Bot auto team balance Off\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        if(bot_bot_balance)
            sprintf(msg, "Bot per team balance On\n");
        else
            sprintf(msg, "Bot per team balance Off\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        sprintf(msg, "\n--All bot commands must be enclosed in quotes--\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        sprintf(msg, "e.g. bot \"bot_chat 20\"\n\n");
        printf(msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        ALERT(at_logged, "[FOXBOT]: %s", msg2);
    } else {
        // have to switch developer 'on' if its not already
        char* cvar_dev = (char*)CVAR_GET_STRING("developer");
        int dev;
        if(strcmp(cvar_dev, "0") == 0)
            dev = 0;
        else
            dev = 1;
        // tell the console all the bot vars
        if(dev == 0)
            CVAR_SET_STRING("developer", "1");
        CBaseEntity* p;
        p = CBaseEntity::Instance(INDEXENT(1));
        hudtextparms_t h;
        h.channel = 4;
        h.effect = 1;
        h.r1 = 10;
        h.g1 = 53;
        h.b1 = 81;
        h.a1 = 255;
        h.r2 = 10;
        h.g2 = 53;
        h.b2 = 81;
        h.a2 = 168;
        h.fadeinTime = 1;
        h.fadeoutTime = 1;
        h.holdTime = 5;
        h.x = 0;
        h.y = 0;
        sprintf(msg, "--FoxBot Loaded--\n--Visit 'www.apg-clan.org' for updates and info--\n");
        ALERT(at_console, msg);
        _snprintf(msg2, 511, msg);

        /*	sprintf(msg,"--* foxbot v%d.%d build# %d *--\n",
                        VER_MAJOR,VER_MINOR,VER_BUILD);*/

        sprintf(msg, "--* foxbot v%d.%d *--\n", VER_MAJOR, VER_MINOR);
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        sprintf(msg, "\n--FoxBot info--\n");
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // waypoints
        if(num_waypoints > 0)
            sprintf(msg, "Waypoints loaded\n");
        else
            sprintf(msg, "Waypoints NOT loaded\n--Warning, bots will not navigate correctly!--\n");
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // area file
        if(num_areas > 0)
            sprintf(msg, "Areas loaded\n");
        else
            sprintf(msg, "Areas not loaded\n");

        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // scripts...loaded/passed?
        if(script_loaded) {
            if(script_parsed)
                sprintf(msg, "Script loaded and parsed\n");
            else
                sprintf(msg,
                    "Script loaded and NOT parsed\n--Warning script file has an error in it and will NOT be used!--\n");
        } else
            sprintf(msg, "No script file loaded\n");
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // now bots vars
        sprintf(msg, "\n--FoxBot vars--\n");
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // bot skill levels
        sprintf(msg, "botskill_lower %d\nbotskill_upper %d\n", botskill_lower, botskill_upper);
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        sprintf(msg, "max_bots %d\nmin_bots %d\n", max_bots, min_bots);
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        // bot chat
        sprintf(msg, "Bot chat %d\n", botchat);
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        if(bot_team_balance)
            sprintf(msg, "Bot auto team balance On\n");
        else
            sprintf(msg, "Bot auto team balance Off\n");
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));

        if(bot_bot_balance)
            sprintf(msg, "Bot per team balance On\n");
        else
            sprintf(msg, "Bot per team balance Off\n");
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        sprintf(msg, "\n");
        ALERT(at_console, msg);
        strncat(msg2, msg, 511 - strlen(msg2));
        ALERT(at_logged, "[FOXBOT]: %s", msg2);
        ALERT(at_console, "\n\n\n");
        // clear it up if it was 0
        if(dev == 0)
            CVAR_SET_STRING("developer", "0");
        if(!display_bot_vars)
            UTIL_HudMessage(p, h, msg2);
        if(waypoint_author[0] != '\0') {
            h.channel = 2;
            h.effect = 1;
            h.r1 = 255;
            h.g1 = 128;
            h.b1 = 0;
            h.a1 = 255;
            h.r2 = 255;
            h.g2 = 170;
            h.b2 = 0;
            h.a2 = 255;
            h.fadeinTime = 1;
            h.fadeoutTime = 1;
            h.holdTime = 7;
            h.x = -1;
            h.y = 0.8;
            sprintf(msg, "-- Waypoint author: %s --", waypoint_author);
            UTIL_HudMessage(p, h, msg);
        }
    }
}

// This function can be used to process a specified bot setting command.
// It can handle bot options specified in a config file or from the
// command line or console.
// e.g. the max_bots setting.
static void changeBotSetting(const char* settingName,
    int* setting,
    const char* arg1,
    const int minValue,
    const int maxValue,
    const int settingSource)
{
    bool settingWasChanged = FALSE;

    char msg[128] = "";

    // if the setting change is from a config file set up a message about it
    char configMessage[] = "[Config] ";
    if(settingSource != SETTING_SOURCE_CONFIG_FILE)
        configMessage[0] = '\0';

    // if an argument was supplied change the setting with it
    if(arg1 != NULL && (*arg1 != 0)) {
        int temp = atoi(arg1);
        if(temp >= minValue && temp <= maxValue) {
            *setting = temp;
            settingWasChanged = TRUE;
        } else {
            // report that a bad setting was requested

            _snprintf(msg, 128, "%s%s should be set from %d to %d\n", configMessage, settingName, minValue, maxValue);
            msg[127] = '\0';

            if(settingSource == SETTING_SOURCE_CLIENT_COMMAND)
                ClientPrint(INDEXENT(1), HUD_PRINTNOTIFY, msg);
            else if(settingSource == SETTING_SOURCE_SERVER_COMMAND)
                printf(msg);
            else if(settingSource == SETTING_SOURCE_CONFIG_FILE) {
                if(IS_DEDICATED_SERVER())
                    printf(msg);
                else
                    ALERT(at_console, msg);
            }
        }
    }

    // report if the setting was actually changed or not
    if(settingWasChanged) {
        _snprintf(msg, 128, "%s%s has been set to %d\n", configMessage, settingName, *setting);
        msg[127] = '\0';
    } else {
        _snprintf(msg, 128, "%s%s is currently set to %d\n", configMessage, settingName, *setting);
        msg[127] = '\0';
    }

    if(settingSource == SETTING_SOURCE_CLIENT_COMMAND)
        ClientPrint(INDEXENT(1), HUD_PRINTNOTIFY, msg);
    else if(settingSource == SETTING_SOURCE_SERVER_COMMAND)
        printf(msg);
    else if(settingSource == SETTING_SOURCE_CONFIG_FILE) {
        if(IS_DEDICATED_SERVER())
            printf(msg);
        else
            ALERT(at_console, msg);
    }
}

// This a general purpose function for kicking bots.
// Set team to -1 if you don't want to specify a team.
// Set totalToKick to MAX_BOTS if you want to kick all of them.
static void kickBots(int totalToKick, const int team)
{
    // sanity check
    if(totalToKick < 1 || team > 3)
        return;

    if(totalToKick >= MAX_BOTS)
        totalToKick = MAX_BOTS - 1;

    for(int index = MAX_BOTS - 1; index >= 0 && totalToKick > 0; index--) {
        if(bots[index].is_used // is this slot used?
            && !FNullEnt(bots[index].pEdict) && (team < 0 || bots[index].bot_team == team)) {
            char cmd[80];

            _snprintf(cmd, 80, "kick \"%s\"\n", bots[index].name);
            cmd[79] = '\0';
            SERVER_COMMAND(cmd); // kick the bot using (kick "name")
            ClearKickedBotsData(index, TRUE);
            bots[index].is_used = FALSE;
            --totalToKick;
        }
    }
}

// This function will kick a random bot.
static void kickRandomBot(void)
{
    int bot_list[MAX_BOTS];
    int bot_total = 0;
    int index;

    // count and list the bots
    for(index = 0; index < MAX_BOTS; index++) {
        if(bots[index].is_used // is this slot used?
            && !FNullEnt(bots[index].pEdict)) {
            bot_list[bot_total] = index;
            ++bot_total;
        }
    }

    // sanity checking
    if(bot_total < 1 || bot_total >= MAX_BOTS)
        return;

    // kick a random bot from the list
    // double check we're kicking an active bot too
    index = bot_list[random_long(0, bot_total - 1)];
    if(index > -1 && index < MAX_BOTS && bots[index].is_used && !FNullEnt(bots[index].pEdict)) {
        char cmd[80];

        _snprintf(cmd, 80, "kick \"%s\"\n", bots[index].name);
        cmd[79] = '\0';
        SERVER_COMMAND(cmd); // kick the bot using (kick "name")
        ClearKickedBotsData(index, TRUE);
        bots[index].is_used = FALSE;
    }
}

// This function can be called when a bot has been kicked and will attempt
// to clear all the bots data that might be unsafe if used.  e.g. pointers.
// Set eraseBotsName to TRUE if you want the bots name to be forgotten too.
static void ClearKickedBotsData(const int botIndex, const bool eraseBotsName)
{
    bots[botIndex].current_wp = -1; // just for the hell of it

    // the unfeasibly scary pointers of great doom!
    bots[botIndex].enemy.ptr = NULL;
    bots[botIndex].lastEnemySentryGun = NULL;
    bots[botIndex].suspectedSpy = NULL;
    bots[botIndex].killer_edict = NULL;
    bots[botIndex].killed_edict = NULL;
    bots[botIndex].has_sentry = FALSE;
    bots[botIndex].sentry_edict = NULL;
    bots[botIndex].has_dispenser = FALSE;
    bots[botIndex].dispenser_edict = NULL;
    bots[botIndex].tpEntrance = NULL;
    bots[botIndex].tpExit = NULL;

    bots[botIndex].sentryWaypoint = -1;
    bots[botIndex].tpEntranceWP = -1;
    bots[botIndex].tpExitWP = -1;

    // clear the messaging data
    bots[botIndex].newmsg = FALSE;
    bots[botIndex].message[0] = '\0';
    bots[botIndex].msgstart[0] = '\0';

    // clear the bots knowledge of any teleporters
    for(int teleIndex = 0; teleIndex < MAX_BOT_TELEPORTER_MEMORY; teleIndex++) {
        BotForgetTeleportPair(&bots[botIndex], teleIndex);
    }

    // we don't like ya, so sod off!
    if(eraseBotsName) {
        botJustJoined[botIndex] = TRUE;
        bots[botIndex].name[0] = '\0';
        bots[botIndex].bot_team = -1; // no team chosen
    }
}

#if 0 // this function is not useful enough yet
// This function scans a maps waypoints for flag waypoints that belong to
// one team or another.  It then tries to find out which teams are trying
// to carry which teams flags.
static void flagTeamCheck(void)
{
	if(num_waypoints < 1)
		return;

	int flagTeam[4] = {-2, -2, -2, -2};

	// find the nearest waypoint with the matching flags...
	for(int index = 0; index < num_waypoints; index++)
	{
		if((waypoints[index].flags & W_FL_TFC_FLAG) != W_FL_TFC_FLAG)
			continue;  // skip this waypoint if the flags don't match

		// skip any deleted waypoints or aiming waypoints
		if(waypoints[index].flags & W_FL_DELETED
			|| waypoints[index].flags & W_FL_AIMING)
			continue;

		// only check team specific flag waypoints
		// (so as to avoid potential confusion)
		if(!(waypoints[index].flags & W_FL_TEAM_SPECIFIC))
			continue;

		// look for a flag very near to this flag waypoint
		edict_t *pent = NULL;
		while((pent = FIND_ENTITY_BY_CLASSNAME( pent, "item_tfgoal" )) != NULL
			&& !FNullEnt(pent))
		{
			if(!(pent->v.effects & EF_NODRAW)
				&& VectorsNearerThan(waypoints[index].origin, pent->v.origin, 80.0))
			{
				for(int j = 0; j < 4; j++)
				{
					if((waypoints[index].flags & W_FL_TEAM) == j)
					{
						flagTeam[j] = UTIL_GetFlagsTeam(pent);
						break;
					}
				}
			}
		}
	}

	// report which team is after which flags
//	for(int j = 0; j < 4; j++)
//	{
//		FILE *fp;
//		fp = UTIL_OpenFoxbotLog();
//		if(j == 0)
//			fprintf(fp,"flagcheck on %s\n", STRING(gpGlobals->mapname);
//		fprintf(fp,"Team %d want to carry team %d flags\n", j, flagTeam[j]);
//		fclose(fp);
//	}
}
#endif
