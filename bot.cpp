//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot.cpp
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
//#include "util.h"
#include "bot.h"
#include <meta_api.h>
#include "cbase.h"
#include "tf_defs.h"

#include "list.h"

#include "bot_func.h"
#include "waypoint.h"
#include "bot_navigate.h"
#include "bot_weapons.h"
#include "bot_job_think.h"

#include "math.h"
#include "engine.h" //originally commented

#include <sys/types.h>
#include <sys/stat.h>

//#include "list.h"
extern List<char*> commanders;

#ifndef __linux__
extern HINSTANCE h_Library;
#else
extern void* h_Library;
#endif

// my global var for checking if we're using metamod or not?
// i.e. should we be a plugin..
bool mr_meta = FALSE;

extern int mod_id;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints; // number of waypoints currently in use
extern edict_t* pent_info_ctfdetect;

static float roleCheckTimer = 30; // set fairly high so players can join first

struct TeamLayout {
    List<bot_t*> attackers[4];
    List<bot_t*> defenders[4];
    List<edict_t*> humanAttackers[4];
    List<edict_t*> humanDefenders[4];
    int total[4];
};

// team data /////////////////////////
extern int RoleStatus[];
extern int team_allies[4];
extern int max_team_players[4];
extern int team_class_limits[4];
extern int spawnAreaWP[4]; // used for tracking the areas where each team spawns
extern int max_teams;

extern bot_weapon_t weapon_defs[MAX_WEAPONS];

extern int flf_bug_fix;

extern int CheckTeleporterExitTime;

static FILE* fp;

int pipeCheckFrame = 15;
extern int debug_engine;

extern bool spawn_check_crash;
extern int spawn_check_crash_count;
extern edict_t* spawn_check_crash_edict;

// bot settings //////////////////  Fixed the unwanted 'const' and replaced 'bool' for 'int' on bot_allow_moods - Arkshine
extern int bot_allow_moods;
extern int botskill_lower;
extern int botskill_upper;
extern bool defensive_chatter;
extern bool offensive_chatter;
extern bool b_observer_mode;
extern bool b_botdontshoot;
extern bool botdontmove;

extern int botchat;
extern int min_bots;
extern int max_bots;
extern int bot_team_balance;
extern bool bot_can_use_teleporter;
extern bool bot_can_build_teleporter;
extern bool bot_xmas;
extern bool g_bot_debug;
extern int spectate_debug; // spectators can trigger debug messages from bots

extern edict_t* clients[32];

// area defs /////////////////
extern AREA areas[MAX_WAYPOINTS];
extern int num_areas;

bool attack[4]; // teams attack
bool defend[4]; // teams defend

// working arrays... these are what are referenced by the bots
// i.e. hold the current available state of points 1..8 for each team
bool blue_av[8];
bool red_av[8];
bool green_av[8];
bool yellow_av[8];

// pre defined msg type.. 64 empty msg command list
struct msg_com_struct msg_com[MSG_MAX];

// and 64 empty msg's, to be filled with messages to intercept
char msg_msg[64][MSG_MAX];

#define PLAYER_SEARCH_RADIUS 50.0
#define FLF_PLAYER_SEARCH_RADIUS 60.0

#define GETPLAYERAUTHID (*g_engfuncs.pfnGetPlayerAuthId)

bot_t bots[MAX_BOTS]; // max of 32 bots in a game

// this can allow us to track which bots are joining the game for the first time
// and which are joining because they were kicked off due to a map change
bool botJustJoined[MAX_BOTS] = { TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
    TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE };

extern chatClass chat; // bot chat stuff

static int number_names = 0;

#define MAX_BOT_NAMES 128

#define VALVE_MAX_SKINS 10
#define GEARBOX_MAX_SKINS 20

static char bot_names[MAX_BOT_NAMES][BOT_NAME_LEN + 1];

// defend response distance per class
const static float defendMaxRespondDist[] = { 300.0f, 1500.0f, 500.0f, 1500.0f, 800.0f, 1500.0f, 1500.0f, 1500.0f,
    1500.0f, 1500.0f };

const static double double_pi = 3.141592653589793238;

// FUNCTION PROTOTYPES /////////////////
static int BotAssignDefaultSkill(void);
static void BotFlagSpottedCheck(bot_t* pBot);
static void BotAmmoCheck(bot_t* pBot);
static void BotCombatThink(bot_t* pBot);
static void BotAttackerCheck(bot_t* pBot);
static void BotGrenadeAvoidance(bot_t* pBot);
static void BotRoleCheck(bot_t* pBot);
static bool botVerifyAccess(edict_t* pPlayer);
static void BotComms(bot_t* pBot);
static bool BotChangeRole(bot_t* pBot, const char* cmdLine, const char* from);
static bool BotChangeClass(bot_t* pBot, int iClass, const char* from);
static void BotPickNewClass(bot_t* pBot);
static bool BotChooseCounterClass(bot_t* pBot);
static bool BotDemomanNeededCheck(bot_t* pBot);
static int guessThreatLevel(bot_t* pBot);
static void BotReportMyFlagDrop(bot_t* pBot);
static void BotEnemyCarrierAlert(bot_t* pBot);
static void BotSenseEnvironment(bot_t* pBot);
static void BotFight(bot_t* pBot);
static void BotSpectatorDebug(bot_t* pBot);

inline edict_t* CREATE_FAKE_CLIENT(const char* netname)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "createfakeclient: %s\n", netname);
            fclose(fp);
        }
    }
    return (*g_engfuncs.pfnCreateFakeClient)(netname);
}

inline char* GET_INFOBUFFER(edict_t* e)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "getinfobuffer: %p\n", (void*)e);
            fclose(fp);
        }
    }
    return (*g_engfuncs.pfnGetInfoKeyBuffer)(e);
}

/* this function is unused so far
inline char *GET_INFO_KEY_VALUE( char *infobuffer, char *key )
{
        if(debug_engine)
        {
                fp = UTIL_OpenFoxbotLog();
                if(fp != NULL)
                {
                        fprintf(fp, "getinfokeyvalue: %s %s\n",infobuffer,key);
                        fclose(fp);
                }
        }
        return (*g_engfuncs.pfnInfoKeyValue)( infobuffer, key );
}*/

inline void SET_CLIENT_KEY_VALUE(int clientIndex, char* infobuffer, char* key, char* value)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnSetClientKeyValue: %s %s\n", key, value);
            fclose(fp);
        }
    }
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
}

// this is the LINK_ENTITY_TO_CLASS function that creates a player (bot)
void player(entvars_t* pev)
{
    static LINK_ENTITY_FUNC otherClassName = NULL;
    if(otherClassName == NULL) {
        otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(h_Library, "player");
    }
    if(otherClassName != NULL) {
        (*otherClassName)(pev);
    }
}

// This function initializes a bots data to safe values.
// i.e. it clears the bots "mind" before it spawns or respawns
void BotSpawnInit(bot_t* pBot)
{
    // v1.1.0.8 on xp..(or possibly fast machines)
    // had the bot crash if it tried to respawn too soon!
    // so put a pause in to stop it pressing 'fire' too soon...
    // this should fix it!!
    pBot->f_killed_time = gpGlobals->time;
    // but it appears to be a bug in valves code, even without bots..
    // so not actually needed

    pBot->curr_wp_diff = -1;
    pBot->f_progressToWaypoint = 4000.0;
    pBot->f_navProblemStartTime = 0.0;
    pBot->routeFailureTally = 0;

    pBot->f_current_wp_deadline = 0.0;
    pBot->current_wp = -1;

    pBot->msecnum = 0;
    pBot->msecdel = 0.0;
    pBot->msecval = 0.0;

    pBot->bot_real_health = 0;
    pBot->bot_armor = 0;
    pBot->bot_weapons = 0;
    pBot->f_blinded_time = 0.0;

    pBot->f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");
    pBot->prev_speed = 0.0; // fake "paused" since bot is NOT stuck

    pBot->job[pBot->currentJob].phase = 0;

    pBot->f_find_item_time = 0.0;

    pBot->strafe_mod = STRAFE_MOD_NORMAL;

    // enemy stuff
    pBot->enemy.ptr = NULL;
    pBot->enemy.seenWithFlag = 0;
    pBot->enemy.f_firstSeen = gpGlobals->time - 1000.0;
    pBot->enemy.f_lastSeen = pBot->enemy.f_firstSeen;
    pBot->enemy.f_seenDistance = 400.0f;
    pBot->enemy.lastLocation = Vector(0.0, 0.0, 0.0);

    pBot->visEnemyCount = 0;
    pBot->visAllyCount = 1; // 1 = the bot itself
    pBot->lastAllyVector = Vector(0.0, 0.0, 0.0);
    pBot->f_lastAllySeenTime = 0.0;
    pBot->f_alliedMedicSeenTime = 0.0;

    pBot->f_view_change_time = 0.0;

    pBot->f_shoot_time = gpGlobals->time;
    pBot->f_primary_charging = -1.0f;
    pBot->f_secondary_charging = -1.0f;
    pBot->charging_weapon_id = 0;
    pBot->tossNade = 0;
    pBot->aimDrift = Vector(0.0, 0.0, 0.0);

    pBot->f_pause_time = 0.0;
    pBot->f_duck_time = 0.0;

    pBot->bot_has_flag = FALSE;

    pBot->scoreAtSpawn = static_cast<int>(pBot->pEdict->v.frags);

    pBot->b_use_health_station = FALSE;
    pBot->f_use_health_time = 0.0;
    pBot->b_use_HEV_station = FALSE;
    pBot->f_use_HEV_time = 0.0;

    pBot->f_use_button_time = 0.0;

    pBot->f_waypoint_drift = 0;
    pBot->goto_wp = -1;
    pBot->lastgoto_wp = -1;
    pBot->f_dontEvadeTime = 0.0;

    pBot->f_enemy_check_time = gpGlobals->time;
    pBot->f_item_check_time = gpGlobals->time;
    pBot->flag_impulse = -1;
    pBot->f_soundReactTime = gpGlobals->time + 3.0f;

    pBot->disturbedViewAmount = 0;
    pBot->f_disturbedViewTime = gpGlobals->time;
    pBot->f_disturbedViewYaw = 0.0;
    pBot->f_disturbedViewPitch = 0.0;

    pBot->f_periodicAlertFifth = gpGlobals->time + 0.2f;
    pBot->f_periodicAlert1 = gpGlobals->time + 1.0f;
    pBot->f_periodicAlert3 = gpGlobals->time + 3.0f;

    pBot->f_snipe_time = 0.0;
    pBot->f_disguise_time = gpGlobals->time;
    pBot->f_feigningTime = 0.0;
    pBot->disguise_state = DISGUISE_NONE;
    pBot->f_spyFeignAmbushTime = gpGlobals->time + static_cast<float>(random_long(20, 30));

    pBot->f_side_route_time = gpGlobals->time + RANDOM_FLOAT(0.0, 60.0);

    // the bots true ammo status can be checked upon later
    pBot->ammoStatus = AMMO_WANTED;

    /////////////////
    // DrEvil vars //
    /////////////////

    pBot->last_spawn_time = gpGlobals->time;
    pBot->nadeType = 0;
    pBot->nadePrimed = FALSE;
    pBot->primeTime = pBot->lastDistance = 0.0f;
    pBot->lastDistanceCheck = pBot->FreezeDelay = 0.0f;
    pBot->f_shortcutCheckTime = gpGlobals->time + 2.0f;

    pBot->f_roleSayDelay = gpGlobals->time + 10.0f;
    pBot->f_discard_time = gpGlobals->time + RANDOM_FLOAT(8.0f, 16.0f);

    pBot->f_grenadeScanTime = gpGlobals->time + 0.7f;

    pBot->branch_waypoint = -1;

    /////////////////////
    // end DrEvil vars //
    /////////////////////

    memset(&(pBot->current_weapon), 0, sizeof(pBot->current_weapon));
    memset(&(pBot->m_rgAmmo), 0, sizeof(pBot->m_rgAmmo));
}

void BotNameInit(void)
{
    FILE* bot_name_fp;
    char bot_name_filename[256];
    char name_buffer[80];

    UTIL_BuildFileName(bot_name_filename, 255, "foxbot_names.txt", NULL);
    bot_name_fp = fopen(bot_name_filename, "r");

    if(bot_name_fp != NULL) {
        int length, index, str_index;
        while((number_names < MAX_BOT_NAMES) && (fgets(name_buffer, 80, bot_name_fp) != NULL)) {
            length = strlen(name_buffer);

            // remove '\n'
            if(name_buffer[length - 1] == '\n') {
                name_buffer[length - 1] = '\0';
                length--;
            }

            str_index = 0;
            while(str_index < length) {
                if((name_buffer[str_index] < ' ') || (name_buffer[str_index] > '~') ||
                    (name_buffer[str_index] == '"')) {
                    for(index = str_index; index < length; index++)
                        name_buffer[index] = name_buffer[index + 1];
                }

                str_index++;
            }

            if(name_buffer[0] != 0) {
                strncpy(bot_names[number_names], name_buffer, BOT_NAME_LEN);
                number_names++;
            }
        }

        fclose(bot_name_fp);
    }
}

// This simple function will return a valid, randomly selected bot skill
// in the range from botskill_lower to botskill_upper.
static int BotAssignDefaultSkill(void)
{
    if(botskill_lower <= botskill_upper)
        return botskill_lower;
    else
        return static_cast<int>(random_long(botskill_upper, botskill_lower));
}

void BotPickName(char* name_buffer)
{
    int index;

    // see if a name exists from a kicked bot (if so, reuse it)
    for(index = 0; index < MAX_BOTS; index++) {
        if(bots[index].is_used == FALSE && bots[index].name[0]) {
            strcpy(name_buffer, bots[index].name);

            // erase the name of the bot whose name we've taken
            // this fixes the bug where two bots would be created with the same name
            // (sometimes a lower indexed bot would copy a higher indexed bots name)
            bots[index].name[0] = 0;

            return;
        }
    }

    // it's time to pick a new name at random
    int name_index = random_long(1, number_names) - 1; // zero based

    // make sure this name isn't used already
    edict_t* pPlayer;
    bool used = TRUE;
    int attempts = 0;
    while(used) {
        used = FALSE;

        // is there a player with this name?
        for(index = 1; index <= gpGlobals->maxClients; index++) {
            pPlayer = INDEXENT(index);

            if(pPlayer && !FNullEnt(pPlayer) && !pPlayer->free &&
                strcmp(bot_names[name_index], STRING(pPlayer->v.netname)) == 0) {
                used = TRUE;
                break;
            }
        }

        if(used) {
            ++name_index;

            if(name_index >= number_names) {
                name_index = 0;
                ++attempts;
            }

            if(attempts >= 2) {
                used = FALSE; // break out of loop even if already used

                // Log this event as it normally shouldn't happen
                fp = UTIL_OpenFoxbotLog();
                if(fp != NULL) {
                    fprintf(fp, "Ran out of unique bot names to assign.\n");
                    fclose(fp);
                }
            }
        }
    }

    strcpy(name_buffer, bot_names[name_index]);
}

void BotCreate(edict_t* pPlayer, const char* arg1, const char* arg2, const char* arg3, const char* arg4)
{
    // indicate which models are currently used for random model allocation
    static bool valve_skin_used[VALVE_MAX_SKINS] = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE };

    /*static bool gearbox_skin_used[GEARBOX_MAX_SKINS] = {
            FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
            FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE
    };*/

    // store the names of the models...
    static const char* valve_bot_skins[VALVE_MAX_SKINS] = { "barney", "gina", "gman", "gordon", "helmet", "hgrunt",
        "recon", "robo", "scientist", "zombie" };

    /*static const char *gearbox_bot_skins[GEARBOX_MAX_SKINS] = {
            "barney", "beret", "cl_suit", "drill", "fassn", "gina", "gman",
            "gordon", "grunt", "helmet", "hgrunt", "massn", "otis", "recon",
            "recruit", "robo", "scientist", "shepard", "tower", "zombie"
    };*/

    // store the player names for each of the models...
    static const char* valve_bot_names[VALVE_MAX_SKINS] = { "Barney", "Gina", "G-Man", "Gordon", "Helmet", "H-Grunt",
        "Recon", "Robo", "Scientist", "Zombie" };

    /*static const char *gearbox_bot_names[GEARBOX_MAX_SKINS] = {
            "Barney", "Beret", "Cl_suit", "Drill", "Fassn", "Gina", "G-Man",
            "Gordon", "Grunt", "Helmet", "H-Grunt", "Massn", "Otis", "Recon",
            "Recruit", "Robo", "Scientist", "Shepard", "Tower", "Zombie"
    };*/

    edict_t* BotEnt;
    bot_t* pBot;
    char c_skin[BOT_SKIN_LEN + 1];
    char c_name[BOT_NAME_LEN + 1];
    c_skin[0] = '\0';
    c_name[0] = '\0';
    int skill;
    int index;
    int i, j, length;
    bool found = FALSE;

    // min/max checking...
    if(max_bots > MAX_BOTS)
        max_bots = MAX_BOTS;
    if(max_bots < -1)
        max_bots = -1;
    if(min_bots > max_bots)
        min_bots = max_bots;
    if(min_bots < -1)
        min_bots = -1;

    // count the number of players present
    int count = 0;
    for(i = 1; i <= 32; i++) //<32
    {
        char* infobuffer;
        char cl_name[128];
        cl_name[0] = '\0';
        infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(INDEXENT(i));
        strcpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
        if(cl_name[0] != '\0')
            count++;
    }

    // count the number of bots present
    int bot_count = 0;
    for(i = 0; i < MAX_BOTS; i++) {
        if(bots[i].is_used)
            bot_count++;
    }

    if(bot_count >= max_bots && bot_count >= min_bots && max_bots != -1) {
        // don't create another bot..
        if(pPlayer)
            ClientPrint(pPlayer, HUD_PRINTNOTIFY, "Max bots reached... Can't create bot!\n");

        if(IS_DEDICATED_SERVER())
            printf("Max bots reached... Can't create bot!\n");

        return;
    }

    if((mod_id == VALVE_DLL)) {
        int max_skin_index;

        if(mod_id == VALVE_DLL)
            max_skin_index = VALVE_MAX_SKINS;
        // else  // must be GEARBOX_DLL
        //	max_skin_index = GEARBOX_MAX_SKINS;

        if((arg1 == NULL) || (*arg1 == 0)) {
            bool* pSkinUsed;

            // pick a random skin
            if(mod_id == VALVE_DLL) {
                index = random_long(0, VALVE_MAX_SKINS - 1);
                pSkinUsed = &valve_skin_used[0];
            }
            /*else  // must be GEARBOX_DLL
            {
                    index = random_long(0, GEARBOX_MAX_SKINS - 1);
                    pSkinUsed = &gearbox_skin_used[0];
            }*/

            // check if this skin has already been used...
            while(pSkinUsed[index] == TRUE) {
                index++;

                if(index == max_skin_index)
                    index = 0;
            }

            pSkinUsed[index] = TRUE;

            // check if all skins are now used...
            for(i = 0; i < max_skin_index; i++) {
                if(pSkinUsed[i] == FALSE)
                    break;
            }

            // if all skins are used, reset used to FALSE for next selection
            if(i == max_skin_index) {
                for(i = 0; i < max_skin_index; i++)
                    pSkinUsed[i] = FALSE;
            }

            if(mod_id == VALVE_DLL)
                strcpy(c_skin, valve_bot_skins[index]);
            // else // must be GEARBOX_DLL
            //	strcpy( c_skin, gearbox_bot_skins[index] );
        } else {
            strncpy(c_skin, arg1, BOT_SKIN_LEN - 1);
            c_skin[BOT_SKIN_LEN] = 0; // make sure c_skin is null terminated
        }

        for(i = 0; c_skin[i] != 0; i++)
            c_skin[i] = tolower(c_skin[i]); // convert to all lowercase

        index = 0;

        while((!found) && (index < max_skin_index)) {
            if(mod_id == VALVE_DLL) {
                if(strcmp(c_skin, valve_bot_skins[index]) == 0)
                    found = TRUE;
                else
                    index++;
            }
            /*else // must be GEARBOX_DLL
            {
                    if(strcmp(c_skin, gearbox_bot_skins[index]) == 0)
                            found = TRUE;
                    else index++;
            }*/
        }

        if(found == TRUE) {
            if((arg2 != NULL) && (*arg2 != 0)) {
                strncpy(c_name, arg2, BOT_SKIN_LEN - 1);
                c_name[BOT_SKIN_LEN] = 0; // make sure c_name is null terminated
            } else {
                if(number_names > 0)
                    BotPickName(c_name);
                else if(mod_id == VALVE_DLL)
                    strcpy(c_name, valve_bot_names[index]);
                // else // must be GEARBOX_DLL
                //	strcpy( c_name, gearbox_bot_names[index] );
            }
        } else {
            char dir_name[128];
            char filename[128];

            struct stat stat_str;

            GET_GAME_DIR(dir_name);

#ifndef __linux__
            _snprintf(filename, 127, "%s\\models\\player\\%s", dir_name, c_skin);
#else
            _snprintf(filename, 127, "%s/models/player/%s", dir_name, c_skin);
#endif

            if(stat(filename, &stat_str) != 0) {
#ifndef __linux__
                _snprintf(filename, 127, "valve\\models\\player\\%s", c_skin);
#else
                _snprintf(filename, 127, "valve/models/player/%s", c_skin);
#endif
                if(stat(filename, &stat_str) != 0) {
                    char err_msg[80];

                    _snprintf(err_msg, 79, "model \"%s\" is unknown.\n", c_skin);
                    if(pPlayer)
                        ClientPrint(pPlayer, HUD_PRINTNOTIFY, err_msg);
                    if(IS_DEDICATED_SERVER())
                        printf(err_msg);

                    if(pPlayer)
                        ClientPrint(pPlayer, HUD_PRINTNOTIFY, "use barney, gina, gman, gordon, helmet, hgrunt,\n");
                    if(IS_DEDICATED_SERVER())
                        printf("use barney, gina, gman, gordon, helmet, hgrunt,\n");
                    if(pPlayer)
                        ClientPrint(pPlayer, HUD_PRINTNOTIFY, "    recon, robo, scientist, or zombie\n");
                    if(IS_DEDICATED_SERVER())
                        printf("    recon, robo, scientist, or zombie\n");
                    return;
                }
            }

            if((arg2 != NULL) && (*arg2 != 0)) {
                strncpy(c_name, arg2, BOT_NAME_LEN - 1);
                c_name[BOT_NAME_LEN] = 0; // make sure c_name is null terminated
            } else {
                if(number_names > 0)
                    BotPickName(c_name);
                else {
                    // copy the name of the model to the bot's name...
                    strncpy(c_name, arg1, BOT_NAME_LEN - 1);
                    c_name[BOT_NAME_LEN] = '\0'; // make sure c_skin is null terminated
                }
            }
        }

        skill = 0;

        if((arg3 != NULL) && (*arg3 != 0))
            skill = atoi(arg3);

        if((skill < 1) || (skill > 5))
            skill = BotAssignDefaultSkill();
    } else {
        if((arg3 != NULL) && (*arg3 != 0)) {
            strncpy(c_name, arg3, BOT_NAME_LEN - 1);
            c_name[BOT_NAME_LEN] = '\0'; // make sure c_name is null terminated
        } else {
            if(number_names > 0)
                BotPickName(c_name);
            else
                strcpy(c_name, "FoxBot");
        }

        skill = 0;

        if((arg4 != NULL) && (*arg4 != 0))
            skill = atoi(arg4);

        if((skill < 1) || (skill > 5))
            skill = BotAssignDefaultSkill();
    }

    length = strlen(c_name);

    // remove any illegal characters from the name
    for(i = 0; i < length; i++) {
        if((c_name[i] < ' ') || (c_name[i] > '~')) {
            // shuffle the remaining chars left (and null)
            for(j = i; j < length; j++)
                c_name[j] = c_name[j + 1];
            --length;
        }
    }

    BotEnt = CREATE_FAKE_CLIENT(c_name);

    if(FNullEnt(BotEnt)) {
        if(pPlayer)
            ClientPrint(pPlayer, HUD_PRINTNOTIFY, "Max. Players reached.  Can't create bot!\n");

        if(IS_DEDICATED_SERVER())
            printf("Max. Players reached.  Can't create bot!\n");

        return;
    } else {
        char ptr[256]; // allocate space for message from ClientConnect
        char* infobuffer;
        int clientIndex;

        int index = 0;
        while((bots[index].is_used) && (index < MAX_BOTS))
            ++index;

        if(index >= MAX_BOTS) {
            ClientPrint(pPlayer, HUD_PRINTNOTIFY, "Can't create bot(maximum bots reached).\n");
            return;
        }

        if(IS_DEDICATED_SERVER())
            printf("Creating bot...\n");
        else if(pPlayer)
            ClientPrint(pPlayer, HUD_PRINTNOTIFY, "Creating bot...\n");

        // clear shit up for clientprintf and clientcommand checking
        // (for Admin Mod)only helps if we've just created a bot
        // probably never used
        i = 0;
        while((i < 32) && (clients[i] != BotEnt))
            i++;

        if(i < 32)
            clients[i] = NULL;
        // why put these here?
        // well admin mod plugin may sneak in as their connecting
        // and before we can add this shit for checking purposes
        pBot = &bots[index];
        if(botJustJoined[index])
            memset(pBot, 0, sizeof(bot_t)); // wipe out the bots data
        pBot->pEdict = BotEnt;

        // clear the player info from the previous player who used this edict
        // (a fix by Pierre-Marie, found in the HPB forum)
        if(BotEnt->pvPrivateData != NULL) {
            FREE_PRIVATE(pBot->pEdict);
            pBot->pEdict->pvPrivateData = NULL;
        }
        pBot->pEdict->v.frags = 0;

        player(VARS(BotEnt)); // redundant?

        infobuffer = GET_INFOBUFFER(BotEnt);
        // infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(BotEnt);
        clientIndex = ENTINDEX(BotEnt);
        // clientIndex = g_engfuncs.pfnIndexOfEdict(BotEnt);
        // else clientIndex = gpGamedllFuncs->dllapi_table->pfnIndexOfEdict(BotEnt);

        if((mod_id == VALVE_DLL))
            SET_CLIENT_KEY_VALUE(clientIndex, infobuffer, "model", c_skin);
        else // other mods
            SET_CLIENT_KEY_VALUE(clientIndex, infobuffer, "model", "gina");

        /*if(mod_id == CSTRIKE_DLL)
        {
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "rate", "3500.000000");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_updaterate", "20");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_lw", "1");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_lc", "1");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "tracker", "0");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_dlmax", "128");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "lefthand", "1");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "friends", "0");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "dm", "0");
                SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "ah", "1");
        }*/

        //{fp=UTIL_OpenFoxbotLog(); fprintf(fp, "-bot connect %x\n",BotEnt); fclose(fp); }
        if(mr_meta) {
            MDLL_ClientConnect(BotEnt, c_name, "127.0.0.1", ptr);
            spawn_check_crash = TRUE;
            spawn_check_crash_count = 0;
            spawn_check_crash_edict = BotEnt;
            MDLL_ClientPutInServer(BotEnt);
            spawn_check_crash = FALSE;
            spawn_check_crash_edict = NULL;
            /*i = 0;
               while((i < 32) && (clients[i] != NULL))
               i++;

               if(i < 32)
               clients[i] = BotEnt;*/
        } else {
            ClientConnect(BotEnt, c_name, "127.0.0.1", ptr);
            spawn_check_crash = TRUE;
            spawn_check_crash_count = 0;
            spawn_check_crash_edict = BotEnt;
            ClientPutInServer(BotEnt); //<- this is the important one..
            spawn_check_crash = FALSE;
            spawn_check_crash_edict = NULL;
        }

        //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"e\n"); fclose(fp); }
        BotEnt->v.flags |= FL_FAKECLIENT;

        pBot->is_used = TRUE;
        pBot->respawn_state = RESPAWN_IDLE;
        pBot->create_time = (gpGlobals->time + 1.0);
        //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"a%f %f\n",pBot->create_time,gpGlobals->time); fclose(fp); }
        pBot->name[0] = 0; // name not set by server yet
        pBot->f_think_time = gpGlobals->time;
        strcpy(pBot->skin, c_skin);
        BotResetJobBuffer(pBot);

        pBot->has_sentry = FALSE;
        pBot->sentry_edict = NULL;
        pBot->sentryWaypoint = -1;
        pBot->SGRotated = FALSE;
        pBot->sentry_ammo = 0;
        pBot->has_dispenser = FALSE;
        pBot->dispenser_edict = NULL;
        pBot->f_dispenserDetTime = 0.0;
        pBot->killer_edict = NULL;
        pBot->killed_edict = NULL;
        pBot->lastEnemySentryGun = NULL;
        pBot->mission = ROLE_ATTACKER;
        pBot->lockMission = FALSE;
        pBot->tpEntrance = NULL;
        pBot->tpExit = NULL;
        pBot->tpEntranceWP = -1;
        pBot->tpExitWP = -1;
        pBot->f_safeWeaponTime = gpGlobals->time;

        // clear the bots knowledge of any teleporters
        for(int teleIndex = 0; teleIndex < MAX_BOT_TELEPORTER_MEMORY; teleIndex++) {
            BotForgetTeleportPair(pBot, teleIndex);
        }

        roleCheckTimer += 4.0;

        // stop the bot from autonomously changing class if
        // a class was specified
        if(arg2 != NULL && mod_id == TFC_DLL)
            pBot->lockClass = TRUE;
        else
            pBot->lockClass = FALSE;

        pBot->f_suspectSpyTime = 0.0;
        pBot->suspectedSpy = NULL;

        pBot->lastFrameHealth = 70;
        pBot->f_injured_time = 0.0f;
        pBot->deathsTillClassChange = 4; //(int)random_long(4, 15);

        // time to set up the bots personality stuff
        if(bot_allow_moods) {
            pBot->trait.aggression = random_long(1, 100);
            pBot->trait.fairplay = random_long(1, 1000);
            pBot->trait.faveClass = random_long(1, 9);
            pBot->trait.health = (random_long(0, 3) * 20) + 20;
            pBot->trait.humour = random_long(0, 100);

            if(random_long(1, 30) > 10)
                pBot->trait.camper = TRUE;
            else
                pBot->trait.camper = FALSE;
        } else // make each bot have the same personality traits
        {
            pBot->trait.aggression = 50;
            pBot->trait.fairplay = 600;
            pBot->trait.faveClass = 3;
            pBot->trait.health = 50;
            pBot->trait.humour = 50;
            pBot->trait.camper = TRUE;
        }

        pBot->f_humour_time = pBot->f_think_time + random_float(60.0, 180.0);

        pBot->sideRouteTolerance = 400; // not willing to branch far initially

        pBot->not_started = TRUE; // hasn't joined game yet
        pBot->bot_start2 = 1;
        pBot->bot_start3 = 0;

        pBot->bot_class = -1;   // not decided yet
        pBot->bot_team = -1;    // not decided yet
        pBot->current_team = 0; // sane value, will be set properly once bot has spawned

        if(botJustJoined[index]) {
            pBot->greeting = FALSE; // new bots can always say "hello!"
            pBot->newmsg = FALSE;
            pBot->message[0] = '\0';
            pBot->msgstart[0] = '\0';
        }

        if(mod_id == TFC_DLL)
            pBot->start_action = MSG_TFC_IDLE;
        /*else if(mod_id == CSTRIKE_DLL)
                pBot->start_action = MSG_CS_IDLE;
        else if((mod_id == GEARBOX_DLL) && (pent_info_ctfdetect != NULL))
                pBot->start_action = MSG_OPFOR_IDLE;
        else if(mod_id == FRONTLINE_DLL)
                pBot->start_action = MSG_FLF_IDLE;
        else pBot->start_action = 0;  // not needed for non-team MODs
        */
        //{fp=UTIL_OpenFoxbotLog(); fprintf(fp, "-bot d \n",BotEnt); fclose(fp); }
        BotSpawnInit(pBot);

        pBot->need_to_initialize = FALSE; // don't need to initialize yet

        BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
        BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;
        BotEnt->v.pitch_speed = BOT_PITCH_SPEED;
        BotEnt->v.yaw_speed = BOT_YAW_SPEED;

        pBot->idle_angle = 0.0;
        pBot->idle_angle_time = 0.0;

        //	pBot->lastLiftOrigin = Vector(0.0, 0.0, 0.0);

        pBot->bot_skill = skill - 1; // 0 based for array indexes

        if(mod_id == TFC_DLL) {
            if(arg1 != NULL && arg1[0] != 0) {
                pBot->bot_team = atoi(arg1);

                if(arg2 != NULL && arg2[0] != 0) {
                    pBot->bot_class = atoi(arg2);
                }
            }
        }

        botJustJoined[index] = FALSE; // the inititation ceremony is over!
    }
}

// This function is responsible for getting bots to spot nearby objects of
// interest and collect or use them.
void BotFindItem(bot_t* pBot)
{
    if(pBot->f_find_item_time >= pBot->f_think_time)
        return;

    // only look for items every 0.5 seconds
    pBot->f_item_check_time = pBot->f_think_time + 0.5f;

    // check if the bot can see a flag
    BotFlagSpottedCheck(pBot);

    edict_t* pPickupEntity = NULL;
    Vector pickup_origin;
    Vector entity_origin;
    bool can_pickup;
    float min_distance;
    char item_name[40];
    TraceResult tr;
    Vector vecStart;
    Vector vecEnd;
    int angle_to_entity;
    edict_t* pEdict = pBot->pEdict;
    job_struct* newJob;

    // use a MUCH smaller search radius when waypoints are available
    float radius = 400.0f;
    if(num_waypoints < 1 || pBot->current_wp == -1)
        radius = 600.0f;

    // put the search sphere in front of the bot
    UTIL_MakeVectors(pBot->pEdict->v.v_angle);
    Vector searchCenter = pBot->pEdict->v.origin + gpGlobals->v_forward * 200;

    // lower the search sphere, closer to the level of the bots feet
    searchCenter.z -= 20;

    min_distance = radius + 0.1f;

    edict_t* pent = NULL;
    while((pent = FIND_ENTITY_IN_SPHERE(pent, searchCenter, radius)) != NULL && (!FNullEnt(pent))) {
        can_pickup = FALSE; // assume can't use it until known otherwise

        strcpy(item_name, STRING(pent->v.classname));

        // see if this is a "func_" type of entity (func_button, etc.)...
        if(strncmp("func_", item_name, 5) == 0) {
            // BModels have 0,0,0 for origin so must use VecBModelOrigin...
            entity_origin = VecBModelOrigin(pent);

            vecStart = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
            vecEnd = entity_origin;

            angle_to_entity = BotInFieldOfView(pBot, vecEnd - vecStart);

            // check if entity is outside field of view (+/- 45 degrees)
            if(angle_to_entity > 45)
                continue; // skip this item if bot can't "see" it

            UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

            // can we see the item?  if not skip it
            if(tr.flFraction < 1.0 && tr.pHit != pent)
                continue;

            // check if the entity is a button...
            if(strcmp("func_button", item_name) == 0 || strcmp("func_rot_button", item_name) == 0) {
                newJob = InitialiseNewJob(pBot, JOB_PUSH_BUTTON);
                if(newJob != NULL && pent->v.target != 0 &&
                    VectorsNearerThan(pBot->pEdict->v.origin, entity_origin, 250.0)) {
                    // ignore this button if the nearest waypoint to it is unavailable
                    const int nearestWP = WaypointFindNearest_V(entity_origin, 80.0, -1);
                    if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                        continue;

                    // set up a job for pushing the button
                    newJob->object = pent;
                    SubmitNewJob(pBot, JOB_PUSH_BUTTON, newJob);
                    continue;
                }
            } else {
                // trace a line from bot's eyes to func_ entity...
                UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

                // check if traced all the way up to the entity (didn't hit wall)
                if(strcmp(item_name, STRING(tr.pHit->v.classname)) == 0) {
                    // find distance to item for later use...
                    float distance = (vecEnd - vecStart).Length();

                    // check if entity is wall mounted health charger...
                    if(strcmp("func_healthcharger", item_name) == 0) {
                        // check if the bot can use this item and
                        // check if the recharger is ready to use (has power left)...
                        if((pEdict->v.health < 100) && (pent->v.frame == 0)) {
                            // check if flag not set...
                            if(!pBot->b_use_health_station) {
                                // check if close enough and facing it directly...
                                if((distance < PLAYER_SEARCH_RADIUS) && (angle_to_entity <= 10)) {
                                    pBot->b_use_health_station = TRUE;
                                    pBot->f_use_health_time = pBot->f_think_time;
                                }

                                can_pickup = TRUE;
                            }
                        } else {
                            // don't need or can't use this item...
                            pBot->b_use_health_station = FALSE;
                        }
                    }

                    // check if entity is wall mounted HEV charger...
                    else if(strcmp("func_recharge", item_name) == 0) {
                        // check if the bot can use this item and
                        // check if the recharger is ready to use (has power left)...
                        if((pEdict->v.armorvalue < VALVE_MAX_NORMAL_BATTERY) && (pent->v.frame == 0)) {
                            // check if flag not set and facing it...
                            if(!pBot->b_use_HEV_station) {
                                // check if close enough and facing it directly...
                                if((distance < PLAYER_SEARCH_RADIUS) && (angle_to_entity <= 10)) {
                                    pBot->b_use_HEV_station = TRUE;
                                    pBot->f_use_HEV_time = pBot->f_think_time;
                                }

                                can_pickup = TRUE;
                            }
                        } else {
                            // don't need or can't use this item...
                            pBot->b_use_HEV_station = FALSE;
                        }
                    }

                    else if(strcmp("func_breakable", item_name) == 0) {
                        // make sure it really is breakable
                        if(pent->v.takedamage <= 0)
                            continue;

                        // check if the item is not visible
                        // (i.e. has not respawned)
                        if(pent->v.effects & EF_NODRAW)
                            continue;

                        // medics have no bashing weapon so make sure they have ammo for this
                        if(pEdict->v.playerclass == TFC_CLASS_MEDIC && pBot->ammoStatus == AMMO_LOW)
                            continue;

                        newJob = InitialiseNewJob(pBot, JOB_ATTACK_BREAKABLE);
                        if(newJob != NULL) {
                            newJob->object = pent;
                            if(SubmitNewJob(pBot, JOB_ATTACK_BREAKABLE, newJob) == TRUE)
                                return;
                        }

                        //	UTIL_HostSay(pBot->pEdict, 0, "breakable found"); //DebugMessageOfDoom!
                        continue;
                    }
                }
            }
        } else // everything else...
        {
            entity_origin = pent->v.origin;

            vecStart = pEdict->v.origin + pEdict->v.view_ofs;
            vecEnd = entity_origin;

            // find angles from bot origin to entity...
            angle_to_entity = BotInFieldOfView(pBot, vecEnd - vecStart);

            // check if entity is outside field of view (+/- 45 degrees)
            if(angle_to_entity > 45)
                continue; // skip this item if bot can't "see" it

            // check if line of sight to object is not blocked (i.e. visible)
            if(BotCanSeeOrigin(pBot, vecEnd)) {
                // check if entity is a back pack dropped by a player
                if(strcmp("tf_ammobox", item_name) == 0) {
                    if(pBot->ammoStatus != AMMO_UNNEEDED && pent->v.owner == NULL)
                        can_pickup = TRUE;
                }
                // check if entity is a back pack
                else if(strcmp("item_tfgoal", item_name) == 0) {
                    if(pBot->ammoStatus == AMMO_UNNEEDED || pent->v.owner != NULL || pent->v.model == 0)
                        continue;

                    // check if the item is not visible
                    // (i.e. has not respawned)
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    char mdlname[32] = "";
                    char temp[255];
                    for(int i = 0; i < 255; i++) {
                        temp[i] = '\0';
                    }
                    strcpy(temp, STRING(pent->v.model));

                    if(strlen(temp) >= 8)
                        strncpy(mdlname, temp + ((int)strlen(temp) - 8), 4);
                    else
                        continue;

                    // ignore the item if it's not a backpack
                    if(strcmp(mdlname, "pack") != 0)
                        continue;

                    // ignore this item if it's on an unavailable waypoint
                    const int nearestWP = WaypointFindNearest_V(pent->v.origin, 80.0, -1);
                    if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                        continue;

                    can_pickup = TRUE;
                }

                // check if entity is a back pack
                else if(strcmp("info_tfgoal", item_name) == 0) {
                    if(pBot->ammoStatus == AMMO_UNNEEDED || pent->v.owner != NULL || pent->v.model == 0)
                        continue;

                    // check if the item is not visible (i.e. has not respawned)
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    char temp[64];
                    char mdlname[64] = "\0";

                    strncpy(temp, STRING(pent->v.model), 64);
                    temp[63] = '\0';

                    // cut pent->v.model down in size from:
                    // "models/backpack.mdl"
                    if(strlen(temp) >= 8) {
                        strncpy(mdlname, temp + ((int)strlen(temp) - 8), 4);
                        mdlname[63] = '\0';
                    }

                    // ignore the item if it's not a backpack
                    if(strncmp(mdlname, "pack", 4) != 0)
                        continue;

                    // ignore this item if it's on an unavailable waypoint
                    const int nearestWP = WaypointFindNearest_V(pent->v.origin, 80.0, -1);
                    if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                        continue;

                    can_pickup = TRUE;
                }

                // check if entity is a dispenser
                /*		else if(strcmp("building_dispenser", item_name) == 0)
                                {
                                        // check if the item is not visible (i.e. has not respawned)
                                        if(pent->v.effects & EF_NODRAW)
                                                continue;

                                        if(pBot->ammoStatus == AMMO_UNNEEDED
                                                && pBot->bot_has_flag == FALSE)
                                        {
                                                // make engineers attend to their own dispensers
                                                if(pEdict->v.playerclass == TFC_CLASS_ENGINEER)
                                                {
                                                        if(pBot->dispenser_edict != pent)
                                                                continue;
                                                        else
                                                        {
                                                                UTIL_SelectItem(pEdict, "tf_weapon_spanner");
                                                                float dis = (pent->v.origin -
                   pEdict->v.origin).Length();
                                                                if(dis < 140.0f)
                                                                {
                                                                        pEdict->v.button |= IN_ATTACK; // charge the
                   weapon
                                                                        pBot->f_duck_time = pBot->f_think_time + 1.0f;
                                                                }
                                                        }
                                                }
                                                else continue;
                                        }

                                        // ignore this item if it's on an unavailable waypoint
                                        const int nearestWP
                                                = WaypointFindNearest_V(pent->v.origin, 80.0, -1);
                                        if(nearestWP != -1
                                                && !WaypointAvailable(nearestWP, pBot->current_team))
                                                continue;

                                //	can_pickup = TRUE;
                                }*/
                else if(strcmp("building_sentrygun", item_name) == 0) {
                    if(pEdict->v.playerclass != TFC_CLASS_ENGINEER ||
                        pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 140 ||
                        pBot->current_team != UTIL_GetTeam(pent))
                        continue;

                    static char modelName[24]; // static for speed reasons
                    strncpy(modelName, STRING(pent->v.model), 24);
                    modelName[23] = '\0';

                    // if the spotted sentry gun is damaged or not at level 3
                    if(strcmp(modelName, "models/sentry3.mdl") != 0 || pent->v.health < 91) {
                        // set up a job to help this sentry gun out
                        newJob = InitialiseNewJob(pBot, JOB_MAINTAIN_OBJECT);
                        if(newJob != NULL) {
                            newJob->object = pent;
                            SubmitNewJob(pBot, JOB_MAINTAIN_OBJECT, newJob);
                        }
                    }
                }
                // teleporter entrances
                else if(strcmp("building_teleporter", item_name) == 0) {
                    // ignore Teleporters whilst in combat
                    if(pBot->enemy.ptr != NULL)
                        continue;

                    // is this the bots own teleporter entrance?
                    // ignore it if the bot hasn't made an exit yet
                    if(pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER && pent == pBot->tpEntrance &&
                        pBot->tpExit == NULL)
                        continue;

                    // if it's an enemy teleporter maybe set up a job to attack it
                    const int TeleportTeam = BotTeamColorCheck(pent);
                    if(pBot->current_team != TeleportTeam && !(team_allies[pBot->current_team] & (1 << TeleportTeam))) {
                        newJob = InitialiseNewJob(pBot, JOB_ATTACK_TELEPORT);
                        if(newJob != NULL) {
                            newJob->object = pent;
                            if(SubmitNewJob(pBot, JOB_ATTACK_TELEPORT, newJob) == TRUE)
                                return;
                        }
                    }

                    // is this an allied Teleporter the bot hasn't used before?
                    else if(pBot->enemy.ptr == NULL && bot_can_use_teleporter == TRUE &&
                        pent->v.iuser1 == W_FL_TFC_TELEPORTER_ENTRANCE) {
                        // if the bot doesn't know where this entrance goes, give it a test run
                        const int telePairIndex = BotRecallTeleportEntranceIndex(pBot, pent);
                        if(telePairIndex == -1                             // unknown entrance
                            || pBot->telePair[telePairIndex].exitWP == -1) // unknown exit
                        {
                            newJob = InitialiseNewJob(pBot, JOB_USE_TELEPORT);
                            if(newJob != NULL) {
                                newJob->object = pent;
                                newJob->waypoint = WaypointFindNearest_E(pent, 500.0, pBot->current_team);
                                newJob->waypointTwo = -1; // remember that destination is unknown
                                if(SubmitNewJob(pBot, JOB_USE_TELEPORT, newJob) == TRUE)
                                    return;
                            }
                        }
                    }
                }
                // check if entity is armor...
                else if(strncmp("item_armor", item_name, 10) == 0) {
                    // see if someone owns this weapon or it hasn't respawned yet
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    if(PlayerArmorPercent(pEdict) < 100)
                        can_pickup = TRUE;
                }
                // check if entity is a healthkit...
                else if(strcmp("item_healthkit", item_name) == 0) {
                    // check if the bot wants to use this item...
                    if(pEdict->v.health >= pEdict->v.max_health)
                        continue;

                    // check if the item is not visible (i.e. has not respawned)
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    // ignore this item if it's on an unavailable waypoint
                    const int nearestWP = WaypointFindNearest_V(pent->v.origin, 80.0, -1);
                    if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                        continue;

                    can_pickup = TRUE;
                }
                // check if entity is ammo...
                else if(strncmp("ammo_", item_name, 5) == 0) {
                    // check if the item is not visible (i.e. has not respawned)
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    if(pBot->ammoStatus == AMMO_UNNEEDED)
                        continue;

                    // ignore this item if it's on an unavailable waypoint
                    const int nearestWP = WaypointFindNearest_V(pent->v.origin, 80.0, -1);
                    if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                        continue;

                    can_pickup = TRUE;
                }
                // check if entity is a weapon...
                else if(strncmp("weapon_", item_name, 7) == 0) {
                    // see if someone owns this weapon or it hasn't respawned yet
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    can_pickup = TRUE;
                }
                // check if entity is a battery...
                else if(strcmp("item_battery", item_name) == 0) {
                    // check if the bot wants to use this item...
                    if(pEdict->v.armorvalue >= VALVE_MAX_NORMAL_BATTERY)
                        continue;

                    // check if the item is not visible (i.e. has not respawned)
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    // ignore this item if it's on an unavailable waypoint
                    const int nearestWP = WaypointFindNearest_V(pent->v.origin, 80.0, -1);
                    if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                        continue;

                    can_pickup = TRUE;
                }
                // check if entity is a packed up weapons box...
                else if(strcmp("weaponbox", item_name) == 0) {
                    // if mod=tfc
                    // check if the item is not visible (i.e. has not respawned)
                    if(pent->v.effects & EF_NODRAW)
                        continue;

                    if(pBot->ammoStatus == AMMO_UNNEEDED)
                        continue;

                    // ignore this item if it's on an unavailable waypoint
                    const int nearestWP = WaypointFindNearest_V(pent->v.origin, 80.0, -1);

                    if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                        continue;

                    can_pickup = TRUE;
                }

                // check if entity is the spot from RPG laser
                //	else if(strcmp("laser_spot", item_name) == 0)
                //	{
                //	}

                //	// check if entity is an armed tripmine
                //	else if(strcmp("monster_tripmine", item_name) == 0)
                //	{
                //	}

                // check if entity is an armed satchel charge
                //	else if(strcmp("monster_satchel", item_name) == 0)
                //	{
                //	}
                // check if entity is a snark (squeak grenade)
                //	else if(strcmp("monster_snark", item_name) == 0)
                //	{
                //	}

                // some neotf stuff here
                // check if entity is a weapon...
                else if(strcmp("ntf_multigun", item_name) == 0) {
                    if(pent->v.effects & EF_NODRAW || pent->v.flags & FL_KILLME)
                        continue;

                    if(pent->v.team == (UTIL_GetTeam(pEdict) + 1))
                        can_pickup = TRUE;
                    else {
                        char* cvar_ntf_capture_mg = (char*)CVAR_GET_STRING("ntf_capture_mg");

                        if(strcmp(cvar_ntf_capture_mg, "1") == 0)
                            can_pickup = TRUE;
                    }
                }
            } // end if object is visible
        }     // end else not "func_" entity

        if(can_pickup) // if the bot found something it can pickup...
        {
            const float distance = (entity_origin - pEdict->v.origin).Length();

            // see if it's the closest item so far...
            if(distance < min_distance && entity_origin.z > (pEdict->v.origin.z - 80) &&
                entity_origin.z < pEdict->v.origin.z + 80) {
                min_distance = distance;       // update the minimum distance
                pPickupEntity = pent;          // remember this entity
                pickup_origin = entity_origin; // remember location of entity
            }
        }
    } // end while loop

    // if the bot has found something to pick up set up a job to pick it up
    if(pPickupEntity != NULL) {
        newJob = InitialiseNewJob(pBot, JOB_PICKUP_ITEM);
        if(newJob != NULL) {
            newJob->object = pPickupEntity;
            newJob->origin = pPickupEntity->v.origin; // remember it's exact location too
            newJob->waypoint = WaypointFindNearest_E(pPickupEntity, 500.0, pBot->current_team);
            SubmitNewJob(pBot, JOB_PICKUP_ITEM, newJob);
        }

        // a cool laser effect to show who has just seen a pickup
        // (for debugging purposes)
        //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
        //		pPickupEntity->v.origin, 10, 2, 250, 50, 250, 200, 10);
    }
}

// This function can be used periodically to get bots to spot flags that
// are quite far away so that they can home in on them.
static void BotFlagSpottedCheck(bot_t* pBot)
{
    if(pBot->bot_has_flag || BufferedJobIndex(pBot, JOB_PICKUP_FLAG) != -1)
        return;

    // only check for flags the bot can see
    // that are sitting around unclaimed
    edict_t* pent = NULL;
    while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "item_tfgoal")) != NULL && !FNullEnt(pent)) {
        // if the bot can see the flag
        if(pent->v.owner == NULL && !(pent->v.effects & EF_NODRAW) &&
            VectorsNearerThan(pBot->pEdict->v.origin, pent->v.origin, 1500.0) &&
            FInViewCone(pent->v.origin, pBot->pEdict) && BotCanSeeOrigin(pBot, pent->v.origin)) {
            // ignore this flag if it's on an unavailable waypoint
            int nearestWP = WaypointFindNearest_V(pent->v.origin, 80.0, -1);
            if(nearestWP != -1 && !WaypointAvailable(nearestWP, pBot->current_team))
                continue;

            // not found a waypoint very near the flag yet?  try further out
            if(nearestWP == -1)
                nearestWP = WaypointFindNearest_E(pent, 500.0f, pBot->current_team);

            if(nearestWP != -1) {
                // set up a job to retrieve the flag
                job_struct* newJob = InitialiseNewJob(pBot, JOB_PICKUP_FLAG);
                if(newJob != NULL) {
                    newJob->waypoint = nearestWP;
                    newJob->object = pent;
                    newJob->origin = pent->v.origin;
                    SubmitNewJob(pBot, JOB_PICKUP_FLAG, newJob);
                }
            }

            return;
        }
    }
}

// This function checks the specified bots general ammo levels and then
// stores it's verdict in pBot->ammoStatus.
// It wont detect if the bots ammo level is very low, as that is done
// elsewhere (probably in the weapon firing code).
static void BotAmmoCheck(bot_t* pBot)
{
    switch(pBot->pEdict->v.playerclass) {
    case TFC_CLASS_SCOUT:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_SCOUT_MAXAMMO_SHOT / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_NAILGUN].iAmmo1] >= PC_SCOUT_MAXAMMO_NAIL / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_SNIPER:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_SNIPER_MAXAMMO_SHOT / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_NAILGUN].iAmmo1] >= PC_SNIPER_MAXAMMO_NAIL / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_SOLDIER:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_SOLDIER_MAXAMMO_SHOT / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_RPG].iAmmo1] >= PC_SOLDIER_MAXAMMO_ROCKET / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_DEMOMAN:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_DEMOMAN_MAXAMMO_SHOT / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_RPG].iAmmo1] >= PC_DEMOMAN_MAXAMMO_ROCKET / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_MEDIC:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_MEDIC_MAXAMMO_SHOT / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_NAILGUN].iAmmo1] >= PC_MEDIC_MAXAMMO_NAIL / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_HWGUY:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_HVYWEAP_MAXAMMO_SHOT / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_PYRO:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_PYRO_MAXAMMO_SHOT / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_FLAMETHROWER].iAmmo1] >= PC_PYRO_MAXAMMO_CELL / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_RPG].iAmmo1] >= PC_PYRO_MAXAMMO_ROCKET / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_SPY:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= PC_SPY_MAXAMMO_SHOT / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_NAILGUN].iAmmo1] >= PC_SPY_MAXAMMO_NAIL / 2)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_ENGINEER:
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] == PC_ENGINEER_MAXAMMO_CELL &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] >= (PC_ENGINEER_MAXAMMO_SHOT - 10) &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_NAILGUN].iAmmo1] >= PC_ENGINEER_MAXAMMO_NAIL / 2 &&
            pBot->m_rgAmmo[weapon_defs[TF_WEAPON_RPG].iAmmo1] == PC_ENGINEER_MAXAMMO_ROCKET)
            pBot->ammoStatus = AMMO_UNNEEDED;
        else if(pBot->ammoStatus != AMMO_LOW)
            pBot->ammoStatus = AMMO_WANTED;
        break;
    case TFC_CLASS_CIVILIAN: // my Umbrella needs ammo!
        pBot->ammoStatus = AMMO_UNNEEDED;
        break;
    }
}

// This function handles the behaviour of bots when they are in close
// proximity to other players.  This returns an edict pointer
// to any player the bot is trying to get around, NULL otherwise.
edict_t* BotContactThink(bot_t* pBot)
{
    if(pBot->f_snipe_time > pBot->f_think_time)
        return NULL;

    float nearestdistance = 100.0f;
    float distanceToPlayer;
    Vector vang; // vang is angle to avoid person
    float pv;
    edict_t* playerToAvoid = NULL;
    edict_t* pPlayer;

    // disguised spies prefer to keep more of a distance
    if(pBot->disguise_state == DISGUISE_COMPLETE && pBot->pEdict->v.playerclass == TFC_CLASS_SPY)
        nearestdistance = 150.0f;

    // search the player list for players who might be too close
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if(pPlayer != NULL && !pPlayer->free && pPlayer != pBot->pEdict && IsAlive(pPlayer)) {
            if((b_observer_mode) && !((pPlayer->v.flags & FL_FAKECLIENT) == FL_FAKECLIENT))
                continue;

            // don't avoid if going for player with one of these weapons
            if(pBot->enemy.ptr == pPlayer || (pBot->currentJob > -1 && pPlayer == pBot->job[pBot->currentJob].player)) {
                if(pBot->current_weapon.iId == TF_WEAPON_KNIFE || pBot->current_weapon.iId == TF_WEAPON_MEDIKIT ||
                    pBot->current_weapon.iId == TF_WEAPON_SPANNER || pBot->current_weapon.iId == TF_WEAPON_AXE)
                    continue;
            }

            vang = pPlayer->v.origin - pBot->pEdict->v.origin;
            distanceToPlayer = vang.Length();

            // nearest player to the bot so far?
            if(distanceToPlayer >= nearestdistance)
                continue;

            vang = UTIL_VecToAngles(vang);
            pv = pBot->pEdict->v.v_angle.y + 180.0;

            vang.y -= pv;
            if(vang.y < 0.0)
                vang.y += 360.0;

            if(!((UTIL_GetTeamColor(pBot->pEdict) == UTIL_GetTeamColor(pPlayer) ||
                     team_allies[pBot->current_team] & (1 << UTIL_GetTeam(pPlayer))) &&
                   pPlayer == pBot->enemy.ptr)) {
                Vector vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

                if(FInViewCone(vecEnd, pBot->pEdict) && FVisible(vecEnd, pBot->pEdict)) {
                    // this player is the nearest to the bot found so far
                    nearestdistance = distanceToPlayer;
                    playerToAvoid = pPlayer;
                }
            }
        }
    }

    // avoid the nearest player if they are too close
    if(playerToAvoid != NULL) {
        // if the other player is ducking jump over them
        if(playerToAvoid->v.button & IN_DUCK && nearestdistance < 70.1f) {
            pBot->pEdict->v.button |= IN_JUMP;
            pBot->f_duck_time = pBot->f_think_time + 0.5f;
        } else {
            // try to strafe around the other player
            if(vang.y <= 180) {
                pBot->side_direction = SIDE_DIRECTION_LEFT;
                pBot->f_side_speed = -(pBot->f_max_speed);
                //	pBot->f_waypoint_drift = -10.0f; // experimental feature
            } else {
                pBot->side_direction = SIDE_DIRECTION_RIGHT;
                pBot->f_side_speed = pBot->f_max_speed;
                //	pBot->f_waypoint_drift = 10.0f; // experimental feature
            }

            /*	// check if the bot is about to strafe around a slow moving
                    // player into a wall, and try to avoid getting stuck doing so
                    if(playerToAvoid->v.velocity.x < 1.0
                            && playerToAvoid->v.velocity.y < 1.0
                            && pBot->f_current_wp_deadline < (pBot->f_think_time + BOT_WP_DEADLINE - 2.0))
                    {
                            WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                                    playerToAvoid->v.origin + Vector(0, 0, 50.0), 10, 2, 250, 50, 250, 200, 10);

                            if(pBot->side_direction = SIDE_DIRECTION_LEFT)
                            {
                                    if(BotCheckWallOnLeft(pBot))
                                    {
                                            job_struct *newJob = InitialiseNewJob(pBot, JOB_GET_UNSTUCK);
                                            if(newJob != NULL)
                                                    SubmitNewJob(pBot, JOB_GET_UNSTUCK, newJob);
                                    }
                            }
                            else
                            {
                                    if(BotCheckWallOnRight(pBot))
                                    {
                                            job_struct *newJob = InitialiseNewJob(pBot, JOB_GET_UNSTUCK);
                                            if(newJob != NULL)
                                                    SubmitNewJob(pBot, JOB_GET_UNSTUCK, newJob);
                                    }
                            }
                    }*/

            // back off if very near and the other player is
            // looking at the bot
            Vector vecEnd = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
            if(nearestdistance <= 70.0 && FInViewCone(vecEnd, playerToAvoid))
                pBot->f_move_speed = -(pBot->f_max_speed);
        }

        // if a slower player is at the bots current waypoint
        // preventing the bot reaching it, try to cope with that here
        if(pBot->current_wp != pBot->goto_wp && pBot->current_wp != -1 && pBot->pEdict->v.flags & FL_ONGROUND &&
            VectorsNearerThan(playerToAvoid->v.origin, waypoints[pBot->current_wp].origin, 40.0) &&
            VectorsNearerThan(pBot->pEdict->v.origin, waypoints[pBot->current_wp].origin, 100.1) &&
            playerToAvoid->v.velocity.Length2D() < pBot->pEdict->v.velocity.Length2D()) {
            int nextWP;
            if(pBot->branch_waypoint == -1)
                nextWP = WaypointRouteFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team);
            else
                nextWP = WaypointRouteFromTo(pBot->current_wp, pBot->branch_waypoint, pBot->current_team);

            if(nextWP != -1 && !(waypoints[pBot->current_wp].flags & W_FL_LIFT) &&
                !(waypoints[pBot->current_wp].flags & W_FL_JUMP) &&
                BotCanSeeOrigin(pBot, waypoints[pBot->current_wp].origin))
                pBot->current_wp = nextWP;
        }

        return playerToAvoid;
    }

    return NULL;
}

void script(const char* sz)
{
    int i, j, current_msg;
    msg_com_struct* curr;
    bool execif;

    if(g_bot_debug) {
        char msg[255];
        _snprintf(msg, 250, "msg (%s)\n", sz);
        ALERT(at_console, msg);

        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "msg (%s)\n", sz);
            fclose(fp);
        }
    }

    for(current_msg = 0; current_msg < MSG_MAX; current_msg++) {
        if(sz != NULL && (msg_msg[current_msg]) != NULL && sz[0] != 0 && (msg_msg[current_msg])[0] != 0) {
            // ALERT( at_console, msg_msg[current_msg]);
            // ALERT( at_console, "-\n");
            // some messages have line returns, so check
            char msg2[255];
            _snprintf(msg2, 250, "%s\n", msg_msg[current_msg]);
            if(strcmp(sz, msg_msg[current_msg]) == 0 || strcmp(sz, msg2) == 0) {
                /*{ fp=UTIL_OpenFoxbotLog();
                        fprintf(fp,"a %d\n",current_msg); fclose(fp); }*/
                if(msg_com[current_msg].ifs[0] == '\0') {
                    if(g_bot_debug) {
                        char msg[255];
                        _snprintf(msg, 250, "no if : %s +++ %s %d\n", msg_msg[current_msg], sz, current_msg);
                        ALERT(at_console, msg);
                        /*{ fp=UTIL_OpenFoxbotLog();
                                fprintf(fp,msg,sz); fclose(fp); }*/
                    }
                    for(i = 0; i < 8; i++) {
                        j = msg_com[current_msg].blue_av[i];
                        if(j == 1)
                            blue_av[i] = TRUE;
                        else if(j == 0)
                            blue_av[i] = FALSE;

                        j = msg_com[current_msg].red_av[i];
                        if(j == 1)
                            red_av[i] = TRUE;
                        else if(j == 0)
                            red_av[i] = FALSE;

                        j = msg_com[current_msg].yellow_av[i];
                        if(j == 1)
                            yellow_av[i] = TRUE;
                        else if(j == 0)
                            yellow_av[i] = FALSE;

                        j = msg_com[current_msg].green_av[i];
                        if(j == 1)
                            green_av[i] = TRUE;
                        else if(j == 0)
                            green_av[i] = FALSE;
                    }
                }
                // now deal with ifs... on message may start with
                // an if(no default behaviour)
                /*{ fp=UTIL_OpenFoxbotLog();
                        fprintf(fp,"b %d\n",current_msg); fclose(fp); }*/

                curr = &msg_com[current_msg];
                while(curr != NULL && curr != 0 && (int)curr != -1) {
                    /*{ fp=UTIL_OpenFoxbotLog();
                            fprintf(fp,"Started while %s %d\n",sz,current_msg);
                            fclose(fp); }*/
                    if(curr->ifs[0] != '\0') {
                        execif = FALSE;

                        /*{ fp=UTIL_OpenFoxbotLog();
                                fprintf(fp, "%s\n",curr->ifs);
                                fclose(fp); }*/

                        // problem with 0 being returned by atoi??
                        if(strncmp(curr->ifs, "b_p_", 4) == 0)
                            if(atoi(&curr->ifs[4]) > 0 && atoi(&curr->ifs[4]) < 9)
                                if(blue_av[atoi(&curr->ifs[4]) - 1])
                                    execif = TRUE;
                        if(strncmp(curr->ifs, "r_p_", 4) == 0)
                            if(atoi(&curr->ifs[4]) > 0 && atoi(&curr->ifs[4]) < 9)
                                if(red_av[atoi(&curr->ifs[4]) - 1])
                                    execif = TRUE;
                        if(strncmp(curr->ifs, "g_p_", 4) == 0)
                            if(atoi(&curr->ifs[4]) > 0 && atoi(&curr->ifs[4]) < 9)
                                if(green_av[atoi(&curr->ifs[4]) - 1])
                                    execif = TRUE;
                        if(strncmp(curr->ifs, "y_p_", 4) == 0)
                            if(atoi(&curr->ifs[4]) > 0 && atoi(&curr->ifs[4]) < 9)
                                if(yellow_av[atoi(&curr->ifs[4]) - 1])
                                    execif = TRUE;
                        // points not available
                        if(strncmp(curr->ifs, "b_pn_", 5) == 0)
                            if(atoi(&curr->ifs[5]) > 0 && atoi(&curr->ifs[5]) < 9)
                                if(!blue_av[atoi(&curr->ifs[5]) - 1])
                                    execif = TRUE;
                        if(strncmp(curr->ifs, "r_pn_", 5) == 0)
                            if(atoi(&curr->ifs[5]) > 0 && atoi(&curr->ifs[5]) < 9)
                                if(!red_av[atoi(&curr->ifs[5]) - 1])
                                    execif = TRUE;
                        if(strncmp(curr->ifs, "g_pn_", 5) == 0)
                            if(atoi(&curr->ifs[5]) > 0 && atoi(&curr->ifs[5]) < 9)
                                if(!green_av[atoi(&curr->ifs[5]) - 1])
                                    execif = TRUE;
                        if(strncmp(curr->ifs, "y_pn_", 5) == 0)
                            if(atoi(&curr->ifs[5]) > 0 && atoi(&curr->ifs[5]) < 9)
                                if(!yellow_av[atoi(&curr->ifs[5]) - 1])
                                    execif = TRUE;
                        // multipoint ifs
                        if(strncmp(curr->ifs, "b_mp_", 5) == 0) {
                            bool con = TRUE;
                            for(int k = 0; k < 8; k++) {
                                if((curr->ifs[k + 5] == '0' || curr->ifs[k + 5] == '1') && con) {
                                    // set to false
                                    // only set to true if we can continue and exec if
                                    con = FALSE;
                                    if(curr->ifs[k + 5] == '1') {
                                        if(blue_av[k])
                                            con = TRUE;
                                    } else {
                                        if(!blue_av[k])
                                            con = TRUE;
                                    }
                                }
                            }
                            if(con)
                                execif = TRUE;
                        }
                        if(strncmp(curr->ifs, "r_mp_", 5) == 0) {
                            bool con = TRUE;
                            for(int k = 0; k < 8; k++) {
                                if((curr->ifs[k + 5] == '0' || curr->ifs[k + 5] == '1') && con) {
                                    // set to false
                                    // only set to true if we can continue and exec if
                                    con = FALSE;
                                    if(curr->ifs[k + 5] == '1') {
                                        if(red_av[k])
                                            con = TRUE;
                                    } else {
                                        if(!red_av[k])
                                            con = TRUE;
                                    }
                                }
                            }
                            if(con)
                                execif = TRUE;
                        }
                        if(strncmp(curr->ifs, "g_mp_", 5) == 0) {
                            bool con = TRUE;
                            for(int k = 0; k < 8; k++) {
                                if((curr->ifs[k + 5] == '0' || curr->ifs[k + 5] == '1') && con) {
                                    // set to false
                                    // only set to true if we can continue and exec if
                                    con = FALSE;
                                    if(curr->ifs[k + 5] == '1') {
                                        if(green_av[k])
                                            con = TRUE;
                                    } else {
                                        if(!green_av[k])
                                            con = TRUE;
                                    }
                                }
                            }
                            if(con)
                                execif = TRUE;
                        }
                        if(strncmp(curr->ifs, "y_mp_", 5) == 0) {
                            bool con = TRUE;
                            for(int k = 0; k < 8; k++) {
                                if((curr->ifs[k + 5] == '0' || curr->ifs[k + 5] == '1') && con) {
                                    // set to false
                                    // only set to true if we can continue and exec if
                                    con = FALSE;
                                    if(curr->ifs[k + 5] == '1') {
                                        if(yellow_av[k])
                                            con = TRUE;
                                    } else {
                                        if(!yellow_av[k])
                                            con = TRUE;
                                    }
                                }
                            }
                            if(con)
                                execif = TRUE;
                        }

                        /*{ fp=UTIL_OpenFoxbotLog();
                                fprintf(fp, "-pass\n"); fclose(fp);}*/
                        if(g_bot_debug) {
                            char msg[255];
                            _snprintf(msg, 250, "if : %s +++ %s %d \nComparing point %s\n", msg_msg[current_msg], sz,
                                current_msg, (curr->ifs) + 4);
                            ALERT(at_console, msg);
                            /*{ fp=UTIL_OpenFoxbotLog();
                                    fprintf(fp,msg,sz); fclose(fp);}*/
                        }
                        if(execif) {
                            if(g_bot_debug) {
                                char msg[64];
                                _snprintf(msg, 63, "Executing if\n");
                                ALERT(at_console, msg);
                                /*{ fp=UTIL_OpenFoxbotLog();
                                        fprintf(fp,msg,sz); fclose(fp); }*/
                            }
                            for(i = 0; i < 8; i++) {
                                j = curr->blue_av[i];
                                if(j == 1)
                                    blue_av[i] = TRUE;
                                else if(j == 0)
                                    blue_av[i] = FALSE;

                                j = curr->red_av[i];
                                if(j == 1)
                                    red_av[i] = TRUE;
                                else if(j == 0)
                                    red_av[i] = FALSE;

                                j = curr->yellow_av[i];
                                if(j == 1)
                                    yellow_av[i] = TRUE;
                                else if(j == 0)
                                    yellow_av[i] = FALSE;

                                j = curr->green_av[i];
                                if(j == 1)
                                    green_av[i] = TRUE;
                                else if(j == 0)
                                    green_av[i] = FALSE;
                            }
                        }
                    }
                    curr = curr->next;
                }
            }
        }
    }

    /*	// stop the bots using scripted waypoints they shouldn't have access to
            for(j = 0; j < MAX_BOTS; j++)
            {
                    if(bots[j].is_used
                            && bots[j].current_team > -1)
                    {
                            // engineers need to scrap stuff they have built at non-available waypoints
                            if(bots[j].pEdict->v.playerclass == TFC_CLASS_ENGINEER)
                            {
                            }
                    }
            }*/
}

// BotArmorValue - returns the percentage of armor a bot has.
int PlayerArmorPercent(const edict_t* pEdict)
{
    const static int tfc_max_armor[10] = { 0, 50, 50, 200, 120, 100, 300, 150, 100, 50 };

    if(mod_id == TFC_DLL && pEdict->v.playerclass >= 0 && pEdict->v.playerclass <= 9)
        return static_cast<int>(pEdict->v.armorvalue / tfc_max_armor[pEdict->v.playerclass] * 100);

    // Unknown mod, return 100%
    return 100;
}

// PlayerHealthValue - returns the percentage of health a bot has.
int PlayerHealthPercent(const edict_t* pEdict)
{
    if(mod_id == TFC_DLL)
        return static_cast<int>((pEdict->v.health / pEdict->v.max_health) * 100);

    // Unknown mod, return 100%
    return 100;
}

// Make the bot spray graffiti.  If sprayDownwards is false the bot will
// try to spray straight ahead instead.
void BotSprayLogo(edict_t* pEntity, const bool sprayDownwards)
{
    int index = -1;

    UTIL_MakeVectors(pEntity->v.v_angle);

    Vector v_src = pEntity->v.origin; // + pEntity->v.view_ofs;
    Vector v_dest;

    if(sprayDownwards)
        v_dest = pEntity->v.origin - Vector(0, 0, 80.0);
    else
        v_dest = v_src + gpGlobals->v_forward * 80.0;

    int count = 0;
    while(index < 0 && count < 10) {
        switch(random_long(0, 10)) {
        case 0:
            index = DECAL_INDEX("{FOXBOT");
            break;
        case 1:
            index = DECAL_INDEX("{FOXBOT0");
            break;
        case 2:
            index = DECAL_INDEX("{FOXBOT1");
            break;
        case 3:
            index = DECAL_INDEX("{FOXBOT2");
            break;
        case 4:
            index = DECAL_INDEX("{FOXBOT3");
            break;
        case 5:
            index = DECAL_INDEX("{FOXBOT4");
            break;
        case 6:
            index = DECAL_INDEX("{FOXBOT5");
            break;
        case 7:
            index = DECAL_INDEX("{FOXBOT6");
            break;
        case 8:
            index = DECAL_INDEX("{FOXBOT7");
            break;
        case 9:
            index = DECAL_INDEX("{FOXBOT8");
            break;
        case 10:
            index = DECAL_INDEX("{FOXBOT9");
            break;
        }
        count++;
    }

    if(index < 0)
        index = DECAL_INDEX("{FOXBOT");
    if(index < 0)
    // return;
    // index=0;
    {
        switch(random_long(1, 4)) {
        case 1:
            index = DECAL_INDEX("{BIOHAZ");
            break;
        case 2:
            index = DECAL_INDEX("{TARGET");
            break;
        case 3:
            index = DECAL_INDEX("{LAMBDA06");
            break;
        case 4:
            index = DECAL_INDEX("{GRAF004");
            break;
        }
    }
    if(index < 0)
        // return;
        index = 0;

    TraceResult pTrace;
    UTIL_TraceLine(v_src, v_dest, ignore_monsters, pEntity->v.pContainingEntity, &pTrace);

    if((pTrace.pHit) && (pTrace.flFraction < 1.0)) {
        if(pTrace.pHit->v.solid != SOLID_BSP)
            return;

        MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

        if(index > 255) {
            WRITE_BYTE(TE_WORLDDECALHIGH);
            index -= 256;
        } else
            WRITE_BYTE(TE_WORLDDECAL);

        WRITE_COORD(pTrace.vecEndPos.x);
        WRITE_COORD(pTrace.vecEndPos.y);
        WRITE_COORD(pTrace.vecEndPos.z);
        WRITE_BYTE(index);

        MESSAGE_END();

        EMIT_SOUND_DYN2(pEntity, CHAN_VOICE, "player/sprayer.wav", 1.0, ATTN_NORM, 0, 100);
    }
}

// This function will check all the enemies around the bot to see
// if they have the attack button pressed down or are making footstep sounds.
// This is a simple(and imperfect) way of making bots respond to enemies.
static void BotAttackerCheck(bot_t* pBot)
{
    // stop the bot from responding to gunshot "sounds" too often,
    // unless they've been injured very recently
    if(pBot->f_soundReactTime > pBot->f_think_time && (pBot->f_injured_time + 0.3f) < pBot->f_think_time)
        return;

    // don't let feigning spies "hear" gunshots unless they've been
    // injured very recently
    if(pBot->pEdict->v.deadflag == 5 && (pBot->f_injured_time + 0.3f) < pBot->f_think_time)
        return;

    // WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
    //	pBot->pEdict->v.origin + Vector(0, 0, 50),
    //	10, 2, 250, 250, 250, 200, 10);

    int player_team;
    edict_t* pPlayer;
    Vector vecEnd;

    edict_t* mysteryPlayer = NULL; // unseen enemies
    bool readyDefender = FALSE;
    if(pBot->mission == ROLE_DEFENDER && pBot->visAllyCount < 4 // too many allies nearby = white noise
        && pBot->bot_skill < 4 && PlayerHealthPercent(pBot->pEdict) >= pBot->trait.health &&
        (pBot->enemy.f_lastSeen + 5.0) < pBot->f_think_time)
        readyDefender = TRUE;

    // start checking from a randomly selected player, this is
    // done to avoid using CPU time picking the nearest heard enemy
    int i = random_long(1, gpGlobals->maxClients);

    // search the world for players...
    for(int index = 1; index <= gpGlobals->maxClients; index++, i++) {
        // wrap the search if it exceeds the number of max players
        if(i > gpGlobals->maxClients)
            i = 1;

        pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if(pPlayer && !pPlayer->free && pPlayer != pBot->pEdict && IsAlive(pPlayer)) {
            if(b_observer_mode && !(pPlayer->v.flags & FL_FAKECLIENT))
                continue;

            // is team play enabled?
            if(mod_id == TFC_DLL) {
                player_team = UTIL_GetTeam(pPlayer);

                // don't target your teammates or allies
                if(pBot->current_team == player_team || team_allies[pBot->current_team] & (1 << player_team))
                    continue;
            }

            // ignore enemies who are too far away
            if(!VectorsNearerThan(pPlayer->v.origin, pBot->pEdict->v.origin, 1000.0f))
                continue;

            // ignore enemies who don't have the fire button pressed
            // TODO: ignore silent weapons(e.g. the Medikit and charging sniper rifle)
            if(!(pPlayer->v.button & IN_ATTACK)) {
                // they don't have the attack button pressed,
                // can we hear their footsteps instead?
                if(UTIL_FootstepsHeard(pBot->pEdict, pPlayer) == FALSE)
                    continue;
            }

            vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

            // if the bot can see the player, but is not facing them then
            // turn in that players general direction
            if(FVisible(vecEnd, pBot->pEdict)) {
                if(!FInViewCone(vecEnd, pBot->pEdict)) {
                    // set up a job for spotting the unseen assailant
                    job_struct* newJob = InitialiseNewJob(pBot, JOB_SPOT_STIMULUS);
                    if(newJob != NULL) {
                        newJob->origin = pPlayer->v.origin + pPlayer->v.view_ofs;
                        newJob->waypoint = pBot->goto_wp;
                        SubmitNewJob(pBot, JOB_SPOT_STIMULUS, newJob);
                    }

                    // don't check for gunshot "sounds" again for a few seconds
                    pBot->f_soundReactTime = pBot->f_think_time + RANDOM_FLOAT(3.0f, 4.0f);

                    return;
                }
            } else if(readyDefender // remember any unseen enemies we can investigate
                && mysteryPlayer == NULL) {
                mysteryPlayer = pPlayer;
            }
        }
    }

    // no visible enemies were found, were any non-visible enemies found
    // near enough to go investigate
    if(mysteryPlayer != NULL) {
        job_struct* newJob = InitialiseNewJob(pBot, JOB_INVESTIGATE_AREA);
        if(newJob != NULL) {
            newJob->waypoint = WaypointFindNearest_V(mysteryPlayer->v.origin, 500.0, pBot->current_team);

            if(newJob->waypoint != -1) {
                SubmitNewJob(pBot, JOB_INVESTIGATE_AREA, newJob);
                /* // debug stuff
                        {
                                WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                                        waypoints[newJob->waypoint].origin, 10, 2, 50, 250, 250, 200, 10);

                                WaypointDrawBeam(INDEXENT(1), mysteryPlayer->v.origin,
                                        waypoints[newJob->waypoint].origin, 10, 2, 250, 0, 0, 200, 10);
                        }*/

                // don't check for footstep "sounds" again for a few seconds
                pBot->f_soundReactTime = pBot->f_think_time + RANDOM_FLOAT(1.5f, 3.0f);
            }
        }
    }
}

// Called by the Sound Hooking Code (in EMIT_SOUND)
// This function is going to handle all the sounds that bots can respond to.
// We are going to loop through all the bots when we receive a sound,
// responding to and push them onto a stack of sounds for the bot.
// According to Botman some sound stuff is handled by the client(such as
// footsteps).  But this function still works on Listen and Dedicated servers.
void BotSoundSense(edict_t* pEdict, const char* pszSample, float fVolume)
{
    if(pEdict == NULL)
        return;

    const int sourceTeam = UTIL_GetTeam(pEdict);
    if(sourceTeam == -1)
        return;

    // get a bot to report what it hears
    /*	if(bots[0].pEdict && UTIL_GetTeam(pEdict) != -1)
            {
                    char msg[80];
                    strcpy(msg, pszSample);
                    UTIL_HostSay(bots[0].pEdict, 0, msg);
                    if(pEdict)
                    {
                            sprintf(msg, "pedict team %d fVolume:%f",
                                    UTIL_GetTeam(pEdict), fVolume);
                            UTIL_HostSay(bots[0].pEdict, 0, msg);
                    }
            }*/

    job_struct* newJob;

    // make defenders respond to combat sounds
    if(strncmp("weapons/ax1", pszSample, 11) == 0 || strncmp("player/pain", pszSample, 11) == 0 ||
        strncmp("items/armoron_1", pszSample, 15) == 0 || strncmp("items/smallmedkit", pszSample, 17) == 0 ||
        strncmp("items/ammopickup", pszSample, 16) == 0) {
        float botDistance;
        const float hearingDistance = 1500 * fVolume;
        int closestWPToThreat = -1;

        for(int i = 0; i < MAX_BOTS; i++) {
            if(bots[i].is_used // Is this a bot?
                && bots[i].visEnemyCount < 1 && bots[i].mission == ROLE_DEFENDER && bots[i].current_wp != -1 &&
                (bots[i].current_team != sourceTeam || random_long(0, 1000) < 333)) {
                botDistance = (bots[i].pEdict->v.origin - pEdict->v.origin).Length();
                if(botDistance < hearingDistance) {
                    // This bot can hear it. Try going to it
                    if(random_long(1, 100) < (100 - bots[i].bot_skill * 10)) {
                        if(closestWPToThreat == -1)
                            closestWPToThreat = WaypointFindNearest_V(pEdict->v.origin, 500.0, bots[i].current_team);

                        int respondDist = static_cast<int>(defendMaxRespondDist[bots[i].pEdict->v.playerclass]);

                        // Respond if we are within range
                        if(WaypointDistanceFromTo(bots[i].current_wp, closestWPToThreat, bots[i].current_team) <
                            respondDist) {
                            // If it's a spy, and we can see it, attack the mofo
                            if(pEdict->v.playerclass == TFC_CLASS_SPY) {
                                if(FInViewCone(pEdict->v.origin, bots[i].pEdict) &&
                                    FVisible(pEdict->v.origin, bots[i].pEdict))
                                    bots[i].enemy.ptr = pEdict;
                            }

                            if(!FInViewCone(pEdict->v.origin, bots[i].pEdict)) {
                                // set up a job to investigate the sound
                                newJob = InitialiseNewJob(&bots[i], JOB_INVESTIGATE_AREA);
                                if(newJob != NULL) {
                                    newJob->waypoint = closestWPToThreat;
                                    SubmitNewJob(&bots[i], JOB_INVESTIGATE_AREA, newJob);
                                }
                            }
                            // UTIL_HostSay(bots[i].pEdict, 0, "Heard something.");
                        }
                    }
                }
            }
        }
    }
    // heard a call for a medic?
    else if(strncmp("speech/saveme", pszSample, 13) == 0) {
        const float hearingDistance = 1000 * fVolume;
        float botDistance;
        float nearestMedDist = 1200.0f;
        float nearestEngDist = 1200.0f;
        int nearestMedic = -1;
        int nearestEngy = -1;

        // find the nearest Medic and Engineer to the sound source
        for(int index = 0; index < 32; index++) {
            // look for the nearest free Medic
            if(bots[index].is_used && bots[index].pEdict->v.playerclass == TFC_CLASS_MEDIC &&
                bots[index].enemy.ptr == NULL) {
                botDistance = (bots[index].pEdict->v.origin - pEdict->v.origin).Length();
                if(botDistance < hearingDistance && botDistance < nearestMedDist) {
                    nearestMedic = index;
                    nearestMedDist = botDistance;
                    continue;
                }
            }

            // look for the nearest free Engineer
            if(bots[index].is_used && bots[index].pEdict->v.playerclass == TFC_CLASS_ENGINEER &&
                bots[index].enemy.ptr == NULL && bots[index].m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 20) {
                botDistance = (bots[index].pEdict->v.origin - pEdict->v.origin).Length();
                if(botDistance < hearingDistance && botDistance < nearestEngDist) {
                    nearestEngy = index;
                    nearestEngDist = botDistance;
                }
            }
        }

        // send the nearest medic found
        if(nearestMedic != -1) {
            // target the patient if they're visible
            if(FInViewCone(pEdict->v.origin, bots[nearestMedic].pEdict) &&
                FVisible(pEdict->v.origin, bots[nearestMedic].pEdict)) {
                if(bots[nearestMedic].current_team == UTIL_GetTeam(pEdict)) {
                    // set up a job to handle the healing/repairing
                    newJob = InitialiseNewJob(&bots[nearestMedic], JOB_BUFF_ALLY);
                    if(newJob != NULL) {
                        newJob->player = pEdict;
                        newJob->origin = pEdict->v.origin; // remember where the player was
                        SubmitNewJob(&bots[nearestMedic], JOB_BUFF_ALLY, newJob);
                    }
                }
            } else // go find the person who called for a Medic
            {
                newJob = InitialiseNewJob(&bots[nearestMedic], JOB_INVESTIGATE_AREA);
                if(newJob != NULL) {
                    int closestWPToSound =
                        WaypointFindNearest_V(pEdict->v.origin, 500.0, bots[nearestMedic].current_team);

                    if(closestWPToSound != -1) {
                        // un-comment this WaypointDrawBeam to see this function in action
                        /*		WaypointDrawBeam(INDEXENT(1), bots[nearestMedic].pEdict->v.origin,
                                                waypoints[closestWPToSound].origin, 10, 2, 50, 250, 50, 200, 10);*/

                        // set up a job to investigate the sound
                        newJob->waypoint = closestWPToSound;
                        SubmitNewJob(&bots[nearestMedic], JOB_INVESTIGATE_AREA, newJob);
                    }
                }
            }
        }

        // send the nearest Engineer found
        if(nearestEngy != -1) {
            // target the patient if they're visible
            if(FInViewCone(pEdict->v.origin, bots[nearestEngy].pEdict) &&
                FVisible(pEdict->v.origin, bots[nearestEngy].pEdict)) {
                if(bots[nearestEngy].current_team == UTIL_GetTeam(pEdict) && !PlayerIsInfected(pEdict) &&
                    (PlayerArmorPercent(pEdict) < 100)) {
                    // set up a job to handle the healing/repairing
                    newJob = InitialiseNewJob(&bots[nearestEngy], JOB_BUFF_ALLY);
                    if(newJob != NULL) {
                        newJob->player = pEdict;
                        newJob->origin = pEdict->v.origin; // remember where the player was
                        SubmitNewJob(&bots[nearestEngy], JOB_BUFF_ALLY, newJob);
                    }
                }
            } else // go find the person who called for a Medic
            {
                newJob = InitialiseNewJob(&bots[nearestEngy], JOB_INVESTIGATE_AREA);
                if(newJob != NULL) {
                    int closestWPToSound =
                        WaypointFindNearest_V(pEdict->v.origin, 500.0, bots[nearestEngy].current_team);

                    if(closestWPToSound != -1) {
                        // un-comment this WaypointDrawBeam to see this function in action
                        /*		WaypointDrawBeam(INDEXENT(1), bots[nearestEngy].pEdict->v.origin,
                                                waypoints[closestWPToSound].origin, 10, 2, 50, 250, 50, 200, 10);*/

                        // set up a job to investigate the sound
                        newJob->waypoint = closestWPToSound;
                        SubmitNewJob(&bots[nearestEngy], JOB_INVESTIGATE_AREA, newJob);
                    }
                }
            }
        }
    }

    // make all other bots turn to face the source of interesting sounds
    const float hearingDistance = 1500 * fVolume;
    float botDistance;
    for(int i = 0; i < MAX_BOTS; i++) {
        // only check with bots who are now ready to respond to a sound
        if(bots[i].is_used // Is this a bot?
            && bots[i].f_soundReactTime < bots[i].f_think_time &&
            bots[i].pEdict->v.deadflag != 5) // skip feigning spies(for now)
        {
            newJob = InitialiseNewJob(&bots[i], JOB_SPOT_STIMULUS);
            if(newJob == NULL)
                continue;

            // sometimes ignore sounds made by allies
            // technically, this is cheating, but will simulate intelligence
            if(bots[i].current_team == sourceTeam && random_long(0, 1000) < 400)
                continue;

            botDistance = (bots[i].pEdict->v.origin - pEdict->v.origin).Length();

            if(botDistance < hearingDistance && botDistance > 100.0f) {
                // if this bot can hear the sound, but is not looking at its source
                if(!bots[i].enemy.ptr && !FInViewCone(pEdict->v.origin, bots[i].pEdict) &&
                    FVisible(pEdict->v.origin, bots[i].pEdict)) {
                    // set up a job for spotting the unseen activity
                    newJob->origin = pEdict->v.origin;
                    newJob->waypoint = bots[i].goto_wp;
                    SubmitNewJob(&bots[i], JOB_SPOT_STIMULUS, newJob);

                    // don't react to sounds again for a few seconds
                    // longer if underwater(don't want to drown)
                    if(bots[i].pEdict->v.waterlevel == WL_NOT_IN_WATER)
                        bots[i].f_soundReactTime = bots[i].f_think_time + RANDOM_FLOAT(3.0f, 4.0f);
                    else
                        bots[i].f_soundReactTime = bots[i].f_think_time + 5.0f;

                    //	WaypointDrawBeam(INDEXENT(1), bots[i].pEdict->v.origin,
                    //		pEdict->v.origin,
                    //		10, 2, 250, 50, 50, 200, 10);
                }
            }
        }
    }
}

// This function should provide a convenient way of clearing the bots memory
// of a particular pair of Teleporters it knows about.
void BotForgetTeleportPair(bot_t* pBot, const int index)
{
    if(index > -1 && index < MAX_BOT_TELEPORTER_MEMORY) {
        pBot->telePair[index].entrance = NULL;
        pBot->telePair[index].entranceWP = -1;
        pBot->telePair[index].exitWP = -1;
    }
}

// If the bot knows about a particular Teleporter entrance this function will return
// the index of the bot's Teleporter memory where this Teleporter entrance is stored.
// Or -1 if the bot hasn't encountered the Teleporter entrance before.
int BotRecallTeleportEntranceIndex(const bot_t* pBot, edict_t* const teleportEntrance)
{
    // just checking!
    if(teleportEntrance == NULL)
        return -1;

    for(int i = 0; i < MAX_BOT_TELEPORTER_MEMORY; i++) {
        if(pBot->telePair[i].entrance == teleportEntrance)
            return i;
    }

    return -1;
}

// This function searches the bots memory of teleporter pairs looking for
// an unused teleporter pair index, and returns it.
// Or -1 if none were free.
int BotGetFreeTeleportIndex(const bot_t* pBot)
{
    for(int i = 0; i < MAX_BOT_TELEPORTER_MEMORY; i++) {
        if(FNullEnt(pBot->telePair[i].entrance) || pBot->telePair[i].entranceWP == -1 || pBot->telePair[i].exitWP == -1)
            return i;
    }

    return -1;
}

// BotEntityAtPoint - By giving this function a vector to check from and a range
// to check, it will return the first entity with the specified name at the location
// passed, or NULL if none. This should be useful for stopping engineers
// building guns at the same defense points.
edict_t* BotEntityAtPoint(const char* entityName, const Vector& location, const float range)
{
    edict_t* pent = NULL;
    while((pent = FIND_ENTITY_IN_SPHERE(pent, location, range)) != NULL && (!FNullEnt(pent))) {
        // strcpy(item_name, STRING(pent->v.classname));
        if(strcmp(entityName, STRING(pent->v.classname)) == 0)
            return pent;
    }

    return NULL;
}

// This function returns the edict of any teammate who is within a specified radius
// of the indicated Vector.  You can specify whether or not the player found must
// be stationary.  This will return NULL if none were found.
edict_t* BotAllyAtVector(const bot_t* pBot, const Vector& r_vecOrigin, const float range, const bool stationaryOnly)
{
    edict_t* pPlayer;
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        pPlayer = INDEXENT(i);

        if(pPlayer != NULL && !pPlayer->free && pPlayer != pBot->pEdict // ignore this bot
            && UTIL_GetTeam(pPlayer) == pBot->current_team &&
            VectorsNearerThan(pPlayer->v.origin, r_vecOrigin, range)) {
            if(stationaryOnly) {
                if(pPlayer->v.velocity.Length() < 1.0)
                    return pPlayer;
            } else
                return pPlayer;
        }
    }

    return NULL;
}

// This function returns the number of a bots teammates who are
// heading towards the indicated waypoint and nearer to it than the
// bot calling this function.
short BotTeammatesNearWaypoint(const bot_t* pBot, const int waypoint)
{
    // sanity check
    if(waypoint < 0)
        return FALSE;

    // number of bots found nearer to the waypoint than this bot
    short total_present = 0;

    const float my_distance = (pBot->pEdict->v.origin - waypoints[waypoint].origin).Length();

    for(int i = 0; i < MAX_BOTS; i++) {
        // Is this player a bot teammate who is
        // heading towards the indicated waypoint?
        if(bots[i].is_used && &bots[i] != pBot // make sure the player isn't THIS bot
            && bots[i].current_wp == waypoint && bots[i].current_team == pBot->current_team) {
            // if this player is nearer than the bot add them to the total
            if(VectorsNearerThan(bots[i].pEdict->v.origin, waypoints[waypoint].origin, my_distance))
                ++total_present;
        }
    }

    return total_present;
}

// This function returns the bot_t address of a bot defender teammate
// who is heading towards the indicated waypoint and within a certain
// radius of it.  Or NULL if none were found.
// This function can help defenders avoid sharing a defense waypoint.
bot_t* BotDefenderAtWaypoint(const bot_t* pBot, const int waypoint, const float range)
{
    // sanity check
    if(waypoint < 0)
        return FALSE;

    for(int i = 0; i < MAX_BOTS; i++) {
        // Is this a bot defender teammate, with an interest in the
        // indicated waypoint?
        if(bots[i].is_used      // make sure this player is a bot
            && &bots[i] != pBot // make sure the player isn't THIS bot
            && bots[i].goto_wp == waypoint && bots[i].mission == ROLE_DEFENDER &&
            bots[i].current_team == pBot->current_team) {
            // if this player is near enough return who they are
            if(VectorsNearerThan(bots[i].pEdict->v.origin, waypoints[waypoint].origin, range))
                return &bots[i];
        }
    }

    return NULL;
}

// This is a modified version of a similar function from the HPB bot source code.
// It scans for live grenades that are in front of the bot and decides
// what the bot should do about them.
static void BotGrenadeAvoidance(bot_t* pBot)
{
    // don't scan too often (as it'll tax the CPU)
    if(pBot->f_grenadeScanTime > pBot->f_think_time)
        return;
    else
        pBot->f_grenadeScanTime = pBot->f_think_time + 0.35f;

    enum avoid_actions { avoid_no_action, avoid_retreat, avoid_strafe };
    short avoid_action = avoid_no_action;

    UTIL_MakeVectors(pBot->pEdict->v.v_angle);

    // put the search sphere in front of the bot
    Vector search_source = pBot->pEdict->v.origin + gpGlobals->v_forward * 120;

    // lower the search sphere, closer to the level of the bots feet
    search_source.z -= 20;

    /*	//visibly show the left and right edge of the scan area(for debugging purposes)
            Vector beam_end = search_source + (gpGlobals->v_forward * 120)
                    + (gpGlobals->v_right * -250);
            WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                    beam_end, 10, 2, 150, 250, 250, 200, 10);
            beam_end = search_source + (gpGlobals->v_forward * 120)
                    + (gpGlobals->v_right * 250);
            WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                    beam_end, 10, 2, 150, 250, 250, 200, 10);*/

    // start looking for known visible grenades in front of the bot
    char classname[30];
    Vector entity_origin;
    edict_t* pent = NULL;
    edict_t* threatEnt = NULL;
    while(avoid_action == avoid_no_action && (pent = FIND_ENTITY_IN_SPHERE(pent, search_source, 250)) != NULL &&
        !FNullEnt(pent)) {
        if(pent->v.classname == 0)
            continue;

        strncpy(classname, STRING(pent->v.classname), 30);
        classname[29] = '\0';

        // beware MIRVIN Marvin!
        if(strncmp("tf_weapon_mirvgrenade", classname, 29) == 0 ||
            strncmp("tf_weapon_mirvbomblet", classname, 29) == 0) {
            entity_origin = pent->v.origin;
            if(FInViewCone(entity_origin, pBot->pEdict) && FVisible(entity_origin, pBot->pEdict)) {
                avoid_action = avoid_retreat;
                threatEnt = pent;
            }
        } else if(strncmp("tf_weapon_gasgrenade", classname, 29) == 0) {
            entity_origin = pent->v.origin;
            // only need to check viewcone, the gas is visible through scenery
            if(static_cast<int>(pBot->pEdict->v.health) < 30 && FInViewCone(entity_origin, pBot->pEdict)) {
                avoid_action = avoid_retreat;
                threatEnt = pent;
            }

            /*			//only need to check viewcone, the gas is visible through scenery
                                    if(FInViewCone(entity_origin, pBot->pEdict)
                                            && pBot->goto_wp > -1)
                                    {
                                            // avoid gas grenades only if they are near the bots destination
                                            // (and their destination is not a flag or capture point)
                                            if((entity_origin - waypoints[pBot->goto_wp].origin).Length() < 180)
                                                    avoid_action = avoid_retreat;
                                    }*/
        } else if(strncmp("tf_weapon_normalgrenade", classname, 29) == 0 ||
            strncmp("tf_weapon_empgrenade", classname, 29) == 0) {
            entity_origin = pent->v.origin;
            if(FInViewCone(entity_origin, pBot->pEdict) && FVisible(entity_origin, pBot->pEdict)) {
                avoid_action = avoid_retreat;
                threatEnt = pent;
            }
        } else if(strncmp("tf_weapon_nailgrenade", classname, 29) == 0 ||
            strncmp("tf_weapon_napalmgrenade", classname, 29) == 0 ||
            (strncmp("tf_gl_grenade", classname, 29) == 0 && pBot->pEdict->v.playerclass != TFC_CLASS_DEMOMAN)) {
            entity_origin = pent->v.origin;
            if(FInViewCone(entity_origin, pBot->pEdict) && FVisible(entity_origin, pBot->pEdict)) {
                if(pBot->trait.aggression < 33 || static_cast<int>(pBot->pEdict->v.health) > 70)
                    pBot->pEdict->v.button |= IN_JUMP; // try jumping the grenade anyway
                else                                   // low health or aggression - retreat!
                {
                    avoid_action = avoid_retreat;
                    threatEnt = pent;
                }
            }
        } else if(strncmp("tf_gl_pipebomb", classname, 29) == 0) {
            const int owner_team = UTIL_GetTeam(pent->v.owner);

            // try to get over or around the pipebomb if an enemy fired it
            if(owner_team != pBot->current_team && !(team_allies[pBot->current_team] & (1 << owner_team))) {
                //	UTIL_HostSay(pBot->pEdict, 0, "enemy PIPEBOMB spotted!");//DebugMessageOfDoom!

                entity_origin = pent->v.origin;
                if(FInViewCone(entity_origin, pBot->pEdict) && FVisible(entity_origin, pBot->pEdict)) {
                    pBot->pEdict->v.button |= IN_JUMP;
                    return;
                }
            }
        } else if(strncmp("tf_weapon_caltrop", classname, 29) == 0 && pBot->bot_skill < 3) {
            entity_origin = pent->v.origin;
            if(FInViewCone(entity_origin, pBot->pEdict) && FVisible(entity_origin, pBot->pEdict))
                pBot->pEdict->v.button |= IN_JUMP;
        }
    }

    if(avoid_action == avoid_retreat && threatEnt != NULL) {
        job_struct* newJob = InitialiseNewJob(pBot, JOB_AVOID_AREA_DAMAGE);
        if(newJob != NULL) {
            newJob->object = threatEnt;           // remember the object
            newJob->origin = threatEnt->v.origin; // remember the object's origin too
            SubmitNewJob(pBot, JOB_AVOID_AREA_DAMAGE, newJob);
        }
    }
}

// This function periodically looks at the number of attackers
// and defenders on each team, and attempts to balance them out,
// by making bots switch combat roles.
static void BotRoleCheck(bot_t* pBot)
{
    if(roleCheckTimer > gpGlobals->time &&
        roleCheckTimer < (gpGlobals->time + 300.0)) // make sure roleCheckTimer is sane
        return;

    roleCheckTimer = gpGlobals->time + 10.0f; // reset the timer

    static TeamLayout teams;

    // Clear our knowledge of each teams composition
    int i;
    for(i = 0; i < 4; i++) {
        teams.attackers[i].clear();
        teams.defenders[i].clear();
        teams.humanAttackers[i].clear();
        teams.humanDefenders[i].clear();
        teams.total[i] = 0;
    }

    // Count the number of bots on each team that are
    // attacking or defending
    for(i = 0; i < MAX_BOTS; i++) {
        // Create lists of the defender/attacker roles any bots are playing
        if(bots[i].is_used && bots[i].pEdict->v.playerclass) {
            teams.total[bots[i].pEdict->v.team - 1]++;
            if(bots[i].mission == ROLE_DEFENDER)
                teams.defenders[bots[i].pEdict->v.team - 1].addTail(&bots[i]);
            else if(bots[i].mission == ROLE_ATTACKER)
                teams.attackers[bots[i].pEdict->v.team - 1].addTail(&bots[i]);
            else if(bots[i].mission == ROLE_NONE)
                bots[i].mission = ROLE_ATTACKER;
        }
    }

    // Guess at the roles any human players have taken
    for(i = 0; i < 32; i++) {
        if(clients[i]) {
            teams.total[clients[i]->v.team - 1]++;
            if(clients[i]->v.playerclass == TFC_CLASS_MEDIC || clients[i]->v.playerclass == TFC_CLASS_SPY ||
                clients[i]->v.playerclass == TFC_CLASS_SCOUT || clients[i]->v.playerclass == TFC_CLASS_PYRO)
                teams.humanAttackers[clients[i]->v.team - 1].addTail(clients[i]);
            else if(clients[i]->v.playerclass == TFC_CLASS_ENGINEER || clients[i]->v.playerclass == TFC_CLASS_SNIPER)
                teams.humanDefenders[clients[i]->v.team - 1].addTail(clients[i]);
        }
    }
    /*char msg[255];
            sprintf(msg, "Blue: %d Attack, %d Defend, %d Total",
                    teams.attackers[0].size(), teams.defenders[0].size(), teams.total[0]);
            UTIL_HostSay(pBot->pEdict, 0, msg);
            sprintf(msg, "Red: %d Attack, %d Defend, %d Total",
                    teams.attackers[1].size(), teams.defenders[1].size(), teams.total[1]);
            UTIL_HostSay(pBot->pEdict, 0, msg);*/

    short team;

    // need to make some adjustments to the roles based on the above figures.
    // Only do this 1 bot per team. Don't want mass role changes.
    for(team = 0; team < 4; team++) {
        // Crash prevention //
        if(teams.total[team] == 0)
            continue;
        // end crash prevention //

        // figure out what percentage of each team a single player represents
        const float playerPercentage = (1.0f / static_cast<float>(teams.total[team])) * 100.0f;

        // figure out an error margin based on the player percentage divided by 2
        // useful when a team has an odd number of players
        const float errorMargin = playerPercentage / 2.0f;

        const int totalAttackers = teams.attackers[team].size() + teams.humanAttackers[team].size();
        const float percentOffense = playerPercentage * totalAttackers;

        char needed_mission;

        // decide if attackers or defenders are needed
        if((percentOffense - errorMargin) > static_cast<float>(RoleStatus[team]))
            needed_mission = ROLE_DEFENDER; // defender needed
        else if(percentOffense < (static_cast<float>(RoleStatus[team]) - errorMargin))
            needed_mission = ROLE_ATTACKER; // attacker needed
        else
            continue; // this team is balanced - leave it alone

        // find out if Demomen should be left as attackers if the map
        // has active detpack waypoints
        bool ignoreDemomen = FALSE;
        if(needed_mission == ROLE_DEFENDER &&
            WaypointTypeExists(W_FL_TFC_DETPACK_CLEAR | W_FL_TFC_DETPACK_SEAL, team) == TRUE)
            ignoreDemomen = TRUE;

        // start the search from a randomly selected bot
        i = random_long(0, MAX_BOTS - 1);

        // look for suitable bots
        for(int count = 0; count < MAX_BOTS; count++, i++) {
            if(i >= MAX_BOTS)
                i = 0; // wrap the search if necessary

            // found a maleable bot, ripe for initiation?
            if(bots[i].is_used && bots[i].pEdict->v.playerclass && bots[i].current_team == team &&
                bots[i].mission != needed_mission && !bots[i].lockMission && !bots[i].bot_has_flag) {
                // these classes are good candidates for switching between attack and defense
                // Engineers are also suitable here because EMP and armor repair is cool
                if(bots[i].pEdict->v.playerclass == TFC_CLASS_SOLDIER ||
                    bots[i].pEdict->v.playerclass == TFC_CLASS_PYRO ||
                    bots[i].pEdict->v.playerclass == TFC_CLASS_HWGUY ||
                    bots[i].pEdict->v.playerclass == TFC_CLASS_ENGINEER ||
                    (bots[i].pEdict->v.playerclass == TFC_CLASS_DEMOMAN && !ignoreDemomen)) {
                    bots[i].mission = needed_mission;
                    /*	char msg[80];//DebugMessageOfDoom!
                            sprintf(msg, "team %d needed_mission %d totalAttackers %d",
                                    team + 1, (int)needed_mission, totalAttackers);
                            UTIL_HostSay(bots[i].pEdict, 0, msg);*/
                    break; // found what we're looking for
                }
            }
        }
    }
}

// BotComms()
// Probably called from BotThink().
// This function exists merely to shrink BotThink() by moving some
// of the bot communication code elsewhere.
static void BotComms(bot_t* pBot)
{
    edict_t* pEdict = pBot->pEdict;

    // RedFox bot coms
    if(pBot->newmsg && pBot->f_periodicAlert1 < pBot->f_think_time) {
        pBot->newmsg = FALSE;

        char buffa[255];
        // char buffb[255];
        char cmd[255];
        int j, k = 1; // loop counters

        strcpy(buffa, pBot->message);
        while(k) {
            // remove start spaces
            j = 0;
            while(buffa[j] == ' ' || buffa[j] == '\n') {
                j++;
            }
            k = 0;
            while(buffa[j] != ' ' && buffa[j] != '\0' && buffa[j] != '\n') {
                cmd[k] = buffa[j];
                buffa[j] = ' ';
                j++;
                k++;
            }
            cmd[k] = '\0';

            // strcpy(buffb, cmd);
            if((stricmp("changeclass", cmd) == 0) || (stricmp("changeclassnow", cmd) == 0)) {
                // Check if this message is from another player on our team.
                char fromName[255] = { 0 };
                if(_strnicmp("(TEAM)", pBot->message + 1, 6) == 0)
                    strcpy(fromName, (pBot->message + 8));
                else
                    strcpy(fromName, pBot->message);

                int counter = 0;
                // Pull the name out of the message.
                while(true) {
                    if(fromName[counter] == ':' && fromName[counter + 1] == ' ') {
                        fromName[counter] = '\0';
                        break;
                    }
                    counter++;
                }

                // Get the class from the command line.
                char theClass = pBot->message[strlen(pBot->message) - 2];
                if(theClass != '\0' && strchr("123456789", theClass)) {
                    int iClass = atoi(&theClass);
                    if(BotChangeClass(pBot, iClass, fromName)) {
                        UTIL_HostSay(pBot->pEdict, 1, "Roger");
                        if(stricmp("changeclassnow", cmd) == 0)
                            FakeClientCommand(pBot->pEdict, "kill", NULL, NULL);
                    }
                }
            }
            if((stricmp("changerole", cmd) == 0) || (stricmp("changerolenow", cmd) == 0)) {
                // Check if this message is from another player on our team.
                char fromName[255] = { 0 };
                if(_strnicmp("(TEAM)", pBot->message + 1, 6) == 0)
                    strcpy(fromName, (pBot->message + 8));
                else
                    strcpy(fromName, pBot->message);

                int counter = 0;
                // Pull the name out of the message.
                while(true) {
                    if(fromName[counter] == ':' && fromName[counter + 1] == ' ') {
                        fromName[counter] = '\0';
                        break;
                    }
                    counter++;
                }
                BotChangeRole(pBot, pBot->message, fromName);
            }
            /*	if(stricmp("follow", cmd) == 0)
                    {
                            UTIL_HostSay(pEdict, 0, "Follow mode");
                    //	pBot->follow = TRUE;
                            pBot->tracked_player = NULL;
                    //	pBot->track_ttime = pBot->f_think_time;
                    }*/
            if(stricmp("die", cmd) == 0) {
                EMIT_SOUND_DYN2(pBot->pEdict, CHAN_VOICE, "barney/c1a4_ba_octo4.wav", 0, 0, SND_STOP, PITCH_NORM);
                EMIT_SOUND_DYN2(pBot->pEdict, CHAN_VOICE, "barney/c1a4_ba_octo4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
            }
            /*if(stricmp("roam", cmd) == 0)
            {
                    UTIL_HostSay(pEdict, 0, "Roam mode");
                    pBot->follow = FALSE;
                    pBot->tracked_player = NULL;
                    pBot->enemy.ptr = NULL;
            }*/
            if(stricmp("xmas", cmd) == 0 && bot_xmas) {
                // TraceResult tr;
                short model;

                UTIL_MakeVectors(pEdict->v.v_angle);
                Vector vecDir = gpGlobals->v_forward;

                /*UTIL_TraceLine( pEdict->v.origin,
                        pEdict->v.origin+vecDir*256,
                        dont_ignore_monsters, NULL, &tr);*/

                if(random_long(0, 10) < 5)
                    model = PRECACHE_MODEL("models/presentlg.mdl");
                else
                    model = PRECACHE_MODEL("models/presentsm.mdl");
                MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pEdict->v.origin);
                WRITE_BYTE(TE_EXPLODEMODEL);
                WRITE_COORD(pEdict->v.origin.x);
                WRITE_COORD(pEdict->v.origin.y);
                WRITE_COORD(pEdict->v.origin.z);
                WRITE_COORD(random_long(100, 1000));
                WRITE_SHORT(model);
                WRITE_SHORT(random_long(2, 6));
                WRITE_BYTE(random_long(20, 60));
                MESSAGE_END();
                if(random_long(0, 100) <= 50) {
                    EMIT_SOUND_DYN2(pBot->pEdict, CHAN_VOICE, "misc/b2.wav", 1.0, ATTN_NORM, 0, 100);
                } else if(random_long(0, 100) <= 50) {
                    EMIT_SOUND_DYN2(pBot->pEdict, CHAN_VOICE, "misc/party2.wav", 1.0, ATTN_NORM, 0, 100);
                } else if(random_long(0, 100) <= 50) {
                    EMIT_SOUND_DYN2(pBot->pEdict, CHAN_VOICE, "misc/party1.wav", 1.0, ATTN_NORM, 0, 100);
                }
            }
        }
    } // end of bot message handling

    // chat related stuff below, ignore if already chatting
    // or if the bot has no current waypoint
    job_struct* newJob = InitialiseNewJob(pBot, JOB_CHAT);
    if(newJob == NULL || pBot->current_wp < 0)
        return;

    // greetings from the bots
    if(pBot->greeting == FALSE && (pBot->create_time + 3.0) > pBot->f_think_time) {
        pBot->greeting = TRUE;
        if(random_long(1, 1000) < static_cast<long>(botchat)) {
            chat.pickRandomChatString(newJob->message, MAX_CHAT_LENGTH, CHAT_TYPE_GREETING, NULL);

            newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
            SubmitNewJob(pBot, JOB_CHAT, newJob);
        }
        if(bot_xmas) {
            strcpy(pBot->message, "xmas");
            pBot->newmsg = TRUE;
        }
        return;
    }

    // my chat, bot has died
    // good idea to check for null...urg!
    if(pBot->killer_edict != NULL && random_long(1, 1000) < static_cast<long>(botchat)) {
        if(pBot->killer_edict->v.frags > pEdict->v.frags) {
            chat.pickRandomChatString(
                newJob->message, MAX_CHAT_LENGTH, CHAT_TYPE_KILLED_LOW, STRING(pBot->killer_edict->v.netname));
        } else {
            chat.pickRandomChatString(
                newJob->message, MAX_CHAT_LENGTH, CHAT_TYPE_KILLED_HI, STRING(pBot->killer_edict->v.netname));
        }

        newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
        SubmitNewJob(pBot, JOB_CHAT, newJob);

        if(bot_xmas && random_long(1, 100) <= 30 && pBot->killer_edict != NULL) {
            strcpy(pBot->message, "xmas");
            pBot->newmsg = TRUE;
        }
        pBot->killer_edict = NULL;
    } else
        pBot->killer_edict = NULL;

    // haha I killed you messages
    if(pBot->killed_edict != NULL && random_long(1, 1000) < static_cast<long>(botchat)) {
        if(pBot->killed_edict->v.frags > pEdict->v.frags) {
            chat.pickRandomChatString(
                newJob->message, MAX_CHAT_LENGTH, CHAT_TYPE_KILL_LOW, STRING(pBot->killed_edict->v.netname));
        } else {
            chat.pickRandomChatString(
                newJob->message, MAX_CHAT_LENGTH, CHAT_TYPE_KILL_HI, STRING(pBot->killed_edict->v.netname));
        }

        newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
        SubmitNewJob(pBot, JOB_CHAT, newJob);

        if(bot_xmas && random_long(1, 100) <= 30 && pBot->killed_edict != NULL) {
            strcpy(pBot->message, "xmas");
            pBot->newmsg = TRUE;
        }
        pBot->killed_edict = NULL;
    } else
        pBot->killed_edict = NULL;
}

// BotChangeClass - Given a bot and a class #, this function should
// make the bot change to that class.
// This function is used primarily when a human tells a bot to change class.
// Returns true if successful.
static bool BotChangeClass(bot_t* pBot, int iClass, const char* from)
{
    // Check for invalid input
    if(iClass > 10 || iClass < 1)
        return FALSE;

    int class_not_allowed;

    // Check if the class supplied is already at the limit.
    if(iClass <= 7)
        class_not_allowed = team_class_limits[pBot->current_team] & (1 << (iClass - 1));
    else
        class_not_allowed = team_class_limits[pBot->current_team] & (1 << (iClass));

    if(class_not_allowed)
        return FALSE;

    // Make sure its from a teammate
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pPlayer = INDEXENT(i);
        if(!pPlayer)
            continue;

        // Is this the guys name that sent the command?
        if(strcmp(STRING(pPlayer->v.netname), from) == 0) {
            // Make sure he is on our team to accept the message.
            if(pBot->current_team != UTIL_GetTeam(pPlayer))
                return FALSE;

            // Check the pPlayer for access to this command.
            if(!botVerifyAccess(pPlayer)) {
                UTIL_HostSay(pBot->pEdict, 1, "You don't have bot commander access.");
                return FALSE;
            }

            /*char msg[80];
               sprintf(msg, "wonid: %d", wonId);
               UTIL_HostSay(pBot->pEdict, 0, msg);*/

            // A-OK Do it.
            pBot->bot_class = iClass;
            char c_class[128];
            switch(pBot->bot_class) {
            case 0:
                strcpy(c_class, "civilian");
                break;
            case 1:
                strcpy(c_class, "scout");
                break;
            case 2:
                strcpy(c_class, "sniper");
                break;
            case 3:
                strcpy(c_class, "soldier");
                break;
            case 4:
                strcpy(c_class, "demoman");
                break;
            case 5:
                strcpy(c_class, "medic");
                break;
            case 6:
                strcpy(c_class, "hwguy");
                break;
            case 7:
                strcpy(c_class, "pyro");
                break;
            case 8:
                strcpy(c_class, "spy");
                break;
            case 9:
                strcpy(c_class, "engineer");
                break;
            default:
                return FALSE;
            }

            FakeClientCommand(pBot->pEdict, c_class, NULL, NULL);
            pBot->lockClass = TRUE;
        }
    }
    return TRUE;
}

// BotChangeRole - Given a bot and a role, this function should make
// the bot change to that role.
// This function is used primarily when a human tells a bot to change role.
// Returns true if successful.
static bool BotChangeRole(bot_t* pBot, const char* cmdLine, const char* from)
{
    // Crash check.
    if(!pBot || !cmdLine || !from)
        return FALSE;

    // Check if the class supplied is already at the limit.

    // Make sure its from a teammate
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pPlayer = INDEXENT(i);
        if(!pPlayer)
            continue;

        // Is this the guys name that sent the command?
        if(strcmp(STRING(pPlayer->v.netname), from) == 0) {
            // Make sure he is on our team to accept the message.
            if(pBot->current_team != UTIL_GetTeam(pPlayer))
                return FALSE;

            // Check the pPlayer for access to this command.
            if(!botVerifyAccess(pPlayer)) {
                UTIL_HostSay(pBot->pEdict, 1, "You don't have access.");
                return FALSE;
            }

            // Get the role to change to first from the cmdLine line
            // int changeTo = 0;
            if(strstr(cmdLine, "attack")) {
                pBot->mission = ROLE_ATTACKER;
                pBot->lockMission = TRUE;
                UTIL_HostSay(pBot->pEdict, 1, "Roger, attacking.");
            } else if(strstr(cmdLine, "defend")) {
                pBot->mission = ROLE_DEFENDER;
                pBot->lockMission = TRUE;
                UTIL_HostSay(pBot->pEdict, 1, "Roger, defending.");
            } else if(strstr(cmdLine, "roam")) {
                pBot->lockMission = FALSE;
                UTIL_HostSay(pBot->pEdict, 1, "Roger, I'm on my own.");
            } else
                UTIL_HostSay(pBot->pEdict, 1, "Invalid Role.");
        }
    }
    return TRUE;
}

// This function checks if the specified user has access through
// the foxbot_commanders.txt
static bool botVerifyAccess(edict_t* pPlayer)
{
    char authId[255];
    strcpy(authId, GETPLAYERAUTHID(pPlayer));

    char szBuffer[64];
    _snprintf(szBuffer, 63, "%s Does not have access.", authId);
    szBuffer[63] = '\0'; // just to be sure
    ALERT(at_console, szBuffer);
    // example steam ID: STEAM_0:1245

    bool found = FALSE;
    LIter<char*> iter(&commanders);
    for(iter.begin(); !iter.end(); ++iter) {
        if(stricmp(authId, (*iter.current())) == 0)
            found = TRUE;
    }

    if(found)
        return TRUE;
    else
        return FALSE;
}

// BotPickNewClass().
// If a bot is allowed to pick a new role or class then this function
// can make it do so when it dies.
// This function should be called each time a bot dies and before
// it respawns.
static void BotPickNewClass(bot_t* pBot)
{
    // check if the bot is allowed to autonomously change class
    if(pBot->lockClass || mod_id != TFC_DLL)
        return;

    static const short defense_classes[] = { 2, 3, 4, 6, 7, 9 };
    static const short offense_classes[] = { 1, 3, 4, 5, 6, 7, 8, 9 };

    // reward/penalise bots based on how they scored until they last died
    if(static_cast<int>(pBot->pEdict->v.frags) > pBot->scoreAtSpawn) {
        // reward bots who scored more than 1 point
        if((static_cast<int>(pBot->pEdict->v.frags) - pBot->scoreAtSpawn) > 1 && pBot->deathsTillClassChange < 10)
            ++pBot->deathsTillClassChange;
    } else if(pBot->deathsTillClassChange > 0)
        --pBot->deathsTillClassChange;

    /*	if(pBot->deathsTillClassChange < 1
                    || pBot->deathsTillClassChange > 9)
            {
                    char msg[80];
                    sprintf(msg, "ALERT:deaths left %hd, scored %d",
                            pBot->deathsTillClassChange, (int)pBot->pEdict->v.frags - pBot->scoreAtSpawn);
                    UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!
            }
            else
            {
                    char msg[80];
                    sprintf(msg, "deaths left %hd, scored %d",
                            pBot->deathsTillClassChange, (int)pBot->pEdict->v.frags - pBot->scoreAtSpawn);
                    UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!
            }*/

    // don't change class unless the bot has been killed
    // one time too many
    if(pBot->deathsTillClassChange > 0) {
        // if HWguys are doing too well make them change class too
        if(!(pBot->pEdict->v.playerclass == TFC_CLASS_HWGUY && pBot->deathsTillClassChange > 9))
            return;
    }

    // see if the bot should pick a class suitable to a given situation
    if(random_long(1, 1000) > 300) {
        if(BotDemomanNeededCheck(pBot) == TRUE)
            return;

        if(BotChooseCounterClass(pBot) == TRUE)
            return;
    }

    bool defender_required = 0;

    // Analyse the composition of this bots team
    short i, attackers = 0, team_total = 0;
    for(i = 0; i < 32; i++) {
        // Tally the number of bots on defender or attacker roles
        if(bots[i].is_used) {
            if(bots[i].pEdict->v.playerclass && (bots[i].pEdict->v.team - 1) == pBot->current_team) {
                ++team_total;
                if(bots[i].mission == ROLE_ATTACKER)
                    ++attackers;
                //	else if(bots[i].mission == ROLE_DEFENDER) ++defenders;
                else if(bots[i].mission == ROLE_NONE) {
                    bots[i].mission = ROLE_ATTACKER;
                    ++attackers;
                }
            }
            continue; // don't check below if this player is a human
        }

        // Guess at the roles any human players have taken
        if(clients[i] && (clients[i]->v.team - 1) == pBot->current_team) {
            ++team_total;
            if(clients[i]->v.playerclass == TFC_CLASS_MEDIC || clients[i]->v.playerclass == TFC_CLASS_SPY ||
                clients[i]->v.playerclass == TFC_CLASS_SCOUT || clients[i]->v.playerclass == TFC_CLASS_PYRO)
                ++attackers;
            /*	else if(clients[i]->v.playerclass == TFC_CLASS_ENGINEER
                            || clients[i]->v.playerclass == TFC_CLASS_SNIPER)
                            ++defenders;*/
        }
    }

    // figure out what percentage of each team a single player represents
    float playerPercentage = (1.0f / static_cast<float>(team_total)) * 100.0f;

    // figure out an error margin based on the player percentage divided by 2
    // useful when a team has an odd number of players
    float errorMargin = playerPercentage / 2.0f;

    float percentOffense = playerPercentage * static_cast<float>(attackers);

    // does this team have too few defenders?
    if((percentOffense - errorMargin) > static_cast<float>(RoleStatus[pBot->current_team]))
        defender_required = 1;

    /*	//debug message: report what the bot is doing
            if(defender_required)
            {
            char msg[80];
            sprintf(msg, "[pickin defense] RoleStatus %d totalAttackers %d",
                    RoleStatus[team], attackers);
            UTIL_HostSay(pBot->pEdict, 0, msg);
            }
            else
            {
            char msg[80];
            sprintf(msg, "[pickin offense] RoleStatus %d totalAttackers %d",
                    RoleStatus[team], attackers);
            UTIL_HostSay(pBot->pEdict, 0, msg);
            }*/

    int new_class;
    if(defender_required)
        new_class = defense_classes[random_long(0, 5)];
    else
        new_class = offense_classes[random_long(0, 6)];

    // Check if the chosen class is already at the limit.
    int class_not_allowed;
    if(new_class <= 7)
        class_not_allowed = team_class_limits[pBot->current_team] & (1 << (new_class - 1));
    else
        class_not_allowed = team_class_limits[pBot->current_team] & (1 << (new_class));

    if(class_not_allowed || new_class == pBot->bot_class)
        return;

    // name the class the bot wishes to use and change to it
    char c_class[16];
    switch(new_class) {
    case 1:
        strcpy(c_class, "scout");
        break;
    case 2:
        strcpy(c_class, "sniper");
        break;
    case 3:
        strcpy(c_class, "soldier");
        break;
    case 4:
        strcpy(c_class, "demoman");
        break;
    case 5:
        strcpy(c_class, "medic");
        break;
    case 6:
        strcpy(c_class, "hwguy");
        break;
    case 7:
        strcpy(c_class, "pyro");
        break;
    case 8:
        strcpy(c_class, "spy");
        break;
    case 9:
        strcpy(c_class, "engineer");
        break;
    default:
        break;
    }
    pBot->bot_class = new_class;
    FakeClientCommand(pBot->pEdict, c_class, NULL, NULL);

    // decide it's role
    if(defender_required)
        pBot->mission = ROLE_DEFENDER;
    else
        pBot->mission = ROLE_ATTACKER;

    // delay changing class again for a while
    if(new_class == pBot->trait.faveClass)
        pBot->deathsTillClassChange = 8;
    else
        pBot->deathsTillClassChange = 4;
}

// This function allows bots to pick a class based upon which class last killed them.
// It allows bots which are getting beaten severely to select classes that
// may counteract certain classes that their enemies are using.
static bool BotChooseCounterClass(bot_t* pBot)
{
    if(FNullEnt(pBot->killer_edict))
        return FALSE;

    short new_class = -1;

    // choose the Spy class if the bot was killed by a Sentry Gun and the team
    // has few spies already
    if(pBot->killer_edict == pBot->lastEnemySentryGun && !FNullEnt(pBot->lastEnemySentryGun)) {
        if(FriendlyClassTotal(pBot->pEdict, TFC_CLASS_SPY, FALSE) < 1)
            new_class = 8;
    } else // enemy is not a sentry gun
    {
        // try to choose a counter class if the enemy who killed the bot
        // has a significantly better score than the bot
        if(pBot->killer_edict->v.frags > 4 && (pBot->pEdict->v.frags + 5) < pBot->killer_edict->v.frags) {
            // Snipers are often a serious threat so pick Sniper or HWGuy
            // to fight back against them
            if(pBot->killer_edict->v.playerclass == TFC_CLASS_SNIPER) {
                if(FriendlyClassTotal(pBot->pEdict, TFC_CLASS_SNIPER, FALSE) < 2) {
                    new_class = 2;
                    //	UTIL_HostSay(pBot->pEdict, 0, "Counter classing a Sniper"); //DebugMessageOfDoom
                } else if(FriendlyClassTotal(pBot->pEdict, TFC_CLASS_HWGUY, FALSE) < 1) {
                    new_class = 6;
                    //	UTIL_HostSay(pBot->pEdict, 0, "Counter classing a Sniper"); //DebugMessageOfDoom
                }
            }
            // a good counter-class for enemy Engineers is the sneaky Spy
            else if(pBot->killer_edict->v.playerclass == TFC_CLASS_ENGINEER) {
                if(FriendlyClassTotal(pBot->pEdict, TFC_CLASS_SPY, FALSE) < 1) {
                    new_class = 8;
                    //	UTIL_HostSay(pBot->pEdict, 0, "Counter classing an Engineer"); //DebugMessageOfDoom
                }
            }
            // not enjoying bio-chemical warfare?  Pick the guy with an antidote
            else if(pBot->killer_edict->v.playerclass == TFC_CLASS_MEDIC) {
                if(FriendlyClassTotal(pBot->pEdict, TFC_CLASS_MEDIC, FALSE) < 1) {
                    new_class = 5;
                    //	UTIL_HostSay(pBot->pEdict, 0, "Counter classing a Medic"); //DebugMessageOfDoom
                }
            }
        }
    }

    // not chosen a counter-class?
    if(new_class == -1)
        return FALSE;

    // Check if the chosen class is already at the limit.
    int class_not_allowed;
    if(new_class <= 7)
        class_not_allowed = team_class_limits[pBot->current_team] & (1 << (new_class - 1));
    else
        class_not_allowed = team_class_limits[pBot->current_team] & (1 << (new_class));

    if(class_not_allowed || new_class == pBot->bot_class)
        return FALSE;

    // name the class the bot wishes to use and change to it
    char c_class[16];
    switch(new_class) {
    case 1:
        strcpy(c_class, "scout");
        break;
    case 2:
        strcpy(c_class, "sniper");
        break;
    case 3:
        strcpy(c_class, "soldier");
        break;
    case 4:
        strcpy(c_class, "demoman");
        break;
    case 5:
        strcpy(c_class, "medic");
        break;
    case 6:
        strcpy(c_class, "hwguy");
        break;
    case 7:
        strcpy(c_class, "pyro");
        break;
    case 8:
        strcpy(c_class, "spy");
        break;
    case 9:
        strcpy(c_class, "engineer");
        break;
    default:
        return FALSE;
    }
    pBot->bot_class = new_class;
    FakeClientCommand(pBot->pEdict, c_class, NULL, NULL);

    pBot->mission = ROLE_ATTACKER; // reset the mission to it's default

    // delay changing class again for a while
    if(new_class == pBot->trait.faveClass)
        pBot->deathsTillClassChange = 8;
    else
        pBot->deathsTillClassChange = 4;
    return TRUE;
}

// This function allows a bot that has just been fragged to decide whether
// or not it should switch to Demoman in order to blow up any detpack waypoints.
// It returns TRUE if the bot switched class to Demoman.
static bool BotDemomanNeededCheck(bot_t* pBot)
{
    // give up if there is a demoman on the bots team already
    if(pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN ||
        FriendlyClassTotal(pBot->pEdict, TFC_CLASS_DEMOMAN, FALSE) > 0)
        return FALSE;

    // Check if any more demoman are allowed on this map.
    const int class_not_allowed = team_class_limits[pBot->current_team] & (1 << (TFC_CLASS_DEMOMAN - 1));

    if(class_not_allowed)
        return FALSE;

    //	char msg[80];
    //	sprintf(msg, "Checking if Demo needed, our score:%d, theirs:%d>",
    //		MatchScores[pBot->current_team], MatchScores[enemy_team]);
    //	UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!

    if(WaypointFindDetpackGoal(pBot->current_wp, pBot->current_team) != -1) {
        pBot->bot_class = TFC_CLASS_DEMOMAN;
        FakeClientCommand(pBot->pEdict, "demoman", NULL, NULL);

        // delay changing class again for a while
        pBot->deathsTillClassChange = 6;

        return TRUE;
    }

    return FALSE;
}

// This function was created so that Spys can check if they are in an
// area suitable for feigning and ambushing enemy players.
//	This function checks if the bot is in a narrow area and is near
// enough to an enemy and returns TRUE if so.
// On success r_wallVector will remember the position of a nearby wall.
bool SpyAmbushAreaCheck(bot_t* pBot, Vector& r_wallVector)
{
    // perform some basic checks first
    // e.g. don't feign near a lift
    if(pBot->pEdict->v.waterlevel != WL_NOT_IN_WATER || pBot->nadePrimed == TRUE || pBot->bot_has_flag ||
        PlayerIsInfected(pBot->pEdict) || (pBot->current_wp > -1 && waypoints[pBot->current_wp].flags & W_FL_LIFT)) {
        return FALSE;
    }

    // see if there are any enemies near enough to this ambush location
    // ideally the bot would listen out for possible new victims
    // for example, if an enemy is moving fast assume the bot can hear them
    edict_t* pPlayer;
    int player_team;
    bool enemies_too_far = TRUE;
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if(pPlayer && !pPlayer->free && pPlayer != pBot->pEdict && IsAlive(pPlayer)) {
            // ignore humans in observer mode
            if(b_observer_mode && !(pPlayer->v.flags & FL_FAKECLIENT))
                continue;

            // ignore allied players
            player_team = UTIL_GetTeam(pPlayer);
            if(player_team > -1 &&
                (player_team == pBot->current_team || team_allies[pBot->current_team] & (1 << player_team)))
                continue;

            if(VectorsNearerThan(pPlayer->v.origin, pBot->pEdict->v.origin, 1200.0f)) {
                enemies_too_far = FALSE;
                break;
            }
        }
    }

    // abort if no enemies are near enough
    if(enemies_too_far)
        return FALSE;

    // this is used so the bot can remember any one of the two walls a traceline hit
    // i.e. the wall on the left or the one on the right of the bot
    bool rememberLeftWall = TRUE;
    if(random_long(1, 1000) < 501)
        rememberLeftWall = FALSE;

    // Now start checking that the bot is in some kind of narrow area.
    Vector v_src = pBot->pEdict->v.origin;
    TraceResult tr;

    UTIL_MakeVectors(pBot->pEdict->v.v_angle);

    // do a trace to the right of the bot
    Vector endpoint = v_src + gpGlobals->v_right * 260;
    UTIL_TraceLine(v_src, endpoint, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

    // report a failure if the trace didn't hit anything
    if(tr.flFraction >= 1.0)
        return FALSE;

    if(!rememberLeftWall)
        r_wallVector = tr.vecEndPos;

    // do a trace to the left of the bot
    endpoint = v_src + gpGlobals->v_right * -260;
    UTIL_TraceLine(v_src, endpoint, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

    // report a failure if the trace didn't hit anything
    if(tr.flFraction >= 1.0)
        return FALSE;

    if(rememberLeftWall)
        r_wallVector = tr.vecEndPos;

    // both traces hit something, so report success because the bot may
    // be in a narrow passage
    return TRUE;
}

// This function is responsible for making bots that dropped the flag report
// where(if they know where) it was dropped.
// You should call this function if you know this bot died whilst carrying a flag.
static void BotReportMyFlagDrop(bot_t* pBot)
{
    if(!pBot->bot_has_flag)
        return;

    bool flag_was_dropped = FALSE;

    // check if the flag still exists where the bot died
    edict_t* pent = NULL;
    while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "item_tfgoal")) != NULL && (!FNullEnt(pent))) {
        if(pBot->flag_impulse == pent->v.impulse) {
            const float ll = (pent->v.origin - pBot->pEdict->v.origin).Length();
            if(ll > 0 && ll < 64.0f) {
                flag_was_dropped = TRUE;
                break;
            }
        }
    }

    if(!flag_was_dropped) {
        pent = NULL;
        while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "info_tfgoal")) != NULL && (!FNullEnt(pent))) {
            if(pBot->flag_impulse == pent->v.impulse) {
                const float ll = (pent->v.origin - pBot->pEdict->v.origin).Length();
                if(ll > 0 && ll < 64.0f) {
                    flag_was_dropped = TRUE;
                    break;
                }
            }
        }
    }

    if(flag_was_dropped)
        pBot->bot_has_flag = FALSE;
    else
        return;

    // set up a job to retrieve the flag
    job_struct* newJob = InitialiseNewJob(pBot, JOB_PICKUP_FLAG);
    if(newJob != NULL) {
        newJob->waypoint = WaypointFindNearest_E(pent, 600.0, pBot->current_team);
        newJob->object = pent;
        newJob->origin = pent->v.origin;
        SubmitNewJob(pBot, JOB_PICKUP_FLAG, newJob);
    }

    // let the other players know where the flag was dropped
    const int bot_area = AreaInsideClosest(pBot->pEdict);
    if(offensive_chatter && bot_area != -1) {
        // give the other bots on the bot's team the same JOB_PICKUP_FLAG
        // job if they're interested
        if(newJob != NULL) {
            for(int i = 0; i < 32; i++) {
                if(bots[i].is_used && &bots[i] != pBot // not this bot
                    && bots[i].bot_has_flag == FALSE && bots[i].mission == ROLE_ATTACKER &&
                    bots[i].current_team == pBot->current_team) {
                    SubmitNewJob(&bots[i], JOB_PICKUP_FLAG, newJob);
                }
            }
        }

        newJob = InitialiseNewJob(pBot, JOB_REPORT);
        if(newJob != NULL) {
            switch(pBot->current_team) {
            case 0:
                _snprintf(newJob->message, MAX_CHAT_LENGTH, "Flag dropped %s", areas[bot_area].namea);
                break;
            case 1:
                _snprintf(newJob->message, MAX_CHAT_LENGTH, "Flag dropped %s", areas[bot_area].nameb);
                break;
            case 2:
                _snprintf(newJob->message, MAX_CHAT_LENGTH, "Flag dropped %s", areas[bot_area].namec);
                break;
            case 3:
                _snprintf(newJob->message, MAX_CHAT_LENGTH, "Flag dropped %s", areas[bot_area].named);
                break;
            default:
                break;
            }

            newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
            SubmitNewJob(pBot, JOB_REPORT, newJob);
        }
    }
}

// This function should be called if the bots current enemy has a flag.
static void BotEnemyCarrierAlert(bot_t* pBot)
{
    if(!pBot->enemy.seenWithFlag)
        return;

    // don't say anything if the bot can see allies
    // or is quite healthy
    // or the bot has said enough already
    if(!defensive_chatter || pBot->visAllyCount > 1 || pBot->f_roleSayDelay > pBot->f_think_time ||
        (pBot->pEdict->v.health / pBot->pEdict->v.max_health) > 0.45)
        return;

    job_struct* newJob = InitialiseNewJob(pBot, JOB_REPORT);
    if(newJob == NULL)
        return;

    // if there's only one enemy find out if the bot is outgunned
    if(pBot->visEnemyCount < 2) {
        if(pBot->bot_class == TFC_CLASS_SCOUT || pBot->bot_class == TFC_CLASS_SNIPER ||
            pBot->bot_class == TFC_CLASS_DEMOMAN || pBot->bot_class == TFC_CLASS_MEDIC ||
            pBot->bot_class == TFC_CLASS_SPY || pBot->bot_class == TFC_CLASS_ENGINEER) {
            if(pBot->enemy.ptr->v.playerclass != TFC_CLASS_SOLDIER ||
                pBot->enemy.ptr->v.playerclass != TFC_CLASS_HWGUY || pBot->enemy.ptr->v.playerclass != TFC_CLASS_PYRO ||
                pBot->enemy.ptr->v.playerclass != TFC_CLASS_MEDIC)
                return;
        }
    }

    const int found_area = AreaInsideClosest(pBot->enemy.ptr);
    if(found_area != -1) {
        switch(pBot->current_team) {
        case 0:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy carrier %s", areas[found_area].namea);
            break;
        case 1:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy carrier %s", areas[found_area].nameb);
            break;
        case 2:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy carrier %s", areas[found_area].namec);
            break;
        case 3:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy carrier %s", areas[found_area].named);
            break;
        default:
            break;
        }

        newJob->message[MAX_CHAT_LENGTH - 1] = '\0'; // not all versions of _snprintf terminate the string
        SubmitNewJob(pBot, JOB_REPORT, newJob);
        pBot->f_roleSayDelay = pBot->f_think_time + 30.0f;

        // look for other defender bots who haven't seen an enemy for
        // a while and maybe send them to help
        /*	if(pBot->current_wp != -1)
                {
                        for(int i = 0; i < 32; i++)
                        {
                                if(bots[i].is_used
                                        && bots[i].mission == ROLE_DEFENDER
                                        && bots[i].bot_has_flag == FALSE
                                        && bots[i].current_team == pBot->current_team
                                        && (bots[i].enemy.f_lastSeen + 20.0f) < pBot->f_think_time)
                                {
                                        bots[i].goto_wp = pBot->current_wp;
                                }
                        }
                }*/
    }
}

// BotThink()
// Called from StartFrame().
void BotThink(bot_t* pBot)
{
    // this sanity check might not be necessary, but wont hurt anything
    if(pBot == NULL)
        return;

    //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp, "1\n"); fclose(fp); }
    // int index = 0;
    // float pitch_degrees, yaw_degrees;
    TraceResult tr;
    bool is_idle;

    pBot->pEdict->v.flags |= FL_FAKECLIENT;

    // TheFatal - START from Advanced Bot Framework (Thanks Rich!)

    // adjust the millisecond delay based on the frame rate interval...
    if(pBot->msecdel <= gpGlobals->time) {
        pBot->msecdel = gpGlobals->time + 0.5; // default 0.5
        if(pBot->msecnum > 0)
            pBot->msecval = 450.0 / pBot->msecnum;
        pBot->msecnum = 0;
    } else
        pBot->msecnum++;

    if(pBot->msecval < 1) // don't allow msec to be less than 1...
        pBot->msecval = 1;

    if(pBot->msecval > 100) // ...or greater than 100
        pBot->msecval = 100;

    // TheFatal - END

    // this is the only place this should be set
    // (gpGlobals->time appears to run in another thread)
    pBot->f_think_time = gpGlobals->time;

    pBot->pEdict->v.button = 0;
    pBot->f_vertical_speed = 0.0; // not swimming up or down initially

    // if the bot hasn't selected stuff to start the game yet, go do that...
    if(pBot->not_started == TRUE) {
        BotResetJobBuffer(pBot);
        pBot->f_duck_time = pBot->f_think_time;
        pBot->strafe_mod = STRAFE_MOD_NORMAL;
        pBot->sentry_edict = NULL;
        pBot->SGRotated = FALSE;
        BotStartGame(pBot);
        g_engfuncs.pfnRunPlayerMove(
            pBot->pEdict, pBot->pEdict->v.v_angle, 0.0, 0, 0, pBot->pEdict->v.button, (byte)0, (byte)pBot->msecval);
        return;
    }

    // keep an up-to-date record of which team the bot is on
    pBot->current_team = UTIL_GetTeam(pBot->pEdict);
    if(pBot->current_team < 0) // shouldn't happen, but, just in case
    {
        pBot->not_started = TRUE; // try joining again
        return;
    }

    //{ FILE *fp=UTIL_OpenFoxbotLog(); fprintf(fp,"name %s, flags %x, health
    //%f\n",STRING(pBot->pEdict->v.netname),pEdict->v.deadflag,pEdict->v.health); fclose(fp); }
    //{ FILE *fp=UTIL_OpenFoxbotLog(); fprintf(fp,"%d\n",pBot->pEdict->v.flags); fclose(fp); }

    // if the bot is dead, randomly press fire to respawn...
    if(pBot->pEdict->v.health < 1 ||
        (pBot->pEdict->v.deadflag != DEAD_NO && pBot->pEdict->v.deadflag != 5) // not a spy feigning death
        || pBot->pEdict->v.playerclass == 0) {
        if(pBot->bot_start2 > 0 && pBot->bot_start3 == 0)
            pBot->bot_start3 = pBot->f_think_time;

        if(pBot->bot_start3 < (pBot->f_think_time - 6) && pBot->bot_start3 != 0) {
            /*char msg[255];
            sprintf(msg," problem wif join, changing team..class t= %d c= %d",
                    pBot->bot_team,pBot->bot_class);
            UTIL_HostSay(pBot->pEdict, 0, msg);*/
            if(pBot->bot_start2 > 1) {
                pBot->bot_team = 5;
                pBot->bot_class = -1;
            }
            pBot->not_started = TRUE;
            pBot->bot_start2++;
            pBot->bot_start3 = 0;
            if(mod_id == TFC_DLL)
                pBot->start_action = MSG_TFC_IDLE;
            /*else if(mod_id == CSTRIKE_DLL)
                    pBot->start_action = MSG_CS_IDLE;
            else if(mod_id == GEARBOX_DLL
                    && pent_info_ctfdetect != NULL)
                    pBot->start_action = MSG_OPFOR_IDLE;
            else if(mod_id == FRONTLINE_DLL)
                    pBot->start_action = MSG_FLF_IDLE;
            else
                    pBot->start_action = 0;  // not needed for non-team MODs
            pBot->create_time = pBot->f_think_time;*/
        }

        if(pBot->need_to_initialize) {
            // ok, flag dropped code here?
            // dropped flag code (is dead, not capped)
            // also, did the flag drop to the ground?!
            // goal_activation(choices) : "Goal Activation bitfields" : 1 =
            // 8 : "8 - return upon death-drop"
            // TFGI_RETURN_DROP
            // slight problem, what if we kicked the bot (and it had the flag)
            // gets an overflow cus of sending the message
            BotReportMyFlagDrop(pBot);

            BotPickNewClass(pBot);
            BotSpawnInit(pBot);
            pBot->need_to_initialize = FALSE;
        }

        pBot->f_duck_time = pBot->f_think_time;
        pBot->strafe_mod = STRAFE_MOD_NORMAL;
        // bool crsh=false;
        /*	UTIL_BotLogPrintf("pEdict %x\nx %f y %f z %f\nspeed %f\nbutton %d\nmsec %f\n name %s\n",
                        pBot->pEdict,pBot->pEdict->v.v_angle.x,pBot->pEdict->v.v_angle.y,
                        pBot->pEdict->v.v_angle.z,pBot->f_move_speed,pBot->pEdict->v.button,
                        pBot->msecval,pBot->name);*/

        // wait at least 1 second before trying to respawn..
        pBot->f_move_speed = 0;
        if(random_long(1, 100) > 50 && (pBot->f_killed_time + 1) < pBot->f_think_time) {
            /*	UTIL_BotLogPrintf(fp,"(1)pEdict %x\nx %f y %f z %f\nspeed %f\nbutton %d\nmsec %f\n name %s\n",
                            pBot->pEdict,pBot->pEdict->v.v_angle.x,pBot->pEdict->v.v_angle.y,
                            pBot->pEdict->v.v_angle.z,pBot->f_move_speed,pBot->pEdict->v.button,
                            pBot->msecval,pBot->name);*/

            pBot->pEdict->v.button = IN_ATTACK;
            pBot->f_killed_time = pBot->f_think_time;
            // crsh=true;
        }
        g_engfuncs.pfnRunPlayerMove(pBot->pEdict, pBot->pEdict->v.v_angle, pBot->f_move_speed, 0, 0,
            pBot->pEdict->v.button, (byte)0, (byte)pBot->msecval);

        // if(crsh)
        //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"returned ok(1)..\n"); fclose(fp); }
        //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"returned ok..\n"); fclose(fp); }
        return;
    }

    pBot->bot_start2 = 0;
    pBot->bot_start3 = 0;

    // set this for the next time the bot dies so it will initialize stuff
    if(pBot->need_to_initialize == FALSE) {
        pBot->need_to_initialize = TRUE;
        pBot->f_bot_spawn_time = pBot->f_think_time;
    }

    // blinded might occur when either team wins all flags on Flagrun
    // on TFC(screen goes black)
    if(pBot->f_blinded_time > (pBot->f_think_time + 60))
        pBot->f_blinded_time = pBot->f_think_time - 1;

    is_idle = FALSE;

    // don't do anything while blinded
    if(pBot->f_blinded_time > pBot->f_think_time)
        is_idle = TRUE;
    if(is_idle) {
        if(pBot->idle_angle_time <= pBot->f_think_time) {
            pBot->idle_angle_time = pBot->f_think_time + RANDOM_FLOAT(0.5, 2.0);

            pBot->pEdict->v.ideal_yaw = pBot->idle_angle + RANDOM_FLOAT(0.0, 40.0) - 20.0;

            BotFixIdealYaw(pBot->pEdict);
        }

        // turn towards ideal_yaw by yaw_speed degrees
        // (slower than normal)
        BotChangeYaw(pBot->pEdict, pBot->pEdict->v.yaw_speed / 2);
        // UTIL_HostSay(pBot->pEdict,0,"a");

        g_engfuncs.pfnRunPlayerMove(pBot->pEdict, pBot->pEdict->v.v_angle, pBot->f_move_speed, 0, 0,
            pBot->pEdict->v.button, (byte)0, (byte)pBot->msecval);
        return;
    }

    if(pBot->name[0] == 0) // name filled in yet?
        strcpy(pBot->name, STRING(pBot->pEdict->v.netname));

    BotRoleCheck(pBot);
    BotComms(pBot);

    // these four functions essentially handle the core of bot behaviour
    BotSenseEnvironment(pBot);
    BotFight(pBot);
    BotJobThink(pBot);
    BotRunJobs(pBot);

    if(spectate_debug)
        BotSpectatorDebug(pBot);

    // if the feign timer is active play dead
    if(pBot->f_feigningTime >= pBot->f_think_time) {
        if(pBot->pEdict->v.deadflag != 5)
            FakeClientCommand(pBot->pEdict, "sfeign", NULL, NULL);
    } else // stop playing dead
    {
        if(pBot->pEdict->v.deadflag == 5)
            FakeClientCommand(pBot->pEdict, "feign", NULL, NULL);
    }

    // don't move while pausing
    if(pBot->f_pause_time >= pBot->f_think_time) {
        pBot->f_move_speed = 0;
        pBot->f_side_speed = 0;
        pBot->f_vertical_speed = 0;
    }

    // crouch if the bots current waypoint is a crouch waypoint...
    if(pBot->current_wp != -1 && waypoints[pBot->current_wp].flags & W_FL_CROUCH &&
        VectorsNearerThan(pBot->pEdict->v.origin, waypoints[pBot->current_wp].origin, 100.0f)) {
        pBot->f_duck_time = pBot->f_think_time + 2.0;
    }

    // put it here so we can do duck jumps :D
    if(pBot->f_duck_time >= pBot->f_think_time)
        pBot->pEdict->v.button |= IN_DUCK;

    // v7 smooth turning, beta
    int speed = 12;
    float diff = -pBot->pEdict->v.v_angle.x;
    // up down
    BotFixIdealPitch(pBot->pEdict);
    if(diff < -180)
        diff = diff + 360;
    if(diff > 180)
        diff = diff - 360;
    diff = diff - pBot->pEdict->v.idealpitch;
    if(diff < -180)
        diff = diff + 360;
    if(diff > 180)
        diff = diff - 360;
    if(diff > -10 && diff < 10)
        speed = 2;
    else if(diff > -20 && diff < 20)
        speed = 3;
    else if(diff > -40 && diff < 40)
        speed = 4;
    else if(diff > -60 && diff < 60)
        speed = 5;
    else if(diff > -90 && diff < 90)
        speed = 7;
    else if(diff > -110 && diff < 110)
        speed = 10;
    if(!pBot->enemy.ptr)
        speed = static_cast<int>(speed * 1.5);
    speed = speed * ((5 - pBot->bot_skill) + (pBot->bot_skill / 5));
    if(speed != 0)
        BotChangePitch(pBot->pEdict, speed);
    BotFixIdealYaw(pBot->pEdict);
    speed = 12;
    diff = pBot->pEdict->v.v_angle.y;
    if(diff < -180)
        diff = diff + 360;
    if(diff > 180)
        diff = diff - 360;
    diff = diff - pBot->pEdict->v.ideal_yaw;
    if(diff < -180)
        diff = diff + 360;
    if(diff > 180)
        diff = diff - 360;
    if(diff > -10 && diff < 10)
        speed = 3;
    else if(diff > -20 && diff < 20)
        speed = 4;
    else if(diff > -40 && diff < 40)
        speed = 4;
    else if(diff > -60 && diff < 60)
        speed = 5;
    else if(diff > -90 && diff < 90)
        speed = 7;
    else if(diff > -110 && diff < 110)
        speed = 10;
    if(pBot->enemy.ptr == NULL)
        speed = static_cast<int>(speed * 2.5);
    speed *= ((5 - pBot->bot_skill) + (pBot->bot_skill / 5));
    if(speed != 0)
        BotChangeYaw(pBot->pEdict, speed);

    // make the body face the same way the bot is looking
    pBot->pEdict->v.angles.y = pBot->pEdict->v.v_angle.y;

    // save the previous speed (for checking if stuck)
    pBot->prev_speed = pBot->f_move_speed;

    // so crates can be pushed out of the way
    if(pBot->f_move_speed >= (pBot->f_max_speed - 0.1))
        pBot->pEdict->v.button |= IN_FORWARD;

    /////////////////////////////////////////////////
    // THIS FUNCTION ACTUALLY MOVES THE BOT INGAME //
    /////////////////////////////////////////////////
    g_engfuncs.pfnRunPlayerMove(pBot->pEdict, pBot->pEdict->v.v_angle, pBot->f_move_speed, pBot->f_side_speed,
        pBot->f_vertical_speed, pBot->pEdict->v.button, (byte)0, (byte)pBot->msecval);
    //////////////////////////////////////////////////

    // check if bots aim should still be affected by concussion and fire etc.
    if(pBot->f_disturbedViewTime <= pBot->f_think_time)
        pBot->disturbedViewAmount = 0;

    pBot->lastFrameHealth = PlayerHealthPercent(pBot->pEdict);

    // reset the periodic timer alerts, if they have activated (this must be done
    // at the end of botThink() so that the alerts can be used by other code)
    if(pBot->f_periodicAlertFifth < pBot->f_think_time)
        pBot->f_periodicAlertFifth = pBot->f_think_time + 0.2f;
    if(pBot->f_periodicAlert1 < pBot->f_think_time)
        pBot->f_periodicAlert1 = pBot->f_think_time + 1.0f;
    if(pBot->f_periodicAlert3 < pBot->f_think_time)
        pBot->f_periodicAlert3 = pBot->f_think_time + 3.0f;
}

// This function is home to most of the sensory functions
// that tell the bot about it's environment and it's own status.
static void BotSenseEnvironment(bot_t* pBot)
{
    BotGrenadeAvoidance(pBot);

    // if the bots are allowed to shoot(via the .cfg file or console)
    if(b_botdontshoot == 0)
        BotEnemyCheck(pBot);
    else
        pBot->enemy.ptr = NULL; // clear enemy pointer(b_botdontshoot is true)

    BotAmmoCheck(pBot); // Stationary bots for TFC minigolf maps [APG]RoboCop[CL]
    if(botdontmove == 0)
        BotEnemyCheck(pBot);
    else
        pBot->f_move_speed = 0;
    pBot->f_side_speed = 0;
    pBot->f_vertical_speed = 0;

    if(pBot->enemy.ptr == NULL)
        BotAttackerCheck(pBot);

    // check if bot should look for items now or not...
    if(pBot->f_find_item_time < pBot->f_think_time)
        BotFindItem(pBot); // see if there are any visible items

    bool didnt_have_flag = FALSE;
    if(pBot->bot_has_flag == FALSE)
        didnt_have_flag = TRUE;

    // update the bots knowledge of whether or not it is carrying a flag
    if(PlayerHasFlag(pBot->pEdict)) {
        pBot->bot_has_flag = TRUE;

        if(didnt_have_flag) {
            //	UTIL_BotLogPrintf("%s(%p)now has flag %d\n",
            //		pBot->name, pBot->pEdict, pBot->flag_impulse);

            if(bot_xmas) {
                pBot->newmsg = TRUE;
                strcpy(pBot->message, "xmas");
            }

            if(random_long(1, 1000) <= 200)
                BotSprayLogo(pBot->pEdict, TRUE);
        }
    } else
        pBot->bot_has_flag = FALSE;
}

// This function handles basic combat actions, such as pointing the active
// waeapon at the bots enemy(if it has one) and pulling the trigger and/or
// throwing grenades.  It does not handle combat movement.
static void BotFight(bot_t* pBot)
{
    // DrEvils Nade update, or toss a nade if threatlevel high enuff.
    if(pBot->lastEnemySentryGun && pBot->enemy.ptr == pBot->lastEnemySentryGun && !FNullEnt(pBot->lastEnemySentryGun))
        BotNadeHandler(pBot, false, GRENADE_STATIONARY);
    else if(pBot->enemy.ptr != NULL && (pBot->enemy.f_firstSeen + 1.0) < pBot->f_think_time &&
        BotAssessThreatLevel(pBot) > 50)
        BotNadeHandler(pBot, true, GRENADE_RANDOM);
    else
        BotNadeHandler(pBot, true, 0);

    // detonate the bots dispenser if enemies are using it
    if(pBot->f_dispenserDetTime > 0.5 // i.e. a valid time
        && pBot->f_dispenserDetTime < pBot->f_think_time && !FNullEnt(pBot->dispenser_edict)) {
        FakeClientCommand(pBot->pEdict, "detdispenser", NULL, NULL);
        pBot->f_dispenserDetTime = 0.0;
        pBot->has_dispenser = FALSE;
        pBot->dispenser_edict = NULL;
    }

    // if the bot has an enemy
    if(pBot->enemy.ptr != NULL) {
        BotShootAtEnemy(pBot);

        BotCombatThink(pBot);

        // if bot sniper has sniper rifle out and an enemy
        /*	if(pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER
                        && pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE
                        && !pBot->bot_has_flag)
                {
                        //if were not in attack, strafe a bit slower
                        if(!(pBot->pEdict->v.button & IN_ATTACK))
                        {
                                if(pBot->side_direction == SIDE_DIRECTION_RIGHT)
                                        pBot->f_side_speed = pBot->f_max_speed / 4;
                                else pBot->f_side_speed = -(pBot->f_max_speed / 4);
                                //go half speed forwards..assuming we've already set the forward speed
                                //nope... stop moving forward... we don't if we're sniping
                                pBot->f_move_speed = 0;
                        }
                        if(pBot->f_pause_time > pBot->f_think_time // is the sniper "paused"?
                           && pBot->f_snipe_time > pBot->f_think_time)
                        {
                                // you could make the bot look left then right, or look up
                                // and down, to make it appear that the bot is hunting for something
                                pBot->f_side_speed = 0;
                                pBot->f_move_speed = 0;

                                //just in case...dam timer vars
                                if(pBot->f_pause_time > (pBot->f_think_time) + 100)
                                        pBot->f_pause_time = 0;
                        }
                }*/
    }
}

// This function is a one-stop-shop for bot behaviour when they are fighting
// an enemy.
static void BotCombatThink(bot_t* pBot)
{
    const int enemy_team = UTIL_GetTeam(pBot->enemy.ptr);

    // get bots to crouch if they are on top of their target
    // so that their close combat weapons can reach
    if(pBot->enemy.f_seenDistance < 100.0f && pBot->pEdict->v.origin.z > pBot->enemy.ptr->v.origin.z)
        pBot->f_duck_time = pBot->f_think_time + 0.3;

    // ignore allies
    if(pBot->current_team == enemy_team || team_allies[pBot->current_team] & (1 << enemy_team))
        return;

    const int ThreatLevel = guessThreatLevel(pBot);

    // random ducking/jumping evasive behaviour
    if(pBot->bot_skill < 3 && pBot->disguise_state != DISGUISE_COMPLETE && pBot->f_dontEvadeTime < pBot->f_think_time) {
        // make snipers duck randomly when sniping
        if(pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER && pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE &&
            pBot->f_periodicAlertFifth < pBot->f_think_time) {
            if(random_long(1, 1000) <= 300)
                pBot->f_duck_time = pBot->f_think_time + 0.8f;
        } else // not a sniping sniper
        {
            // jump a lot if the enemy is a nearby soldier or pyro
            // (extra annoying for the enemy!)
            if(pBot->f_periodicAlert1 < pBot->f_think_time && (pBot->enemy.ptr->v.playerclass == TFC_CLASS_SOLDIER ||
                                                                  pBot->enemy.ptr->v.playerclass == TFC_CLASS_PYRO) &&
                pBot->bot_skill < 2 && pBot->enemy.f_seenDistance < 800.0)
                pBot->pEdict->v.button |= IN_JUMP;

            // duck/jump randomly in battle
            if(pBot->f_periodicAlertFifth < pBot->f_think_time && random_long(0, 120) < pBot->trait.aggression) {
                if(random_long(1, 1000) < 501)
                    pBot->pEdict->v.button |= IN_JUMP;
                else if(pBot->f_duck_time < pBot->f_think_time)
                    pBot->f_duck_time = pBot->f_think_time + 0.25f;

                /*	if(random_long(1, 1000) <= 120)
                                pBot->pEdict->v.button |= IN_JUMP;
                        if(random_long(1, 1000) <= 100)
                                pBot->f_duck_time = pBot->f_think_time + 0.6f;*/
            }
        }
    }

    /*	// strafing??
            //can we get the bot to strafe whilst fighting?
            if(pBot->f_periodicAlert1
                    && random_long(0, 1000) < 200)
            {
                    if(pBot->side_direction == SIDE_DIRECTION_RIGHT)
                            pBot->side_direction = SIDE_DIRECTION_LEFT;
                    else pBot->side_direction = SIDE_DIRECTION_RIGHT;
            }

            // once a second, make sure the bot isn't strafing into a wall
            if(pBot->f_periodicAlert1 < pBot->f_think_time)
            {
                    if(BotCheckWallOnRight(pBot))
                            pBot->side_direction = SIDE_DIRECTION_LEFT;
                    else if(BotCheckWallOnLeft(pBot))
                            pBot->side_direction = SIDE_DIRECTION_RIGHT;
            }

            if(pBot->strafe_mod == STRAFE_MOD_NORMAL)
            {
                    if(pBot->side_direction == SIDE_DIRECTION_RIGHT)
                            pBot->f_side_speed = pBot->f_max_speed;
                    else pBot->f_side_speed = -(pBot->f_max_speed);
            }
            else if(pBot->strafe_mod == STRAFE_MOD_HEAL)
            {
                    if(pBot->side_direction == SIDE_DIRECTION_RIGHT)
                            pBot->f_side_speed = pBot->f_max_speed / 4;
                    else pBot->f_side_speed = -(pBot->f_max_speed / 4);
            }*/

    // pursue the enemy?
    if((ThreatLevel <= 25 || pBot->enemy.seenWithFlag) && !pBot->bot_has_flag &&
        pBot->f_periodicAlert1 < pBot->f_think_time) {
        bool tooBusySniping = FALSE; // to stop snipers running after everybody!
        if(pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER &&
            (pBot->trait.aggression < 50 || waypoints[pBot->current_wp].flags & W_FL_SNIPER))
            tooBusySniping = TRUE;

        if(tooBusySniping == FALSE) {
            job_struct* newJob = InitialiseNewJob(pBot, JOB_PURSUE_ENEMY);
            if(newJob != NULL) {
                newJob->player = pBot->enemy.ptr;
                newJob->origin = pBot->enemy.ptr->v.origin; // remember where
                SubmitNewJob(pBot, JOB_PURSUE_ENEMY, newJob);
            }
        }
    }

    // make the hunted seek backup when alone(the snipers love a lone fawn in the woods!)
    if(pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN && pBot->bot_has_flag && pBot->visAllyCount <= 1 &&
        (pBot->f_lastAllySeenTime + 15.0) > pBot->f_think_time && pBot->f_lastAllySeenTime > pBot->f_killed_time) {
        job_struct* newJob = InitialiseNewJob(pBot, JOB_SEEK_BACKUP);
        if(newJob != NULL)
            SubmitNewJob(pBot, JOB_SEEK_BACKUP, newJob);
    }

    // should the bot retreat from it's enemy?
    if(ThreatLevel >= 66 && (pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN ||
                                !pBot->bot_has_flag)) // the civilian has an invisible flag in hunted
    {
        // seek an ally if the bot is alone and saw an ally recently
        job_struct* newJob = InitialiseNewJob(pBot, JOB_SEEK_BACKUP);
        if(newJob != NULL && pBot->visAllyCount < 2 && (pBot->f_lastAllySeenTime + 15.0) > pBot->f_think_time) {
            SubmitNewJob(pBot, JOB_SEEK_BACKUP, newJob);
        } else {
            job_struct* newJob = InitialiseNewJob(pBot, JOB_AVOID_ENEMY);
            if(newJob != NULL) {
                newJob->player = pBot->enemy.ptr;
                SubmitNewJob(pBot, JOB_AVOID_ENEMY, newJob);
            }
        }
    }

    // make Demomen reload if they have an empty clip
    if(pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN &&
        (pBot->current_weapon.iId == TF_WEAPON_GL || pBot->current_weapon.iId == TF_WEAPON_PL) &&
        pBot->current_weapon.iClip == 0) {
        pBot->pEdict->v.button |= IN_RELOAD; // press reload button
        pBot->f_shoot_time = pBot->f_think_time + 1.0f;
    }

    if(pBot->f_periodicAlert1 < pBot->f_think_time)
        BotEnemyCarrierAlert(pBot);
}

// Assess the threat level of the target using their class & my class.
// 0-100 scale higher is more dangerous
// Used in the assessment of when to evade enemies.
static int guessThreatLevel(bot_t* pBot)
{
    // show no fear if infected
    if(PlayerIsInfected(pBot->pEdict))
        return 0;

    // This will keep track of the threat level.
    int Threat = 0;

    // Set base threat from MY class
    switch(pBot->pEdict->v.playerclass) {
    case TFC_CLASS_CIVILIAN:
        Threat += 75;
        break;
    case TFC_CLASS_SCOUT:
        Threat += 45;
        break;
    case TFC_CLASS_SNIPER:
        Threat += 25;
        break;
    case TFC_CLASS_SOLDIER:
        Threat += 20;
        break;
    case TFC_CLASS_DEMOMAN:
        Threat += 25;
        break;
    case TFC_CLASS_MEDIC:
        Threat += 25;
        break;
    case TFC_CLASS_HWGUY:
        Threat += 15;
        break;
    case TFC_CLASS_PYRO:
        Threat += 20;
        break;
    case TFC_CLASS_SPY:
        Threat += 35; // less gunplay, more sneaking about
        break;
    case TFC_CLASS_ENGINEER:
        Threat += 25;
        break;
    default:
        break;
    }

    // Add based on targets class.
    switch(pBot->enemy.ptr->v.playerclass) {
    case TFC_CLASS_CIVILIAN:
        Threat -= 50; // curse you, anomolous man in a blue suit!
        break;
    case TFC_CLASS_SCOUT:
        Threat += 5;
        break;
    case TFC_CLASS_SNIPER:
        Threat += 11;
        if(pBot->enemy.f_seenDistance > 800.0f && pBot->enemy.f_seenDistance < 1200.0f)
            Threat += 10;
        break;
    case TFC_CLASS_SOLDIER:
        Threat += 15;
        if(pBot->enemy.f_seenDistance < 600.0f)
            Threat += 20;
        break;
    case TFC_CLASS_DEMOMAN:
        Threat += 13;
        if(pBot->enemy.f_seenDistance < 600.0f)
            Threat += 25;
        break;
    case TFC_CLASS_MEDIC:
        Threat += 15;
        if(pBot->pEdict->v.playerclass != TFC_CLASS_MEDIC && pBot->enemy.f_seenDistance < 400.0f)
            Threat += 30;
        break;
    case TFC_CLASS_HWGUY:
        Threat += 20;
        if(pBot->enemy.f_seenDistance < 600.0f)
            Threat += 30;
        break;
    case TFC_CLASS_PYRO:
        Threat += 15;
        if(pBot->enemy.f_seenDistance < 200.0f)
            Threat += 10;
        break;
    case TFC_CLASS_SPY:
        Threat += 5; // Spies should be attacked on sight
        break;
    case TFC_CLASS_ENGINEER:
        Threat += 15;
        if(pBot->enemy.f_seenDistance < 300.0f)
            Threat += 10;
        break;
    default:
        break;
    }

    // concussed, burning etc?
    if(pBot->disturbedViewAmount > 20)
        Threat += 15;

    // modulate based upon the bots aggression trait
    if(pBot->trait.aggression <= 33) {
        Threat += 15;

        if(PlayerHealthPercent(pBot->pEdict) < 35)
            Threat += 10;
    } else if(pBot->trait.aggression > 66)
        Threat -= 15;

    // check if the enemy is not looking at the bot
    const Vector vecEnd = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
    if(!FInViewCone(vecEnd, pBot->enemy.ptr))
        Threat -= 20;

    // defenders should be braver
    if(pBot->mission == ROLE_DEFENDER && pBot->pEdict->v.playerclass != TFC_CLASS_SNIPER)
        Threat -= 20;

    // flag carriers should be braver
    if(pBot->bot_has_flag && pBot->pEdict->v.playerclass != TFC_CLASS_CIVILIAN)
        Threat -= 20;

    // ramp up the threat level if the bot is outnumbered
    // or reduce the threat level if the enemy appears outnumbered
    if(pBot->visEnemyCount > pBot->visAllyCount)
        Threat += (pBot->visEnemyCount - pBot->visAllyCount) * 20;
    else
        Threat -= (pBot->visAllyCount - pBot->visEnemyCount) * 20;

    // if the bots team has been scripted to attack more
    // reduce the bots level of caution
    if(RoleStatus[pBot->current_team] > 70)
        Threat -= 15;

    // if the bot is a certain class and their clip is empty
    // pursuade them to retreat so they can reload
    if(pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN &&
        (pBot->current_weapon.iId == TF_WEAPON_GL || pBot->current_weapon.iId == TF_WEAPON_PL) &&
        pBot->current_weapon.iClip == 0)
        Threat += 20;

    // encourage the bot to retreat from an enemy if the enemy is close
    // and the bot is about to throw a grenade at them
    if(pBot->nadePrimed && pBot->pEdict->v.playerclass != TFC_CLASS_SCOUT && pBot->enemy.f_seenDistance < 400.0f)
        Threat += 50;

    return Threat;
}

// This function is useful for when a bot is waiting somewhere and you want it
// to look about as if curious.
// This function will also try to stop the bot from staring at a nearby wall.
// NOTE: The bot should be stationary when running this function.
void BotLookAbout(bot_t* pBot)
{
    // give the bot time to return to it's waypoint afterwards
    pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

    // use a timer to minimise CPU usage
    if(pBot->f_view_change_time <= pBot->f_think_time) {
        // if the bot saw an enemy recently behave differently
        if((pBot->enemy.f_lastSeen + random_float(4.0, 6.0)) >= pBot->f_think_time) {
            // set the timer again to a shorter, more alert level
            pBot->f_view_change_time = pBot->f_think_time + random_float(0.7, 1.2);

            // most of the time look at the last enemies last position
            if(random_long(1, 1000) < 667) {
                BotSetFacing(pBot, pBot->enemy.lastLocation);
                return;
            }
        }

        // just check out the environment, see what's up

        Vector newAngle = pBot->pEdict->v.v_angle;
        newAngle.x = 0.0; // level this out in case the bot is staring at the ground
        newAngle.y = random_float(-180.0, 180.0);

        UTIL_MakeVectors(newAngle);

        // do a trace 200 units ahead of the new view angle to check for sight barriers
        const Vector v_forwards = pBot->pEdict->v.origin + (gpGlobals->v_forward * 200);

        TraceResult tr;
        UTIL_TraceLine(
            pBot->pEdict->v.origin, v_forwards, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

        // if the new view angle is relatively unblocked make the bot turn to match it
        if(tr.flFraction >= 1.0) {
            pBot->pEdict->v.ideal_yaw = newAngle.y;
            BotFixIdealYaw(pBot->pEdict);
            pBot->pEdict->v.idealpitch = random_float(-15.0, 15.0);

            // set the timer again
            pBot->f_view_change_time = pBot->f_think_time + RANDOM_FLOAT(1.0f, 3.0f);
        }
    }
}

// This function gets the bot to trigger debug messages only if there is a
// human player spectating very near to it.
static void BotSpectatorDebug(bot_t* pBot)
{
    // don't do anything on a dedicated server
    if(IS_DEDICATED_SERVER())
        return;

    // this should be the host player on a Listen server(probably a developer!)
    edict_t* pPlayer = INDEXENT(1);

    // only give debug messages if a spectating human player is very close
    if(pPlayer != NULL && !pPlayer->free && !(pPlayer->v.flags & FL_FAKECLIENT)
        //	&& pPlayer->v.movetype == MOVETYPE_NOCLIP  // didn't work
        && !IsAlive(pPlayer) && VectorsNearerThan(pBot->pEdict->v.origin, pPlayer->v.origin, 80.0)) {
        char msg[255];

        // show the bots name, skill, and role
        if(pBot->mission == ROLE_DEFENDER)
            _snprintf(msg, 72, "%s, skill %d, Defending\n", pBot->name, pBot->bot_skill);
        else
            _snprintf(msg, 72, "%s, skill %d, Attacking\n", pBot->name, pBot->bot_skill);

        msg[71] = '\0';

        // if spectate_debug is set to 1 show the bots job list
        if(spectate_debug == 1) {
            /*	if(pBot->currentJob > -1)
                    {
                            char msgBuffer[128] = "";
                            _snprintf(msgBuffer, 128, "CURRENT JOB - phase %d runtime %f\n",
                                    pBot->job[pBot->currentJob].phase, pBot->f_think_time -
               pBot->job[pBot->currentJob].f_bufferedTime);
                            strncat(msg, msgBuffer, 255 - strlen(msg));
                    }*/

            // list the jobs in the buffer
            for(int i = 0; i < JOB_BUFFER_MAX; i++) {
                // list one job per line
                if(pBot->jobType[i] > JOB_NONE && pBot->jobType[i] < JOB_TYPE_TOTAL) {
                    strncat(msg, jl[pBot->jobType[i]].jobNames, 255 - strlen(msg));

                    // indicate the current job and how long it's been running
                    if(pBot->currentJob == i) {
                        char msgBuffer[128] = "";
                        _snprintf(msgBuffer, 128, " [phase %d, buffered %f]\n", pBot->job[pBot->currentJob].phase,
                            pBot->f_think_time - pBot->job[pBot->currentJob].f_bufferedTime);
                        strncat(msg, msgBuffer, 255 - strlen(msg));
                    } else
                        strncat(msg, "\n", 255 - strlen(msg)); // add a newline on the end

                    /*	if(pBot->currentJob == i)  // indicate the current job
                                    strncat(msg, "     [CURRENT JOB]\n", 255 - strlen(msg));
                            else strncat(msg, "\n", 255 - strlen(msg));  // add a newline on the end*/
                } else
                    strncat(msg, "\n", 255 - strlen(msg)); // skip empty job indexes
            }
        } else if(spectate_debug == 2) // list what jobs are blacklisted
        {
            // list any jobs in the blacklist
            strncat(msg, "Blacklist:\n", 255 - strlen(msg));
            for(int i = 0; i < JOB_BLACKLIST_MAX; i++) {
                // list one blacklisted job per line
                if(pBot->jobBlacklist[i].type > JOB_NONE && pBot->jobBlacklist[i].f_timeOut >= pBot->f_think_time &&
                    pBot->jobBlacklist[i].type < JOB_TYPE_TOTAL) {
                    strncat(msg, jl[pBot->jobBlacklist[i].type].jobNames, 255 - strlen(msg));
                    strncat(msg, "\n", 255 - strlen(msg)); // add a newline on the end
                } else
                    strncat(msg, "\n", 255 - strlen(msg)); // skip empty blacklist indexes
            }
        } else if(spectate_debug == 3) // show the bots personality traits
        {
            char msgBuffer[128] = "";
            _snprintf(msgBuffer, 128, "aggression %d\nhealth threshold%d\ncamper %d", pBot->trait.aggression,
                pBot->trait.health, pBot->trait.camper);
            strncat(msg, msgBuffer, 255 - strlen(msg));
        } else // some other spectate_debug mode - show navigation info
        {
            char msgBuffer[128] = "";
            _snprintf(msgBuffer, 128, "waypoint deadline %f\nnavDistance %f\nf_navProblemStartTime %f\n",
                (pBot->f_current_wp_deadline - pBot->f_think_time),
                (waypoints[pBot->current_wp].origin - pBot->pEdict->v.origin).Length(),
                pBot->f_think_time - pBot->f_navProblemStartTime);
            strncat(msg, msgBuffer, 255 - strlen(msg));

            _snprintf(msgBuffer, 128, "velocity length 2D %f\n velocity Z %f\n", pBot->pEdict->v.velocity.Length2D(),
                pBot->pEdict->v.velocity.z);
            strncat(msg, msgBuffer, 255 - strlen(msg));
        }

        msg[254] = '\0';

        // output the message
        MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pPlayer);
        WRITE_BYTE(TE_TEXTMESSAGE);
        WRITE_BYTE(2 & 0xFF);

        // top of the screen
        WRITE_SHORT(FixedSigned16(1, -1 << 13)); // coordinates X
        WRITE_SHORT(FixedSigned16(1, 0 << 13));  // coordinates Y

        /*			// left side of the screen
                                WRITE_SHORT( 0 ); // coordinates X
                                WRITE_SHORT( -8192 ); // coordinates Y*/

        WRITE_BYTE(1); // effect

        WRITE_BYTE(255);
        WRITE_BYTE(255);
        WRITE_BYTE(255);
        WRITE_BYTE(255);

        WRITE_BYTE(255);
        WRITE_BYTE(255);
        WRITE_BYTE(255);
        WRITE_BYTE(255);
        WRITE_SHORT(FixedUnsigned16(0, 1 << 8)); // fade-in time
        WRITE_SHORT(FixedUnsigned16(0, 1 << 8)); // fade-out time
        WRITE_SHORT(FixedUnsigned16(1, 1 << 8)); // hold time

        WRITE_STRING(msg);
        MESSAGE_END();

        // show the bots current waypoint, altering the color to show
        // the state of any nav problems
        if(pBot->current_wp > -1 && pBot->current_wp < num_waypoints) {
            if(pBot->f_navProblemStartTime < 0.1) // green - all is well
                WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin, waypoints[pBot->current_wp].origin, 10, 2, 50,
                    250, 50, 200, 1);
            else if((pBot->f_navProblemStartTime + 0.5) >= pBot->f_think_time)
                WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin, // amber - caution
                    waypoints[pBot->current_wp].origin, 10, 2, 250, 128, 0, 200, 1);
            else
                WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin, // red - trouble!
                    waypoints[pBot->current_wp].origin, 10, 2, 250, 50, 50, 200, 1);
        }

        // show what other waypoints the bot is interested in
        if(pBot->f_periodicAlertFifth < pBot->f_think_time) {
            if(pBot->goto_wp > -1 && pBot->goto_wp < num_waypoints)
                WaypointDrawBeam(
                    INDEXENT(1), pBot->pEdict->v.origin, waypoints[pBot->goto_wp].origin, 10, 2, 50, 50, 250, 200, 10);

            // if the bot is taking a side route, show it
            if(pBot->branch_waypoint != -1)
                WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
                    waypoints[pBot->branch_waypoint].origin + Vector(0, 0, 35.0), 10, 2, 250, 250, 50, 200, 10);
        }

        return;
    }
}
