//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_start.cpp
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
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"

extern int mod_id;
extern edict_t* pent_info_ctfdetect;

extern bool bot_team_balance;

extern int max_team_players[4];
extern int playersPerTeam[4];
extern int team_class_limits[4];
extern int max_teams;
extern bool is_team[4];

extern bool spawn_check_crash;
extern int spawn_check_crash_count;
extern edict_t* spawn_check_crash_edict;

static int BotPickFavoredTeam_TFC(int faveClass);

void BotStartGame(bot_t* pBot)
{
    pBot->not_started = TRUE;

    char c_team[32];
    char c_class[32];
    //	char c_item[32];
    int team;
    edict_t* pEdict = pBot->pEdict;

    if(mod_id == TFC_DLL) {
        if(pBot->create_time > (gpGlobals->time) + 1.0 || pBot->create_time + 3.0 <= gpGlobals->time)
            pBot->create_time = (gpGlobals->time) + 1.0;

        if(pBot->create_time > gpGlobals->time && pBot->create_time - 0.5 < gpGlobals->time) {
            if(!spawn_check_crash) {
                spawn_check_crash = TRUE;
                spawn_check_crash_count = 0;
                spawn_check_crash_edict = pEdict;
                char* cvar_specs = (char*)CVAR_GET_STRING("allow_spectators");
                if(strcmp(cvar_specs, "0") == 0) {
                    CVAR_SET_STRING("allow_spectators", "1");
                    FakeClientCommand(pBot->pEdict, "spectate", NULL, NULL);
                    CVAR_SET_STRING("allow_spectators", "0");
                } else
                    FakeClientCommand(pBot->pEdict, "spectate", NULL, NULL);

                spawn_check_crash = FALSE;
                spawn_check_crash_edict = NULL;
            }

            pBot->create_time = gpGlobals->time;
            //	UTIL_BotLogPrintf("\n%p in limbo\n", pBot);
            return;
        }

        // force team selection
        if((pBot->create_time + 1.0 <= gpGlobals->time))
            pBot->start_action = MSG_TFC_TEAM_SELECT;

        if((pBot->create_time + 1.5 <= gpGlobals->time))
            pBot->start_action = MSG_TFC_CLASS_SELECT;

        // if we dont start after 2, sort other stuff
        if((pBot->create_time + 2.0 <= gpGlobals->time))
            pBot->not_started = FALSE;

        // handle Team Fortress Classic stuff here...

        if(pBot->start_action == MSG_TFC_TEAM_SELECT) {
            pBot->start_action = MSG_TFC_IDLE; // switch back to idle

            // reset the bots team if the team it's chosen is full already
            int botsTeam = pBot->bot_team - 1;
            if(botsTeam > -1 && botsTeam < 4 && max_team_players[botsTeam] > 0 &&
                playersPerTeam[botsTeam] >= max_team_players[botsTeam]) {
                pBot->bot_team = -1;
                //	UTIL_BotLogPrintf("%p resetting team to %d\n", pBot, pBot->bot_team);
            }

            // if the bot has not already chosen a team
            if(pBot->bot_team < 1 || pBot->bot_team > 5) {
                // if the bot is not a fair player(a scoundrel) then
                // let the bot choose the team it prefers, otherwise
                // auto-assign the bots team(auto-assign keeps the teams balanced)
                if(bot_team_balance == FALSE && RANDOM_LONG(1, 1000) > pBot->trait.fairplay) {
                    // add one to what BotPickFavoredTeam_TFC() returns, because
                    // it returns team values from 0 to 3
                    pBot->bot_team = BotPickFavoredTeam_TFC(pBot->trait.faveClass) + 1;
                    //	UTIL_BotLogPrintf("%p goin rogue with %d\n", pBot, pBot->bot_team);
                    if(pBot->bot_team < 1)
                        pBot->bot_team = 5;
                } else
                    pBot->bot_team = 5; // auto-select team via the menu
            }

            // select the team the bot wishes to join...
            if(pBot->bot_team == 1)
                strcpy(c_team, "1");
            else if(pBot->bot_team == 2)
                strcpy(c_team, "2");
            else if(pBot->bot_team == 3)
                strcpy(c_team, "3");
            else if(pBot->bot_team == 4)
                strcpy(c_team, "4");
            else
                strcpy(c_team, "5");

            //	UTIL_BotLogPrintf("%p joining team %d\n", pBot, pBot->bot_team);

            FakeClientCommand(pEdict, "jointeam", c_team, NULL);

            return;
        }

        if(pBot->start_action == MSG_TFC_CLASS_SELECT) {
            pBot->start_action = MSG_TFC_IDLE; // switch back to idle
            if((pBot->bot_class < 1) || (pBot->bot_class > 9))
                pBot->bot_class = -1;
            if(pBot->bot_class == -1)
                pBot->bot_class = RANDOM_LONG(1, 9);
            team = UTIL_GetTeam(pEdict);

            if(team_class_limits[team] == -1) // civilian only?
                pBot->bot_class = 0;          // civilian
            else {
                int class_not_allowed;

                if(pBot->bot_class == 10)
                    class_not_allowed = team_class_limits[team] & (1 << 7);
                else if(pBot->bot_class <= 7)
                    class_not_allowed = team_class_limits[team] & (1 << (pBot->bot_class - 1));
                else
                    class_not_allowed = team_class_limits[team] & (1 << (pBot->bot_class));

                while(class_not_allowed) {
                    pBot->bot_class = RANDOM_LONG(1, 9);

                    if(pBot->bot_class == 10)
                        class_not_allowed = team_class_limits[team] & (1 << 7);
                    else if(pBot->bot_class <= 7)
                        class_not_allowed = team_class_limits[team] & (1 << (pBot->bot_class - 1));
                    else
                        class_not_allowed = team_class_limits[team] & (1 << (pBot->bot_class));
                }
            }

            // select the class the bot wishes to use...
            if(pBot->bot_class == 0)
                strcpy(c_class, "civilian");
            else if(pBot->bot_class == 1)
                strcpy(c_class, "scout");
            else if(pBot->bot_class == 2)
                strcpy(c_class, "sniper");
            else if(pBot->bot_class == 3)
                strcpy(c_class, "soldier");
            else if(pBot->bot_class == 4)
                strcpy(c_class, "demoman");
            else if(pBot->bot_class == 5)
                strcpy(c_class, "medic");
            else if(pBot->bot_class == 6)
                strcpy(c_class, "hwguy");
            else if(pBot->bot_class == 7)
                strcpy(c_class, "pyro");
            else if(pBot->bot_class == 8)
                strcpy(c_class, "spy");
            else if(pBot->bot_class == 9)
                strcpy(c_class, "engineer");

            FakeClientCommand(pEdict, c_class, NULL, NULL);

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = FALSE;
            //	UTIL_BotLogPrintf("%p chosen class %d\n", pBot, pBot->bot_class);
            return;
        }
    } /* else if(mod_id == CSTRIKE_DLL) {
        // handle Counter-Strike stuff here...

        if(pBot->start_action == MSG_CS_TEAM_SELECT) {
            pBot->start_action = MSG_CS_IDLE; // switch back to idle

            if((pBot->bot_team != 1) && (pBot->bot_team != 2) && (pBot->bot_team != 5))
                pBot->bot_team = -1;

            if(pBot->bot_team == -1)
                pBot->bot_team = RANDOM_LONG(1, 2);

            // select the team the bot wishes to join...
            if(pBot->bot_team == 1)
                strcpy(c_team, "1");
            else if(pBot->bot_team == 2)
                strcpy(c_team, "2");
            else
                strcpy(c_team, "5");

            FakeClientCommand(pEdict, "menuselect", c_team, NULL);

            return;
        }

        if(pBot->start_action == MSG_CS_CT_SELECT) // counter terrorist
        {
            pBot->start_action = MSG_CS_IDLE; // switch back to idle

            if((pBot->bot_class < 1) || (pBot->bot_class > 4))
                pBot->bot_class = -1; // use random if invalid

            if(pBot->bot_class == -1)
                pBot->bot_class = RANDOM_LONG(1, 4);

            // select the class the bot wishes to use...
            if(pBot->bot_class == 1)
                strcpy(c_class, "1");
            else if(pBot->bot_class == 2)
                strcpy(c_class, "2");
            else if(pBot->bot_class == 3)
                strcpy(c_class, "3");
            else if(pBot->bot_class == 4)
                strcpy(c_class, "4");
            else
                strcpy(c_class, "5"); // random

            FakeClientCommand(pEdict, "menuselect", c_class, NULL);

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = FALSE;

            return;
        }

        if(pBot->start_action == MSG_CS_T_SELECT) // terrorist select
        {
            pBot->start_action = MSG_CS_IDLE; // switch back to idle

            if((pBot->bot_class < 1) || (pBot->bot_class > 4))
                pBot->bot_class = -1; // use random if invalid

            if(pBot->bot_class == -1)
                pBot->bot_class = RANDOM_LONG(1, 4);

            // select the class the bot wishes to use...
            if(pBot->bot_class == 1)
                strcpy(c_class, "1");
            else if(pBot->bot_class == 2)
                strcpy(c_class, "2");
            else if(pBot->bot_class == 3)
                strcpy(c_class, "3");
            else if(pBot->bot_class == 4)
                strcpy(c_class, "4");
            else
                strcpy(c_class, "5"); // random

            FakeClientCommand(pEdict, "menuselect", c_class, NULL);

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = FALSE;

            return;
        }
    } else if((mod_id == GEARBOX_DLL) && (pent_info_ctfdetect != NULL)) {
        // handle Opposing Force CTF stuff here...

        if(pBot->start_action == MSG_OPFOR_TEAM_SELECT) {
            pBot->start_action = MSG_OPFOR_IDLE; // switch back to idle

            if((pBot->bot_team != 1) && (pBot->bot_team != 2) && (pBot->bot_team != 3))
                pBot->bot_team = -1;

            if(pBot->bot_team == -1)
                pBot->bot_team = RANDOM_LONG(1, 2);

            // select the team the bot wishes to join...
            if(pBot->bot_team == 1)
                strcpy(c_team, "1");
            else if(pBot->bot_team == 2)
                strcpy(c_team, "2");
            else
                strcpy(c_team, "3");

            FakeClientCommand(pEdict, "jointeam", c_team, NULL);

            return;
        }

        if(pBot->start_action == MSG_OPFOR_CLASS_SELECT) {
            pBot->start_action = MSG_OPFOR_IDLE; // switch back to idle

            if((pBot->bot_class < 0) || (pBot->bot_class > 10))
                pBot->bot_class = -1;

            if(pBot->bot_class == -1)
                pBot->bot_class = RANDOM_LONG(1, 10);

            // select the class the bot wishes to use...
            if(pBot->bot_class == 1)
                strcpy(c_class, "1");
            else if(pBot->bot_class == 2)
                strcpy(c_class, "2");
            else if(pBot->bot_class == 3)
                strcpy(c_class, "3");
            else if(pBot->bot_class == 4)
                strcpy(c_class, "4");
            else if(pBot->bot_class == 5)
                strcpy(c_class, "5");
            else if(pBot->bot_class == 6)
                strcpy(c_class, "6");
            else
                strcpy(c_class, "7");

            FakeClientCommand(pEdict, "selectchar", c_class, NULL);

            // bot has now joined the game (doesn't need to be started)
            pBot->not_started = FALSE;

            return;

    } else if(mod_id == FRONTLINE_DLL) {
        // not directly supported by Foxbot
    } else {
        // otherwise, don't need to do anything to start game...
        // UTIL_HostSay(INDEXENT(1),0,"wtf!");
        pBot->not_started = FALSE;
    }}*/
}

// This function allows a bot to pick a team according to it's own preferences.
// It's meant for TFC only.
// Note: this function returns preferred team values from 0 to 3.
// Otherwise, a return value of -1 means a failure to pick a preferred team.
static int BotPickFavoredTeam_TFC(int faveClass)
{
    if(mod_id != TFC_DLL)
        return -1;

    short activeTeamList[4] = { -1, -1, -1, -1 };
    short activeTeamTotal = 0;

    // count the number of suitable teams
    for(int i = 0; i < 4; i++) {
        //	UTIL_BotLogPrintf("team:%d, is_team:%d, maxplayers:%d, total players %d\n",
        //		i, is_team[i], max_team_players[i], playersPerTeam[i]);

        // skip inactive/non-valid teams
        if(is_team[i] == FALSE)
            continue;

        // skip teams that are full already
        if(max_team_players[i] > 0 && playersPerTeam[i] >= max_team_players[i])
            continue;

        // Check if the bots favourite class is allowed on this team.
        int class_not_allowed;

        if(faveClass <= 7)
            class_not_allowed = team_class_limits[i] & (1 << (faveClass - 1));
        else
            class_not_allowed = team_class_limits[i] & (1 << (faveClass));

        if(class_not_allowed)
            continue;

        activeTeamList[activeTeamTotal] = i;
        ++activeTeamTotal;
    }

    //	UTIL_BotLogPrintf("Bots fave class:%d, activeTeamTotal:%d, map:%s\n",
    //		faveClass, activeTeamTotal, STRING(gpGlobals->mapname));

    // not found a team the bot likes?
    if(activeTeamTotal == 0)
        return -1;

    // pick a suitable team at random
    return activeTeamList[random_long(0, (activeTeamTotal - 1))];
}
