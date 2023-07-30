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
#include <entity_state.h>

#include <cmath>
#include <osdep.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "waypoint.h"

// meta mod includes
#include <dllapi.h>
#include <meta_api.h>

#include "botcam.h"

#define VER_MAJOR 0
#define VER_MINOR 85
//#define VER_BUILD 0

#define MENU_NONE 0
#define MENU_1 1
#define MENU_2 2
#define MENU_3 3
#define MENU_4 4
#define MENU_5 5
#define MENU_6 6
#define MENU_7 7

cvar_t foxbot = { "foxbot", "0.85-beta1", FCVAR_SERVER | FCVAR_UNLOGGED, 0, nullptr };
cvar_t enable_foxbot = { "enable_foxbot", "1", FCVAR_SERVER | FCVAR_UNLOGGED, 0, nullptr };
cvar_t sv_bot = { "bot", "", 0, 0, nullptr };

extern GETENTITYAPI other_GetEntityAPI;
extern GETNEWDLLFUNCTIONS other_GetNewDLLFunctions;

extern int debug_engine;

extern char g_argv[256];
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
bool defensive_chatter = true;
bool offensive_chatter = true;
bool observer_mode = false;
bool botdontshoot = false;
bool botdontmove = false;
int bot_chat = 500;
int bot_allow_moods = 1;  // whether bots can have different personality traits or not
int bot_allow_humour = 1; // whether bots can choose to do daft things or not
bool bot_can_use_teleporter = true;
bool bot_can_build_teleporter = true;
int bot_use_grenades = 2;
bool bot_team_balance = false;
bool bot_bot_balance = false;
int min_bots = -1;
int max_bots = -1;
int bot_total_varies = 0;
float bot_create_interval = 3.0f;
int botskill_upper = 1;
int botskill_lower = 3;
int bot_skill_1_aim = 20;   // accuracy for skill 1 bots
int bot_aim_per_skill = 10; // accuracy modifier for bots from skill 1 downwards
bool bot_xmas = false;      // silly stuff
bool g_bot_debug = false;
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

extern msg_com_struct msg_com[MSG_MAX];
extern char msg_msg[64][MSG_MAX];

// const static double double_pi = 3.1415926535897932384626433832795;

// define the sources that a bot option/setting can be changed from
// used primarily by the changeBotSetting() function
enum {
	SETTING_SOURCE_CLIENT_COMMAND, // command issued at console by host on Listen server
	SETTING_SOURCE_SERVER_COMMAND, // command issued to the server directly
	SETTING_SOURCE_CONFIG_FILE
}; // command issued by a config file

namespace global {
	static FILE* fp;
}
DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0);

int mod_id = TFC_DLL;
int m_spriteTexture = 0;
static int isFakeClientCommand = 0;
static int fake_arg_count;
static float bot_check_time = 30.0f;
static edict_t* first_player = nullptr;
int num_bots = 0;
int prev_num_bots = 0;
bool g_GameRules = false;
edict_t* clients[32];
static int welcome_index = -1;
static int g_menu_waypoint;
static int g_menu_state = 0;

float is_team_play = 0.0f;
bool checked_teamplay = false;
// char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
int num_teams = 0;
edict_t* pent_info_tfdetect = nullptr;
edict_t* pent_info_ctfdetect = nullptr;
edict_t* pent_item_tfgoal = nullptr;
int max_team_players[4];
int team_class_limits[4];
int team_allies[4]; // bit mapped allies BLUE, RED, YELLOW, and GREEN
int max_teams = 0;
FLAG_S flags[MAX_FLAGS];
int num_flags = 0;

static FILE* bot_cfg_fp = nullptr;
// changed..cus we set it else where
static bool need_to_open_cfg = false;
static bool need_to_open_cfg2 = false;
static int cfg_file = 1;

// my display stuff...
static bool display_bot_vars = true;
static float display_start_time;
static bool script_loaded = false;
static bool script_parsed = false;

static short scanpos;
static bool player_vis[8];

static float bot_cfg_pause_time = 0.0f;
static float respawn_time = 0.0f;
static bool spawn_time_reset = false;
bool botcamEnabled = false;

chatClass chat; // bot chat stuff

// waypoint menu entries
const char* show_menu_1 = { "Waypoint Tags\n\n1. Team Specific\n2. Locations\n3. Items\n4. Actions p1\n5. Actions p2\n6. "
					 "Control Points\n7. Exit" };
const char* show_menu_2 = { "Team Specific Tags\n\n1. Team 1\n2. Team 2\n3. Team 3\n4. Team 4\n5. Exit" };
const char* show_menu_3 = { "Location Tags\n\n1. Flag Location\n2. Flag Goal Location\n3. Exit" };
const char* show_menu_4 = { "Item Tags\n\n1. Health\n2. Armour\n3. Ammo\n4. Exit" };
const char* show_menu_5 = { "Action Tags p1\n\n1. Defend (Soldier/HW/Demo)\n2. Defend(Demoman Only)\n3. Sniper\n4. Build "
					 "Sentry\n5. Rotate SG 180\n6. Build TP Entrance\n7. Build TP Exit\n8. Exit" };
const char* show_menu_6 = { "Action Tags p2\n\n1. RJ/CJ\n2. Jump\n3. Wait For Lift\n4. Walk\n5. Detpack(Clear "
					 "passageway)\n6. Detpack(Seal passageway)\n7. Path Check\n8. Exit" };
const char* show_menu_7 = { "Waypoint Tags\n\n1. Point1\n2. Point2\n3. Point3\n4. Point4\n5. Point5\n6. Point6\n7. Point7\n8. Point8\n9 Exit" };

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
void BotNameInit();
void UpdateClientData(const edict_s* ent, int sendweapons, clientdata_s* cd);
static void varyBotTotal();
static void TeamBalanceCheck();
static void BotBalanceTeams_Casual();
static bool BotBalanceTeams(int a, int b);
static bool BBotBalanceTeams(int a, int b);
static bool HBalanceTeams(int a, int b);
static void ProcessBotCfgFile();
static void changeBotSetting(const char* settingName, int* setting, const char* arg1, int minValue, int maxValue, int settingSource);
static void kickBots(int totalToKick, int team);
static void kickRandomBot();
static void ClearKickedBotsData(int botIndex, bool eraseBotsName);
// void UTIL_CSavePent(CBaseEntity *pent);

void FOX_HudMessage(edict_t* pEntity, const hudtextparms_t& textparms, const char* pMessage) {
	if (FNullEnt(pEntity))
		return;

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, pEntity);
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

	if (textparms.effect == 2)
		WRITE_SHORT(FixedUnsigned16(textparms.fxTime, 1 << 8));

	if (std::strlen(pMessage) < 512) {
		WRITE_STRING(pMessage);
	}
	else {
		char temp[512];
		std::strncpy(temp, pMessage, 511);
		temp[511] = '\0';
		WRITE_STRING(temp);
	}

	MESSAGE_END();
}

void KewlHUDNotify(edict_t* pEntity, const char* msg_name) {
	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, pEntity);
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(3 & 0xFF);
	WRITE_SHORT(FixedSigned16(1, 1 << 13));
	WRITE_SHORT(FixedSigned16(1, 1 << 13));
	WRITE_BYTE(2); // effect

	WRITE_BYTE(205);
	WRITE_BYTE(205);
	WRITE_BYTE(10);
	WRITE_BYTE(255);

	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(255);
	WRITE_BYTE(255);
	WRITE_SHORT(FixedUnsigned16(0.03f, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(1, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(6, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(4, 1 << 8));
	WRITE_STRING(msg_name);
	MESSAGE_END();
}

// If bot_total_varies is active then this function will periodically alter
// the number of bots that want to play on the server.
static void varyBotTotal() {
	if (bot_total_varies == 0)
		return; // just in case

	// this governs when the number of bots wanting to play will next change
	static float f_interested_bots_change = 0.0f;

	if (f_interested_bots_change < gpGlobals->time) {
		if (bot_total_varies == 3) // busy server(players coming/going often)
			f_interested_bots_change = gpGlobals->time + random_float(10.0f, 120.0f);
		else if (bot_total_varies == 2)
			f_interested_bots_change = gpGlobals->time + random_float(40.0f, 360.0f);
		else if (bot_total_varies == 1) // slow changes in number of bots
			f_interested_bots_change = gpGlobals->time + random_float(90.0f, 600.0f);

		//	UTIL_BotLogPrintf("interested_bots:%d, time:%f, next change:%f\n",
		//	interested_bots, gpGlobals->time, f_interested_bots_change);

		// try and get some bots interested in joining the
		// game if the game has just started
		if (interested_bots < 0) {
			if (max_bots > 0 && max_bots > min_bots) {
				if (min_bots > -1)
					interested_bots = random_long(min_bots, max_bots);
				else
					interested_bots = random_long(1, max_bots);
			}
			else
				interested_bots = min_bots;
		}

		// randomly increase/decrease the number of interested bots
		if (max_bots > 0 && max_bots > min_bots) {
			if (random_long(1, 1000) > 500) {
				// favor increasing the bots
				// if max_bots is reached decrease the bots
				if (interested_bots < max_bots)
					++interested_bots;
				else if (min_bots > 0) {
					if (interested_bots > min_bots)
						--interested_bots;
				}
				else if (interested_bots > 0)
					--interested_bots;
			}
			else {
				// favor decreasing the bots
				// if min_bots is reached increase the bots
				if (min_bots > 0 && interested_bots > min_bots)
					--interested_bots;
				if (min_bots < 1 && interested_bots > 0)
					--interested_bots;
				else if (interested_bots < max_bots)
					++interested_bots;
			}
		}

		//	UTIL_BotLogPrintf("interested_bots changed to:%d\n", interested_bots);
	}
	else {
		// make sure f_interested_bots_change is sane
		// (gpGlobals->time resets on map change)
		if (f_interested_bots_change - 601.0f > gpGlobals->time)
			f_interested_bots_change = gpGlobals->time + random_float(10.0f, 120.0f);
	}
}

// This function is the core function for checking if the teams need balancing
// and making sure the bots switch teams if they are supposed to.
static void TeamBalanceCheck() {
	if (mod_id != TFC_DLL) // Fix for bot_team_balance? [APG]RoboCop[CL]
		return;

	if (bot_team_balance && !bot_bot_balance) {
		// team 1 has more than team 2?
		bool done = BotBalanceTeams(1, 2);
		if (!done)
			done = BotBalanceTeams(1, 3);
		if (!done)
			done = BotBalanceTeams(1, 4);

		if (!done)
			done = BotBalanceTeams(2, 1);
		if (!done)
			done = BotBalanceTeams(2, 3);
		if (!done)
			done = BotBalanceTeams(2, 4);

		if (!done)
			done = BotBalanceTeams(3, 1);
		if (!done)
			done = BotBalanceTeams(3, 2);
		if (!done)
			done = BotBalanceTeams(3, 4);

		if (!done)
			done = BotBalanceTeams(4, 1);
		if (!done)
			done = BotBalanceTeams(4, 2);
		if (!done)
			BotBalanceTeams(4, 3);
	}
	if (bot_bot_balance) {
		// team 1 has more than team 2?
		bool done = BBotBalanceTeams(1, 2);
		if (!done)
			done = BBotBalanceTeams(1, 3);
		if (!done)
			done = BBotBalanceTeams(1, 4);

		if (!done)
			done = BBotBalanceTeams(2, 1);
		if (!done)
			done = BBotBalanceTeams(2, 3);
		if (!done)
			done = BBotBalanceTeams(2, 4);

		if (!done)
			done = BBotBalanceTeams(3, 1);
		if (!done)
			done = BBotBalanceTeams(3, 2);
		if (!done)
			done = BBotBalanceTeams(3, 4);

		if (!done)
			done = BBotBalanceTeams(4, 1);
		if (!done)
			done = BBotBalanceTeams(4, 2);
		if (!done)
			done = BBotBalanceTeams(4, 3);

		if (!done && bot_team_balance) {
			// balance the humans!
			// team 1 has more than team 2?
			done = HBalanceTeams(1, 2);
			if (!done)
				done = HBalanceTeams(1, 3);
			if (!done)
				done = HBalanceTeams(1, 4);

			if (!done)
				done = HBalanceTeams(2, 1);
			if (!done)
				done = HBalanceTeams(2, 3);
			if (!done)
				done = HBalanceTeams(2, 4);

			if (!done)
				done = HBalanceTeams(3, 1);
			if (!done)
				done = HBalanceTeams(3, 2);
			if (!done)
				done = HBalanceTeams(3, 4);

			if (!done)
				done = HBalanceTeams(4, 1);
			if (!done)
				done = HBalanceTeams(4, 2);
			if (!done)
				HBalanceTeams(4, 3);
		}
	}

	// if auto-balance is switched off then let the bots
	// balance the teams if they "feel" like it
	else if (!bot_team_balance && !bot_bot_balance)
		BotBalanceTeams_Casual();
}

// This function should only be called if bot_team_balance is switched off.
// It allows bots to decide for themselves if they "feel" like switching teams
// to balance the teams.
static void BotBalanceTeams_Casual() {
	if (mod_id != TFC_DLL) // Fix for bot_team_balance? [APG]RoboCop[CL]
		return;

	static float nextBalanceCheck = 5.0f;

	// perform a balance check every random number of seconds
	if (nextBalanceCheck < gpGlobals->time)
		nextBalanceCheck = gpGlobals->time + random_float(30.0f, 120.0f);
	else {
		// make sure nextBalanceCheck is sane(gpGlobals->time resets on map change)
		if (nextBalanceCheck - 600.0f > gpGlobals->time)
			nextBalanceCheck = gpGlobals->time + random_float(15.0f, 120.0f);
		return;
	}

	// find out which teams have player limits, and are full
	int i;
	bool team_is_full[4] = { false, false, false, false };
	for (i = 0; i < 4; i++) {
		if (max_team_players[i] > 0 && playersPerTeam[i] >= max_team_players[i])
			team_is_full[i] = true;
	}

	/*	UTIL_BotLogPrintf("team_is_full[] - 0:%d, 1:%d, 2:%d, 3:%d\n",
					team_is_full[0], team_is_full[1], team_is_full[2], team_is_full[3]);*/

					// find out which teams have the most and the fewest players
	int smallest_team = -1;
	int biggest_team = -1;
	int size_difference = -1;
	for (i = 0; i < 3; i++) {
		// stop comparing if the next team isn't used on this map
		if (is_team[i + 1] == false)
			break;

		// is this team bigger than the next one?
		if (!team_is_full[i + 1] && playersPerTeam[i] > playersPerTeam[i + 1]) {
			biggest_team = i;
			smallest_team = i + 1;
			size_difference = playersPerTeam[i] - playersPerTeam[i + 1];
		}
		// or is the next team bigger?
		else if (!team_is_full[i] && playersPerTeam[i] < playersPerTeam[i + 1]) {
			biggest_team = i + 1;
			smallest_team = i;
			size_difference = playersPerTeam[i + 1] - playersPerTeam[i];
		}

		if (size_difference > 1)
			break; // we've found a big enough imbalance
	}

	//	UTIL_BotLogPrintf("\nbiggest_team:%d, smallest_team:%d, size_difference:%d\n",
	//	biggest_team + 1, smallest_team + 1, size_difference);

	if (size_difference < 2 || biggest_team < 0 || smallest_team < 0)
		return;

	// add one to both teams as the bots use team numbers 1 to 4, not 0 to 3
	++biggest_team;
	++smallest_team;

	// let only one bot change team(if it wants to) once per function call
	for (i = 0; i < 32; i++) {
		// is this an active bot on the bigger team?
		// and is it willing to change teams?
		if (bots[i].is_used && bots[i].pEdict->v.team == biggest_team && random_long(1, 1000) < bots[i].trait.fairplay) {
			//	UTIL_BotLogPrintf("vteam:%d, team:%d\n",
			//		bots[i].pEdict->v.team, bots[i].bot_team);

			char msg[16];
			snprintf(msg, 16, "%d", smallest_team);
			msg[15] = '\0';
			//	FakeClientCommand(bots[i].pEdict, "jointeam", msg, NULL);
			bots[i].bot_team = smallest_team; // choose your team
			bots[i].not_started = true;       // join the team, pick a class
			bots[i].start_action = MSG_TFC_IDLE;
			bots[i].create_time = gpGlobals->time + 2.0f;
			ClearKickedBotsData(i, false);

			//	UTIL_BotLogPrintf("joined team:%d, vteam:%d, team:%d\n",
			//		smallest_team, bots[i].pEdict->v.team, bots[i].bot_team);

			break; // job done, success!
		}
	}
}

static bool BotBalanceTeams(int a, int b) {
	if (playersPerTeam[a - 1] - 1 > playersPerTeam[b - 1] && (max_team_players[b - 1] > playersPerTeam[b - 1] || max_team_players[b - 1] == 0) && is_team[b - 1]) {
		for (int i = 31; i >= 0; i--) {
			// is this slot used?
			if (bots[i].is_used && bots[i].pEdict->v.team == a) {
				char msg[32];
				snprintf(msg, 32, "%d", b);
				//	FakeClientCommand(bots[i].pEdict, "jointeam", msg, NULL);
				bots[i].bot_team = b;       // choose your team
				bots[i].not_started = true; // join the team, pick a class
				bots[i].start_action = MSG_TFC_IDLE;
				bots[i].create_time = gpGlobals->time + 2.0f;
				ClearKickedBotsData(i, false);
				return true;
			}
		}
	}
	return false;
}

// used with the bot_bot_balance setting
static bool BBotBalanceTeams(int a, int b) {
	// now just set up teams to include bots in them :D
	int bteams[4] = { 0, 0, 0, 0 };

	for (int i = 0; i < 32; i++) ///<
	{
		if (bots[i].is_used) {
			char cl_name[128];
			cl_name[0] = '\0';
			char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(bots[i].pEdict);
			std::strcpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
			if (cl_name[0] != '\0') {
				const int team = bots[i].pEdict->v.team - 1;

				if (team >= 0 && team < 4)
					bteams[team] = bteams[team] + 1;
			}
		}
	}
	if (bteams[a - 1] - 1 > bteams[b - 1] && (max_team_players[b - 1] > playersPerTeam[b - 1] || max_team_players[b - 1] == 0) && is_team[b - 1]) {
		for (int i = 31; i >= 0; i--) {
			// is this slot used?
			if (bots[i].is_used && bots[i].pEdict->v.team == a) {
				char msg[32];
				snprintf(msg, 32, "%d", b);
				//	FakeClientCommand(bots[i].pEdict, "jointeam", msg, NULL);
				bots[i].bot_team = b;       // choose your team
				bots[i].not_started = true; // join the team, pick a class
				bots[i].start_action = MSG_TFC_IDLE;
				bots[i].create_time = gpGlobals->time + 2.0f;
				ClearKickedBotsData(i, false);
				return true;
			}
		}
	}
	return false;
}

// This function will balance the human players amongst the teams by forcing
// them to switch teams if necessary.
static bool HBalanceTeams(int a, int b) {
	if (playersPerTeam[a - 1] - 1 > playersPerTeam[b - 1] && (max_team_players[b - 1] > playersPerTeam[b - 1] || max_team_players[b - 1] == 0) && is_team[b - 1]) {
		for (int i = 1; i <= 32; i++) {
			bool not_bot = true;
			for (int j = 31; j >= 0; j--) {
				if (bots[j].is_used && bots[j].pEdict == INDEXENT(i))
					not_bot = false;
			}

			if (not_bot && INDEXENT(i) != nullptr) {
				if (INDEXENT(i)->v.team == a && INDEXENT(i)->v.netname != 0) {
					CLIENT_COMMAND(INDEXENT(i), UTIL_VarArgs("jointeam %d\n", b));
					return true;
				}
			}
		}
	}
	return false;
}

// This function is called once upon startup
void GameDLLInit() {
	CVAR_REGISTER(&foxbot);
	CVAR_REGISTER(&sv_bot);
	CVAR_REGISTER(&enable_foxbot);

	for (int i = 0; i < 32; i++)
		clients[i] = nullptr;

	// initialize the bots array of structures...
	std::memset(bots, 0.0f, sizeof bots);

	// read the bot names from the bot name file
	BotNameInit();

	// read the chat strings from the bot chat file
	chat.readChatFile();

	if (!mr_meta)
		(*other_gFunctionTable.pfnGameInit)();
	else
		SET_META_RESULT(MRES_IGNORED);
}

// Constructor for the chatClass class
// sets up the names of the chat section headers
chatClass::chatClass() {
	this->section_names_[CHAT_TYPE_GREETING] = "[GREETINGS]";
	this->section_names_[CHAT_TYPE_KILL_HI] = "[KILL WINNING]";
	this->section_names_[CHAT_TYPE_KILL_LOW] = "[KILL LOSING]";
	this->section_names_[CHAT_TYPE_KILLED_HI] = "[KILLED WINNING]";
	this->section_names_[CHAT_TYPE_KILLED_LOW] = "[KILLED LOSING]";
	this->section_names_[CHAT_TYPE_SUICIDE] = "[SUICIDE]";

	int j;

	// explicitly clear all the chat strings
	for (int i = 0; i < TOTAL_CHAT_TYPES; i++) {
		this->string_count_[i] = 0;
		for (j = 0; j < MAX_CHAT_STRINGS; j++) {
			this->strings_[i][j][0] = 0x0;
			//this->strings_[i][j] = "";
		}

		for (j = 0; j < 5; j++) {
			this->recent_strings_[i][j] = -1;
		}
	}
}

// This function is responsible for reading in the chat from the bot chat file.
void chatClass::readChatFile() {
	char filename[256];

	UTIL_BuildFileName(filename, 255, "foxbot_chat.txt", nullptr);
	FILE* bfp = std::fopen(filename, "r");

	if (bfp == nullptr) {
		UTIL_BotLogPrintf("Unable to read from the Foxbot chat file.  The bots will not chat.");
		bot_chat = 0; // stop the bots trying to chat
		return;
	}

	char buffer[MAX_CHAT_LENGTH] = "";
	char* ptr;
	int chat_section = -1;

	while (UTIL_ReadFileLine(buffer, MAX_CHAT_LENGTH, bfp)) {
		// ignore comment lines
		if (buffer[0] == '#')
			continue;

		size_t length = std::strlen(buffer);

		// turn '\n' into '\0'
		if (buffer[length - 1] == '\n') {
			buffer[length - 1] = 0;
			length--;
		}

		// change %n to %s
		if ((ptr = std::strstr(buffer, "%n")) != nullptr)
			*(ptr + 1) = 's';

		// watch out for the chat section headers
		if (buffer[0] == '[') {
			bool newSectionFound = false;
			for (int i = 0; i < TOTAL_CHAT_TYPES; i++) {
				if (buffer == this->section_names_[i]) {
					chat_section = i;
					newSectionFound = true;
				}
			}
			if (newSectionFound)
				continue;
		}

		// this line is not a comment, empty, or a section header
		// so treat it as a chat string and load it up
		if (chat_section != -1 && this->string_count_[chat_section] < MAX_CHAT_STRINGS && length > 0) {
			// UTIL_BotLogPrintf("buffer %s %d\n", buffer, this->stringCount[chat_section]);

			this->strings_[chat_section][this->string_count_[chat_section]] = buffer;
			++this->string_count_[chat_section];
		}
	}

	std::fclose(bfp);
}

// This function will return a C ASCII string pointer to a randomly selected
// chat message of the type defined by chatSection.
// Some chat strings use a players name with the "%n" specifier, so you can
// specify a players name with playerName, or set it to NULL.
void chatClass::pickRandomChatString(char* msg, size_t maxLength, const int chatSection, const char* playerName) {
	msg[0] = '\0'; // just in case

	// make sure this chat section contains at least one chat string
	if (this->string_count_[chatSection] < 1)
		return;

	int i, recentCount = 0;
	int randomIndex = 0;

	// try to pick a string that hasn't been used recently
	while (recentCount < 5) {
		randomIndex = random_long(0, this->string_count_[chatSection] - 1);
		bool used = false;
		for (i = 0; i < 5; i++) {
			if (this->recent_strings_[chatSection][i] == randomIndex)
				used = true;
		}

		if (used)
			++recentCount;
		else
			break; // found an unused chat string
	}

	// push the selected string on to the list of recently used strings
	for (i = 4; i > 0; i--) {
		this->recent_strings_[chatSection][i] = this->recent_strings_[chatSection][i - 1];
	}
	this->recent_strings_[chatSection][0] = randomIndex;

	// set up the message string
	// is "%s" in the text?
	if (playerName != nullptr && std::strstr(this->strings_[chatSection][randomIndex].c_str(), "%s") != nullptr) {
		snprintf(msg, maxLength, this->strings_[chatSection][randomIndex].c_str(), playerName);
	}
	else
		snprintf(msg, maxLength, this->strings_[chatSection][randomIndex].c_str(), "%s");

	msg[maxLength - 1] = '\0';
}

int DispatchSpawn(edict_t* pent) {
	if (gpGlobals->deathmatch) {
		const char* pClassname = STRING(pent->v.classname);

		if (debug_engine) {
			global::fp = UTIL_OpenFoxbotLog();
			std::fprintf(global::fp, "DispatchSpawn: %p %s\n", static_cast<void*>(pent), pClassname);
			if (pent->v.model != 0)
				std::fprintf(global::fp, " model=%s\n", STRING(pent->v.model));
			if (pent->v.target != 0)
				std::fprintf(global::fp, " t=%s\n", STRING(pent->v.target));
			if (pent->v.targetname != 0)
				std::fprintf(global::fp, " tn=%s\n", STRING(pent->v.targetname));
			std::fclose(global::fp);
		}

		if (std::strcmp(pClassname, "worldspawn") == 0) {
			// do level initialization stuff here...

			WaypointInit();
			WaypointLoad(nullptr);
			AreaDefLoad(nullptr);

			// my clear var for lev reload..
			std::strcpy(prevmapname, "null");

			pent_info_tfdetect = nullptr;
			pent_info_ctfdetect = nullptr;
			pent_item_tfgoal = nullptr;

			for (int index = 0; index < 4; index++) {
				max_team_players[index] = 0;  // no player limit
				team_class_limits[index] = 0; // no class limits
				team_allies[index] = 0;
			}

			max_teams = 0;
			is_team[0] = false;
			is_team[1] = false;
			is_team[2] = false;
			is_team[3] = false;
			num_flags = 0;

			g_waypoint_cache = false;
			if (g_waypoint_on || g_area_def) {
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

				g_waypoint_cache = true;
			}

			PRECACHE_MODEL("models/presentlg.mdl");
			PRECACHE_MODEL("models/presentsm.mdl");

			//PRECACHE_SOUND("barney/c1a4_ba_octo4.wav");
			PRECACHE_SOUND("misc/b2.wav");
			PRECACHE_SOUND("misc/party2.wav");
			PRECACHE_SOUND("misc/party1.wav");

			m_spriteTexture = PRECACHE_MODEL("sprites/lgtning.spr");

			g_GameRules = true;

			is_team_play = 0.0f;
			//	memset(team_names, 0, sizeof(team_names));
			num_teams = 0;
			checked_teamplay = false;

			respawn_time = 0.0f;
			spawn_time_reset = false;

			prev_num_bots = num_bots;
			num_bots = 0;

			bot_check_time = gpGlobals->time + 30.0f;
		}
	}

	if (!mr_meta)
		return (*other_gFunctionTable.pfnSpawn)(pent);
	RETURN_META_VALUE(MRES_HANDLED, 0);
}

void DispatchThink(edict_t* pent) {
	if (FStrEq(STRING(pent->v.classname), "entity_botcam")) {
		TraceResult tr;
		int off_f = 16;
		UTIL_MakeVectors(pent->v.euser1->v.v_angle);
		if (pent->v.euser1 != nullptr && !FNullEnt(pent->v.euser1) && pent->v.owner != nullptr && !FNullEnt(pent->v.owner)) {
			bot_t* pBot = UTIL_GetBotPointer(pent->v.euser1);

			UTIL_TraceLine(pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs + gpGlobals->v_forward * static_cast<float>(off_f), pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs + gpGlobals->v_forward * 4000, ignore_monsters, pent->v.euser1, &tr);

			MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, pent->v.owner);
			WRITE_BYTE(TE_TEXTMESSAGE);
			WRITE_BYTE(2 & 0xFF);
			WRITE_SHORT(FixedSigned16(1, 1 << 13));
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
			if (pBot->mission == ROLE_ATTACKER) {
				snprintf(msg, 254, "BotCam: Role:Attacker\n ammoStatus:%d deathsTillClassChange:%d trait.health:%d", pBot->ammoStatus, pBot->deathsTillClassChange, pBot->trait.health);
				WRITE_STRING(msg);
			}
			else if (pBot->mission == ROLE_DEFENDER) {
				snprintf(msg, 254, "BotCam: Role:Defender\n ammoStatus:%d deathsTillClassChange:%d trait.health:%d", pBot->ammoStatus, pBot->deathsTillClassChange, pBot->trait.health);
				WRITE_STRING(msg);
			}
			else {
				snprintf(msg, 254, "BotCam: Role:None\n ammoStatus:%d deathsTillClassChange:%d trait.health:%d", pBot->ammoStatus, pBot->deathsTillClassChange, pBot->trait.health);
				WRITE_STRING(msg);
			}
			MESSAGE_END();

			if (pBot->enemy.ptr != nullptr) {
				Vector end;
				Vector st;
				float zz = pBot->enemy.ptr->v.maxs.z;
				zz = zz / 4 / 2;
				float xx = pBot->enemy.ptr->v.maxs.x;
				xx = xx / 2;

				if (scanpos > 1)
					zz = zz * 2;
				if (scanpos > 3)
					zz = zz * 2;
				if (scanpos > 5)
					zz = zz * 2;
				if (scanpos == 0 || scanpos == 2 || scanpos == 4 || scanpos == 6)
					xx = -xx;
				float xo = xx;
				Vector vang = pBot->enemy.ptr->v.origin - pBot->pEdict->v.origin;
				float distance = vang.Length();
				vang = UTIL_VecToAngles(vang);
				vang.y = vang.y + 45;
				if (vang.y < 0)
					vang.y += 360;
				vang.y = vang.y * static_cast<float>(M_PI);
				vang.y = vang.y / 180;
				xx = xo * std::cos(vang.y);
				float yy = xo * std::sin(vang.y);

				tr.pHit = nullptr;
				UTIL_TraceLine(pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs, pBot->enemy.ptr->v.origin + Vector(xx, yy, zz), dont_ignore_monsters, dont_ignore_glass, pent->v.euser1, &tr);

				if (tr.pHit == pBot->enemy.ptr || tr.flFraction == 1.0f)
					player_vis[scanpos] = true;
				else
					player_vis[scanpos] = false;
				scanpos++;
				if (scanpos > 7) // 7
					scanpos = 0;
				float p_vis = 0;
				for (int i = 0; i < 8; i++) {
					if (player_vis[i])
						p_vis = p_vis + 12.5f;
				}

				char text[511];
				int amb = 0;
				float vidsize = 2.0f;
				float sz = pBot->enemy.ptr->v.maxs.z * (vidsize / distance) * 100;
				int d = GETENTITYILLUM(pBot->enemy.ptr);
				if (amb == 0) {
					edict_t* s = nullptr;
					edict_t* pPoint = nullptr;
					while ((s = FIND_ENTITY_IN_SPHERE(s, pBot->enemy.ptr->v.origin, 50)) != nullptr && !FNullEnt(s) && amb == 0) {
						if (std::strcmp(STRING(s->v.classname), "entity_botlightvalue") == 0)
							pPoint = s;
					}
					if (pPoint == nullptr) {
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
					}
					else {
						amb = GETENTITYILLUM(pPoint);
						REMOVE_ENTITY(pPoint);
					}
				}

				Vector v = pBot->pEdict->v.v_angle;
				v.y = v.y + 180;
				if (v.y > 180)
					v.y -= 360;
				if (v.y < -180)
					v.y += 360;

				double dgrad = v.y;
				dgrad = dgrad + 180;
				if (dgrad > 180)
					dgrad -= 360;
				if (dgrad < -180)
					dgrad += 360;
				dgrad = dgrad * M_PI;
				dgrad = dgrad / 180;

				Vector vel = pBot->enemy.ptr->v.velocity;
				vel.x = vel.x * std::sin(dgrad);
				vel.y = vel.y * std::cos(dgrad);
				dgrad = v.x;
				dgrad = dgrad + 180;
				if (dgrad > 180)
					dgrad -= 360;
				if (dgrad < -180)
					dgrad += 360;
				dgrad = dgrad * M_PI;
				dgrad = dgrad / 180;
				vel.z = vel.z * std::cos(dgrad);

				if (vel.x < 0)
					vel.x = -vel.x;
				if (vel.y < 0)
					vel.y = -vel.y;
				if (vel.z < 0)
					vel.z = -vel.z;

				float veloff = vel.x + vel.y + vel.z;
				veloff = veloff * (vidsize / distance) * 10;

				snprintf(text, 510, "Vis %.1f\nAmb %d %d\nSz %.1f\nveloff %.1f", p_vis, amb, d, sz, veloff);

				MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, pent->v.owner);
				WRITE_BYTE(TE_TEXTMESSAGE);
				WRITE_BYTE(4 & 0xFF);
				WRITE_SHORT(FixedSigned16(0.3f, 1 << 13));
				WRITE_SHORT(FixedSigned16(0.3f, 1 << 13));
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
				WRITE_STRING(text);
				MESSAGE_END();

				MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, pent->v.owner);
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
			pent->v.euser1->v.origin + pent->v.euser1->v.view_ofs + gpGlobals->v_forward * static_cast<float>(off_f), // + gpGlobals->v_up * 10
			ignore_monsters, pent->v.euser1, &tr);

		// UTIL_SetOrigin(pent, tr.vecEndPos);
		pent->v.origin = tr.vecEndPos;

		pent->v.angles = pent->v.euser1->v.v_angle;
		pent->v.velocity = Vector(0, 0, 0);
		if (pent->v.angles.y >= 360)
			pent->v.angles.y -= 360;
		if (pent->v.angles.y < 0)
			pent->v.angles.y += 360;
		pent->v.nextthink = gpGlobals->time;
	}
	if (!mr_meta)
		(*other_gFunctionTable.pfnThink)(pent);
	else
		SET_META_RESULT(MRES_HANDLED);
}

void DispatchKeyValue(edict_t* pentKeyvalue, KeyValueData* pkvd) {
	if (mod_id == TFC_DLL) {
		static int flag_index;
		static edict_t* temp_pent;
		if (pentKeyvalue == pent_info_tfdetect) {
			if (std::strcmp(pkvd->szKeyName, "ammo_medikit") == 0) // max BLUE players
				max_team_players[0] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "ammo_detpack") == 0) // max RED players
				max_team_players[1] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_medikit") == 0) // max YELLOW players
				max_team_players[2] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_detpack") == 0) // max GREEN players
				max_team_players[3] = atoi(pkvd->szValue);

			else if (std::strcmp(pkvd->szKeyName, "maxammo_shells") == 0) // BLUE class limits
				team_class_limits[0] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_nails") == 0) // RED class limits
				team_class_limits[1] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_rockets") == 0) // YELLOW class limits
				team_class_limits[2] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_cells") == 0) // GREEN class limits
				team_class_limits[3] = atoi(pkvd->szValue);

			else if (std::strcmp(pkvd->szKeyName, "team1_allies") == 0) // BLUE allies
				team_allies[0] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "team2_allies") == 0) // RED allies
				team_allies[1] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "team3_allies") == 0) // YELLOW allies
				team_allies[2] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "team4_allies") == 0) // GREEN allies
				team_allies[3] = atoi(pkvd->szValue);
		}
		else if (pent_info_tfdetect == nullptr) {
			if (std::strcmp(pkvd->szKeyName, "classname") == 0 && std::strcmp(pkvd->szValue, "info_tfdetect") == 0) {
				pent_info_tfdetect = pentKeyvalue;
			}
		}

		if (pentKeyvalue == pent_item_tfgoal) {
			if (std::strcmp(pkvd->szKeyName, "team_no") == 0)
				flags[flag_index].team_no = atoi(pkvd->szValue);

			if (std::strcmp(pkvd->szKeyName, "mdl") == 0 && (std::strcmp(pkvd->szValue, "models/flag.mdl") == 0 || std::strcmp(pkvd->szValue, "models/keycard.mdl") == 0 || std::strcmp(pkvd->szValue, "models/ball.mdl") == 0)) {
				flags[flag_index].mdl_match = true;
				num_flags++;
			}
		}
		else if (pent_item_tfgoal == nullptr) {
			if (std::strcmp(pkvd->szKeyName, "classname") == 0 && std::strcmp(pkvd->szValue, "item_tfgoal") == 0) {
				if (num_flags < MAX_FLAGS) {
					pent_item_tfgoal = pentKeyvalue;

					flags[num_flags].mdl_match = false;
					flags[num_flags].team_no = 0; // any team unless specified
					flags[num_flags].edict = pentKeyvalue;

					flag_index = num_flags; // in case the mdl comes before team_no
				}
			}
		}
		else
			pent_item_tfgoal = nullptr; // reset for non-flag item_tfgoal's

		if (std::strcmp(pkvd->szKeyName, "classname") == 0 && (std::strcmp(pkvd->szValue, "info_player_teamspawn") == 0 || std::strcmp(pkvd->szValue, "info_tf_teamcheck") == 0 || std::strcmp(pkvd->szValue, "i_p_t") == 0)) {
			temp_pent = pentKeyvalue;
		}
		else if (pentKeyvalue == temp_pent) {
			if (std::strcmp(pkvd->szKeyName, "team_no") == 0) {
				const int value = atoi(pkvd->szValue);

				is_team[value - 1] = true;
				if (value > max_teams)
					max_teams = value;
			}
		}
	}

	if (!mr_meta)
		(*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
	else
		SET_META_RESULT(MRES_HANDLED);
}

BOOL ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) {
	if (gpGlobals->deathmatch) {
		// int count = 0;
		if (debug_engine) {
			global::fp = UTIL_OpenFoxbotLog();
			if (global::fp != nullptr) {
				std::fprintf(global::fp, "ClientConnect: pent=%p name=%s\n", static_cast<void*>(pEntity), pszName);
				std::fclose(global::fp);
			}
		}

		// ALERT(at_console, "[FOXBOT] This server is running FoxBot (%d build %d), Get it at foxbot.net!\n", VER_MINOR,
		// VER_BUILD);
		if (!mr_meta) {
			char msg[255];
			// std::sprintf(msg, "[FOXBOT] This server is running FoxBot (v %d build %d), Get it at foxbot.net!\n",
			// VER_MINOR, VER_BUILD);
			// std::sprintf(msg, "[FOXBOT] This server is running FoxBot (v%d.%d), Get it at foxbot.net!\n", VER_MAJOR,
			// VER_MINOR);
			snprintf(msg, 254, "[FOXBOT] This server is running FoxBot (v%d.%d), get it at www.apg-clan.org\n", VER_MAJOR, VER_MINOR);
			CLIENT_PRINTF(pEntity, print_console, msg);
		}

		// check if this is NOT a bot joining the server...
		if (std::strcmp(pszAddress, "127.0.0.1") != 0) {
			int i = 0;
			while (i < 32 && (clients[i] != nullptr && clients[i] != pEntity))
				i++;
			if (i < 32)
				clients[i] = pEntity;
			if (welcome_index == -1)
				welcome_index = i;
			// don't try to add bots for 30 seconds, give client time to get added
			bot_check_time = gpGlobals->time + 30.0f;
			// save the edict of the first player to join this server...
			if (first_player == nullptr)
				first_player = pEntity;
		}
	}

	if (!mr_meta)
		return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
	RETURN_META_VALUE(MRES_HANDLED, true);
}

BOOL ClientConnect_Post(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) {
	if (gpGlobals->deathmatch) {
		if (debug_engine) {
			global::fp = UTIL_OpenFoxbotLog();
			if (global::fp != nullptr) {
				std::fprintf(global::fp, "ClientConnect_Post: pent=%p name=%s\n", static_cast<void*>(pEntity), pszName);
				std::fclose(global::fp);
			}
		}

		// ALERT(at_console, "[FOXBOT] This server is running FoxBot (%d build %d), Get it at foxbot.net!\n", VER_MINOR,
		// VER_BUILD);
		char msg[255];
		// std::sprintf(msg, "[FOXBOT] This server is running FoxBot (v %d build %d), Get it at foxbot.net!\n", VER_MINOR,
		// VER_BUILD);
		// std::sprintf(msg, "[FOXBOT] This server is running FoxBot (v%d.%d), Get it at foxbot.net!\n", VER_MAJOR,
		// VER_MINOR);
		snprintf(msg, 254, "[FOXBOT] This server is running FoxBot (v%d.%d), get it at www.apg-clan.org\n", VER_MAJOR, VER_MINOR);
		CLIENT_PRINTF(pEntity, print_console, msg);
	}
	RETURN_META_VALUE(MRES_HANDLED, true);
}

void ClientDisconnect(edict_t* pEntity) {
	if (gpGlobals->deathmatch) {
		if (debug_engine) {
			global::fp = UTIL_OpenFoxbotLog();
			if (global::fp != nullptr) {
				std::fprintf(global::fp, "ClientDisconnect: %p\n", static_cast<void*>(pEntity));
				std::fclose(global::fp);
			}
		}

		int i, index = -1;

		for (i = 0; i < 32; i++) {
			if (bots[i].pEdict == pEntity && bots[i].is_used) {
				index = i;
				break;
			}
		}

		if (index != -1) {
			// someone has kicked this bot off of the server...

			ClearKickedBotsData(index, false);
			bots[index].is_used = false;               // this bot index is now free to re-use
			bots[index].f_kick_time = gpGlobals->time; // save the kicked time
		}
		else {
			i = 0;

			while (i < 32 && clients[i] != pEntity)
				i++;

			if (i < 32)
				clients[i] = nullptr;
			// human left?
			// what about level changes?
		}
	}

	if (!mr_meta)
		(*other_gFunctionTable.pfnClientDisconnect)(pEntity);
	else {
		RETURN_META(MRES_HANDLED);
	}
}

void ClientCommand(edict_t* pEntity) {
	if (mod_id == TFC_DLL && pEntity != nullptr) {
		const char* pcmd = CMD_ARGV(0);
		const char* arg1 = CMD_ARGV(1);
		const char* arg2 = CMD_ARGV(2);
		const char* arg3 = CMD_ARGV(3);
		const char* arg4 = CMD_ARGV(4);
		// char msg[80];
		if (debug_engine) {
			global::fp = UTIL_OpenFoxbotLog();
			std::fprintf(global::fp, "ClientCommand: %s %p", pcmd, static_cast<void*>(pEntity));
			if (arg1 != nullptr) {
				if (*arg1 != 0)
					std::fprintf(global::fp, " 1:%s", arg1);
			}
			if (arg2 != nullptr) {
				if (*arg2 != 0)
					std::fprintf(global::fp, " 2:%s", arg2);
			}
			if (arg3 != nullptr) {
				if (*arg3 != 0)
					std::fprintf(global::fp, " 3:%s", arg3);
			}
			if (arg4 != nullptr) {
				if (*arg4 != 0)
					std::fprintf(global::fp, " 4:%s", arg4);
			}
			std::fprintf(global::fp, "\n");
			std::fclose(global::fp);
		}
	}

	if (gpGlobals->deathmatch && !IS_DEDICATED_SERVER() && pEntity == INDEXENT(1)) {
		const char* pcmd = CMD_ARGV(0);
		const char* arg1 = CMD_ARGV(1);
		const char* arg2 = CMD_ARGV(2);
		const char* arg3 = CMD_ARGV(3);
		const char* arg4 = CMD_ARGV(4);
		char msg[512];

		if (debug_engine) {
			global::fp = UTIL_OpenFoxbotLog();
			std::fprintf(global::fp, "ClientCommand: %s %p", pcmd, static_cast<void*>(pEntity));
			if (arg1 != nullptr) {
				if (*arg1 != 0)
					std::fprintf(global::fp, " '%s'(1)", arg1);
			}
			if (arg2 != nullptr) {
				if (*arg2 != 0)
					std::fprintf(global::fp, " '%s'(2)", arg2);
			}
			if (arg3 != nullptr) {
				if (*arg3 != 0)
					std::fprintf(global::fp, " '%s'(3)", arg3);
			}
			if (arg4 != nullptr) {
				if (*arg4 != 0)
					std::fprintf(global::fp, " '%s'(4)", arg4);
			}
			std::fprintf(global::fp, "\n");
			std::fclose(global::fp);
		}

		if (FStrEq(pcmd, "addbot") || FStrEq(pcmd, "foxbot_addbot")) {
			if (arg2 != nullptr && *arg2 != 0)
				BotCreate(pEntity, arg1, arg2, arg3, arg4);
			else {
				char c[8];
				std::strcpy(c, "-1");
				BotCreate(pEntity, arg1, c, arg3, arg4);
			}
			bot_check_time = gpGlobals->time + 5.0f;
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		if (FStrEq(pcmd, "min_bots")) {
			changeBotSetting("min_bots", &min_bots, arg1, -1, 31, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		if (FStrEq(pcmd, "max_bots")) {
			changeBotSetting("max_bots", &max_bots, arg1, -1, MAX_BOTS, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		if (FStrEq(pcmd, "bot_total_varies")) {
			changeBotSetting("bot_total_varies", &bot_total_varies, arg1, 0, 3, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		if (FStrEq(pcmd, "bot_info")) {
			DisplayBotInfo();
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		if (FStrEq(pcmd, "bot_team_balance")) {
			if (arg1 != nullptr) {
				if (*arg1 != 0) {
					int temp;
					temp = atoi(arg1);
					if (temp)
						bot_team_balance = true;
					else
						bot_team_balance = false;
				}
			}
			if (bot_team_balance)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_team_balance (1) On\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_team_balance (0) Off\n");
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		if (FStrEq(pcmd, "bot_bot_balance")) {
			if (arg1 != nullptr) {
				if (*arg1 != 0) {
					int temp;
					temp = atoi(arg1);
					if (temp)
						bot_bot_balance = true;
					else
						bot_bot_balance = false;
				}
			}

			if (bot_bot_balance)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_bot_balance (1) On\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_bot_balance (0) Off\n");

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		if (FStrEq(pcmd, "kickall") || FStrEq(pcmd, "foxbot_kickall")) {
			// kick all bots off of the server after time/frag limit...
			kickBots(MAX_BOTS, -1);
		}
		else if (FStrEq(pcmd, "kickteam") || FStrEq(pcmd, "foxbot_kickteam")) {
			if (arg1 != nullptr && *arg1 != 0) {
				int whichTeam = atoi(arg1);
				kickBots(MAX_BOTS, whichTeam);
			}
		}
		else if (FStrEq(pcmd, "observer")) {
			if (arg1 != nullptr && *arg1 != 0) {
				int temp;
				temp = atoi(arg1);
				if (temp)
					observer_mode = true;
				else
					observer_mode = false;
			}

			if (observer_mode)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "botskill_lower")) {
			changeBotSetting("botskill_lower", &botskill_lower, arg1, 1, 5, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "botskill_upper")) {
			changeBotSetting("botskill_upper", &botskill_upper, arg1, 1, 5, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_skill_1_aim")) {
			changeBotSetting("bot_skill_1_aim", &bot_skill_1_aim, arg1, 0, 200, SETTING_SOURCE_CLIENT_COMMAND);

			BotUpdateSkillInaccuracy();
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_aim_per_skill")) {
			changeBotSetting("bot_aim_per_skill", &bot_aim_per_skill, arg1, 5, 50, SETTING_SOURCE_CLIENT_COMMAND);

			BotUpdateSkillInaccuracy();
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "spectate_debug")) {
			changeBotSetting("spectate_debug", &spectate_debug, arg1, 0, 4, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "botdontshoot")) {
			if (arg1 != nullptr) {
				if (*arg1 != 0) {
					int temp;
					temp = atoi(arg1);
					if (temp)
						botdontshoot = true;
					else
						botdontshoot = false;
				}
			}

			if (botdontshoot)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot ENABLED\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot DISABLED\n");

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "botdontmove")) {
			if (arg1 != nullptr) {
				if (*arg1 != 0) {
					int temp;
					temp = atoi(arg1);
					if (temp)
						botdontmove = true;
					else
						botdontmove = false;
				}
			}

			if (botdontmove)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontmove ENABLED\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontmove DISABLED\n");

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "debug_engine") || FStrEq(pcmd, "foxbot_debug_engine")) {
			if (FStrEq(arg1, "on")) {
				debug_engine = 1;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine enabled!\n");
			}
			else if (FStrEq(arg1, "off")) {
				debug_engine = 0;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine disabled!\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}

		// botcam
		else if (FStrEq(pcmd, "botcam")) {
			edict_t* pBot = nullptr;
			char botname[BOT_NAME_LEN + 1];
			int index;

			botname[0] = 0;

			if (arg1 != nullptr) {
				if (*arg1 != 0) {
					if (std::strchr(arg1, '\"') == nullptr)
						std::strcpy(botname, arg1);
					else
						std::sscanf(arg1, R"("%s")", &botname[0]);

					index = 0;

					while (index < 32) {
						if (bots[index].is_used && strcasecmp(bots[index].name, botname) == 0)
							break;
						index++;
					}

					if (index < 32) {
						pBot = bots[index].pEdict;
					}
				}
			}
			else {
				index = 0;

				while (bots[index].is_used == false && index < 32)
					index++;

				if (index < 32) {
					pBot = bots[index].pEdict;
				}
			}

			if (pBot == nullptr) {
				if (botname[0])
					CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs("there is no bot named \"%s\"!\n", botname));
				else
					CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs("there are no bots!\n"));
			}
			else {
				if (!g_waypoint_cache)
					CLIENT_PRINTF(pEntity, print_console, "Turn waypoints on so proper entities can be cached. Then retry botcam.");
				else {
					KillCamera(pEntity);
					CreateCamera(pEntity, pBot);
					botcamEnabled = true;
				}
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "nobotcam")) {
			KillCamera(pEntity);
			botcamEnabled = false;
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "waypoint_author")) {
			if (arg1 != nullptr) {
				if (*arg1 != 0) {
					char message[512];
					std::sprintf(message, "Waypoint author set to : %s", arg1);
					CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs(message));
					std::strncpy(waypoint_author, arg1, 250);
					waypoint_author[251] = '\0';

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
					h.y = 0.8f;
					std::sprintf(message, "-- Waypoint author: %s --", waypoint_author);
					FOX_HudMessage(INDEXENT(1), h, message);
				}
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "waypoint")) {
			if (FStrEq(arg1, "on")) {
				g_waypoint_on = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_waypoint_on = false;
				g_path_oneway = false;
				g_path_twoway = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
			}
			else if (FStrEq(arg1, "add")) {
				if (!g_waypoint_on)
					g_waypoint_on = true; // turn waypoints on if off

				WaypointAdd(pEntity);
			}
			else if (FStrEq(arg1, "delete") && g_waypoint_on) {
				if (!g_waypoint_on)
					g_waypoint_on = true; // turn waypoints on if off

				WaypointDelete(pEntity);
			}
			else if (FStrEq(arg1, "save") && g_waypoint_on) {
				WaypointSave();

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints saved\n");
			}
			else if (FStrEq(arg1, "load")) {
				if (WaypointLoad(pEntity))
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");
			}
			else if (FStrEq(arg1, "menu") && g_waypoint_on) {
				int index = WaypointFindNearest_E(pEntity, 50.0f, -1);
				if (num_waypoints > 0 && index != -1) {
					g_menu_waypoint = index;
					g_menu_state = MENU_1;

					UTIL_ShowMenu(pEntity, 0x7F, -1, false, show_menu_1);
					//	UTIL_ShowMenu(pEntity, 0x3F, -1, false, show_menu_1);
				}
			}
			else if (FStrEq(arg1, "info")) {
				WaypointPrintInfo(pEntity);
			}
			else if (FStrEq(arg1, "autobuild")) {
				if (!g_waypoint_on)
					g_waypoint_on = true; // turn waypoints on if off

				WaypointAutoBuild(pEntity);
			}
			else {
				if (g_waypoint_on)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
			}

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "autowaypoint")) {
			if (FStrEq(arg1, "on")) {
				g_auto_waypoint = true;
				g_waypoint_on = true; // turn this on just in case
			}
			else if (FStrEq(arg1, "off")) {
				g_auto_waypoint = false;
			}

			if (g_auto_waypoint)
				std::sprintf(msg, "autowaypoint is ON\n");
			else
				std::sprintf(msg, "autowaypoint is OFF\n");

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_debug")) {
			if (FStrEq(arg1, "on")) {
				CVAR_SET_STRING("developer", "1");
				g_bot_debug = true;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot debugging is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_bot_debug = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot debugging is OFF\n");
				CVAR_SET_STRING("developer", "0");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "available")) {
			int blue_state, red_state, yellow_state, green_state;
			for (int i = 0; i < 8; i++) {
				blue_state = red_state = yellow_state = green_state = 0;

				if (blue_av[i])
					blue_state = 1;
				if (red_av[i])
					red_state = 1;
				if (green_av[i])
					green_state = 1;
				if (yellow_av[i])
					yellow_state = 1;

				snprintf(msg, 250, "POINT:%d, AVAILABILITY: Blue %d Red %d Yellow %d Green %d\n", i + 1, blue_state, red_state, yellow_state, green_state);
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
			FOX_HudMessage(INDEXENT(1), h, msg);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_chat")) {
			changeBotSetting("bot_chat", &bot_chat, arg1, 0, 1000, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_xmas")) {
			if (FStrEq(arg1, "on")) {
				bot_xmas = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot xmas is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				bot_xmas = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot xmas is OFF\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_allow_moods")) {
			changeBotSetting("bot_allow_moods", &bot_allow_moods, arg1, 0, 1, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_allow_humour")) {
			changeBotSetting("bot_allow_humour", &bot_allow_humour, arg1, 0, 1, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_use_grenades")) {
			changeBotSetting("bot_use_grenades", &bot_use_grenades, arg1, 0, 2, SETTING_SOURCE_CLIENT_COMMAND);

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_can_use_teleporter")) // bot_can_use_teleporter - by yuraj
		{
			if (FStrEq(arg1, "on")) {
				bot_can_use_teleporter = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_use_teleporter is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				bot_can_use_teleporter = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_use_teleporter is OFF\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "bot_can_build_teleporter")) // bot_can_build_teleporter - by yuraj
		{
			if (FStrEq(arg1, "on")) {
				bot_can_build_teleporter = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_build_teleporter is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				bot_can_build_teleporter = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot_can_build_teleporter is OFF\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "locate_waypoint")) {
			if (FStrEq(arg1, "on")) {
				g_find_waypoint = true;
				g_path_waypoint = true;
				g_waypoint_on = true; // turn this on just in case
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "locate waypoint is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_find_waypoint = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "locate waypoint is OFF\n");
			}
			else {
				g_find_waypoint = true;
				g_waypoint_on = true; // turn this on just in case

				int l_waypoint = atoi(arg1);
				if (l_waypoint <= num_waypoints) {
					g_find_wp = l_waypoint;
					char s[255];
					std::sprintf(s, "locate waypoint %d\n", l_waypoint);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, s);
				}
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "pathwaypoint_connect")) {
			if (FStrEq(arg1, "on")) {
				g_path_waypoint = true;
				g_waypoint_on = true; // turn this on just in case
				g_path_connect = true;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint auto connect is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_path_connect = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint auto connect is OFF\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "pathrun_oneway")) {
			if (FStrEq(arg1, "on")) {
				wpt1 = -1;
				wpt2 = -1;
				g_path_waypoint = true;
				g_waypoint_on = true; // turn this on just in case
				g_path_connect = false;
				g_path_oneway = true;
				g_path_twoway = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun oneway ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_path_oneway = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun oneway OFF\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "pathrun_twoway")) {
			if (FStrEq(arg1, "on")) {
				wpt1 = -1;
				wpt2 = -1;
				g_path_waypoint = true;
				g_waypoint_on = true; // turn this on just in case
				g_path_connect = false;
				g_path_oneway = false;
				g_path_twoway = true;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun twoway ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_path_twoway = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathrun twoway OFF\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "pathwaypoint")) {
			if (FStrEq(arg1, "on")) {
				g_path_waypoint = true;
				g_waypoint_on = true; // turn this on just in case

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_path_waypoint = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypoint is OFF\n");
			}
			else if (FStrEq(arg1, "create1") && g_path_waypoint) {
				WaypointCreatePath(pEntity, 1);
			}
			else if (FStrEq(arg1, "create2") && g_path_waypoint) {
				WaypointCreatePath(pEntity, 2);
			}
			else if (FStrEq(arg1, "remove1") && g_path_waypoint) {
				WaypointRemovePath(pEntity, 1);
			}
			else if (FStrEq(arg1, "remove2") && g_path_waypoint) {
				WaypointRemovePath(pEntity, 2);
			}

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "area")) {
			if (FStrEq(arg1, "on")) {
				g_area_def = true;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				g_area_def = false;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is OFF\n");
			}
			else if (FStrEq(arg1, "add")) {
				g_area_def = true;
				AreaDefCreate(pEntity);
			}
			else if (FStrEq(arg1, "delete") && g_area_def) {
				g_area_def = true;
				AreaDefDelete(pEntity);
			}
			else if (FStrEq(arg1, "save") && g_area_def) {
				AreaDefSave();

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "areas saved\n");
			}
			else if (FStrEq(arg1, "load")) {
				if (AreaDefLoad(pEntity))
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "areas loaded\n");
			}
			else if (FStrEq(arg1, "info")) {
				AreaDefPrintInfo(pEntity);
			}
			else if (FStrEq(arg1, "name") && g_area_def) {
				int i = AreaInsideClosest(pEntity);
				if (i != -1 && std::strlen(arg2) < 64) {
					std::strcpy(areas[i].namea, arg2);
					std::strcpy(areas[i].nameb, arg2);
					std::strcpy(areas[i].namec, arg2);
					std::strcpy(areas[i].named, arg2);
				}
				AreaDefPrintInfo(pEntity);
			}
			else if (FStrEq(arg1, "name1") && g_area_def) {
				int i = AreaInsideClosest(pEntity);
				if (i != -1 && std::strlen(arg2) < 64) {
					std::strcpy(areas[i].namea, arg2);
				}
				AreaDefPrintInfo(pEntity);
			}
			else if (FStrEq(arg1, "name2") && g_area_def) {
				int i = AreaInsideClosest(pEntity);
				if (i != -1 && std::strlen(arg2) < 64) {
					std::strcpy(areas[i].nameb, arg2);
				}
				AreaDefPrintInfo(pEntity);
			}
			else if (FStrEq(arg1, "name3") && g_area_def) {
				int i = AreaInsideClosest(pEntity);
				if (i != -1 && std::strlen(arg2) < 64) {
					std::strcpy(areas[i].namec, arg2);
				}
				AreaDefPrintInfo(pEntity);
			}
			else if (FStrEq(arg1, "name4") && g_area_def) {
				int i = AreaInsideClosest(pEntity);
				if (i != -1 && std::strlen(arg2) < 64) {
					std::strcpy(areas[i].named, arg2);
				}
				AreaDefPrintInfo(pEntity);
			}
			else if (FStrEq(arg1, "autobuild1")) {
				g_area_def = true;

				AreaAutoBuild1();

				// ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is ON\n");
			}
			else if (FStrEq(arg1, "merge")) {
				g_area_def = true;

				AreaAutoMerge();

				// ClientPrint(pEntity, HUD_PRINTNOTIFY, "area is ON\n");
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "menuselect") && g_menu_state != MENU_NONE) {
			if (g_menu_state == MENU_1) // main menu...
			{
				if (FStrEq(arg1, "1")) // team specific...
				{
					g_menu_state = MENU_2; // display team specific menu...
					UTIL_ShowMenu(pEntity, 0x1F, -1, false, show_menu_2);
					if (mr_meta)
						RETURN_META(MRES_SUPERCEDE);
					return;
				}
				if (FStrEq(arg1, "2")) // display locations menu
				{
					g_menu_state = MENU_3;
					UTIL_ShowMenu(pEntity, 0x07, -1, false, show_menu_3);

					if (mr_meta)
						RETURN_META(MRES_SUPERCEDE);
					return;
				}
				if (FStrEq(arg1, "3")) // display items menu
				{
					g_menu_state = MENU_4;
					UTIL_ShowMenu(pEntity, 0x0F, -1, false, show_menu_4);

					if (mr_meta)
						RETURN_META(MRES_SUPERCEDE);
					return;
				}
				if (FStrEq(arg1, "4")) // display actions menu 1
				{
					g_menu_state = MENU_5;
					UTIL_ShowMenu(pEntity, 0xFF, -1, false, show_menu_5);

					if (mr_meta)
						RETURN_META(MRES_SUPERCEDE);
					return;
				}
				if (FStrEq(arg1, "5")) // display actions menu 2
				{
					g_menu_state = MENU_6;
					UTIL_ShowMenu(pEntity, 0xFF, -1, false, show_menu_6);

					if (mr_meta)
						RETURN_META(MRES_SUPERCEDE);
					return;
				}
				if (FStrEq(arg1, "6")) // control points
				{
					g_menu_state = MENU_7;
					UTIL_ShowMenu(pEntity, 0x1FF, -1, false, show_menu_7);
					if (mr_meta)
						RETURN_META(MRES_SUPERCEDE);
					return;
				}
			}
			else if (g_menu_state == MENU_2) // team specific tags
			{
				int team = atoi(arg1);
				team--; // make 0 - 3
				if (waypoints[g_menu_waypoint].flags & W_FL_TEAM_SPECIFIC && (team >= 0 && team <= 3)) {
					waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM;
					waypoints[g_menu_waypoint].flags &= ~W_FL_TEAM_SPECIFIC; // off
				}
				else {
					if (team >= 0 && team <= 3) {
						waypoints[g_menu_waypoint].flags |= team;
						waypoints[g_menu_waypoint].flags |= W_FL_TEAM_SPECIFIC; // on
					}
				}
			}
			else if (g_menu_state == MENU_3) // location tags
			{
				if (mod_id == TFC_DLL) {
					if (FStrEq(arg1, "1")) // flag location
					{
						if (waypoints[g_menu_waypoint].flags & W_FL_TFC_FLAG)
							waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_FLAG; // off
						else
							waypoints[g_menu_waypoint].flags |= W_FL_TFC_FLAG; // on
					}
					else if (FStrEq(arg1, "2"))                            // flag goal
					{
						if (waypoints[g_menu_waypoint].flags & W_FL_TFC_FLAG_GOAL)
							waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_FLAG_GOAL; // off
						else
							waypoints[g_menu_waypoint].flags |= W_FL_TFC_FLAG_GOAL; // on
					}
				}
			}
			else if (g_menu_state == MENU_4) // item tags
			{
				if (FStrEq(arg1, "1")) // set health
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_HEALTH)
						waypoints[g_menu_waypoint].flags &= ~W_FL_HEALTH; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_HEALTH; // on
				}
				else if (FStrEq(arg1, "2"))                          // set armor
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_ARMOR)
						waypoints[g_menu_waypoint].flags &= ~W_FL_ARMOR; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_ARMOR; // on
				}
				else if (FStrEq(arg1, "3"))                         // set ammo
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_AMMO)
						waypoints[g_menu_waypoint].flags &= ~W_FL_AMMO; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_AMMO; // on
				}
			}
			else if (g_menu_state == MENU_5) // action tags
			{
				if (FStrEq(arg1, "1")) // set soldier/hw defender
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_PL_DEFEND)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_PL_DEFEND; // off
					else {
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_PL_DEFEND; // on
						WaypointAddAiming(pEntity);
					}
				}
				else if (FStrEq(arg1, "2")) // set demo defender
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_PIPETRAP)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_PIPETRAP; // off
					else {
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_PIPETRAP; // on
						WaypointAddAiming(pEntity);
					}
				}
				else if (FStrEq(arg1, "3")) // sniper spot
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_SNIPER)
						waypoints[g_menu_waypoint].flags &= ~W_FL_SNIPER; // off
					else {
						waypoints[g_menu_waypoint].flags |= W_FL_SNIPER; // on
						// set the aiming waypoint...
						WaypointAddAiming(pEntity);
					}
				}
				else if (FStrEq(arg1, "4")) // set sentry
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_SENTRY)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_SENTRY; // off
					else {
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_SENTRY; // on
						WaypointAddAiming(pEntity);
					}
				}
				else if (FStrEq(arg1, "5")) // set 180 sentry
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_SENTRY) {
						if (waypoints[g_menu_waypoint].flags & W_FL_TFC_SENTRY_180)
							waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_SENTRY_180; // off
						else {
							waypoints[g_menu_waypoint].flags |= W_FL_TFC_SENTRY_180; // on
						}
					}
					else {
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "This MUST be placed on an existing sentry waypoint\n");
					}
				}
				else if (FStrEq(arg1, "6")) // set to teleport entrance
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_TELEPORTER_ENTRANCE)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_TELEPORTER_ENTRANCE; // off
					else {
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_TELEPORTER_ENTRANCE; // on
						WaypointAddAiming(pEntity);
					}
				}
				else if (FStrEq(arg1, "7")) // set to teleport exit
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_TELEPORTER_EXIT)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_TELEPORTER_EXIT; // off
					else {
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_TELEPORTER_EXIT; // on
						WaypointAddAiming(pEntity);
					}
				}
			}
			else if (g_menu_state == MENU_6) // action tags p2
			{
				if (FStrEq(arg1, "1")) // RJ CJ
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_JUMP)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_JUMP; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_JUMP; // on
				}
				else if (FStrEq(arg1, "2"))                            // set jump
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_JUMP)
						waypoints[g_menu_waypoint].flags &= ~W_FL_JUMP; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_JUMP; // on
				}
				else if (FStrEq(arg1, "3"))                        // wait for lift...
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_LIFT)
						waypoints[g_menu_waypoint].flags &= ~W_FL_LIFT; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_LIFT; // on
				}
				else if (FStrEq(arg1, "4"))                        // walk waypoint
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_WALK)
						waypoints[g_menu_waypoint].flags &= ~W_FL_WALK; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_WALK; // on
				}
				else if (FStrEq(arg1, "5"))                        // set detpack(blow passage open only)
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_DETPACK_CLEAR)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_DETPACK_CLEAR; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_DETPACK_CLEAR; // on
				}
				else if (FStrEq(arg1, "6"))                                     // set detpack(blow passage closed only)
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_TFC_DETPACK_SEAL)
						waypoints[g_menu_waypoint].flags &= ~W_FL_TFC_DETPACK_SEAL; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_TFC_DETPACK_SEAL; // on
				}
				else if (FStrEq(arg1, "7"))                                    // check if path is obstructed
				{
					if (waypoints[g_menu_waypoint].flags & W_FL_PATHCHECK)
						waypoints[g_menu_waypoint].flags &= ~W_FL_PATHCHECK; // off
					else
						waypoints[g_menu_waypoint].flags |= W_FL_PATHCHECK; // on
				}
			}
			else if (g_menu_state == MENU_7) // control point tags
			{
				if (FStrEq(arg1, "1")) // point1
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT1)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT1; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT1; // on
				}
				else if (FStrEq(arg1, "2"))                                 // point2
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT2)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT2; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT2; // on
				}
				else if (FStrEq(arg1, "3"))                                 // point3
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT3)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT3; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT3; // on
				}
				else if (FStrEq(arg1, "4"))                                 // point4
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT4)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT4; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT4; // on
				}
				else if (FStrEq(arg1, "5"))                                 // point5
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT5)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT5; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT5; // on
				}
				else if (FStrEq(arg1, "6"))                                 // point6
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT6)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT6; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT6; // on
				}
				else if (FStrEq(arg1, "7"))                                 // point7
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT7)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT7; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT7; // on
				}
				else if (FStrEq(arg1, "8"))                                 // point8
				{
					if (waypoints[g_menu_waypoint].script_flags & S_FL_POINT8)
						waypoints[g_menu_waypoint].script_flags &= ~S_FL_POINT8; // off
					else
						waypoints[g_menu_waypoint].script_flags |= S_FL_POINT8; // on
				}
				g_menu_state = MENU_NONE;
			}
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		else if (FStrEq(pcmd, "search")) {
			edict_t* pent = nullptr;
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "searching...\n");
			while ((pent = FIND_ENTITY_IN_SPHERE(pent, pEntity->v.origin, 200.0f)) != nullptr && !FNullEnt(pent)) {
				char str[80];
				std::sprintf(str, "Found %s at %5.2f %5.2f %5.2f modelindex- %d t %s tn %s\n", STRING(pent->v.classname), static_cast<double>(pent->v.origin.x), static_cast<double>(pent->v.origin.y), static_cast<double>(pent->v.origin.z),
					pent->v.modelindex, STRING(pent->v.target), STRING(pent->v.targetname));
				ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

				FILE* fp = UTIL_OpenFoxbotLog();
				if (fp != nullptr) {
					std::fprintf(fp, "ClientCommmand: search %s\n", str);
					// std::fwrite(&pent->v, sizeof(pent->v), 1, fp);
					std::fclose(fp);
				}
				UTIL_SavePent(pent);
			}

			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		// DREVIL CVARS
		else if (FStrEq(pcmd, "defensive_chatter")) {
			if (FStrEq(arg1, "on")) {
				defensive_chatter = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				defensive_chatter = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is OFF\n");
			}
			else if (FStrEq(arg1, "1")) {
				defensive_chatter = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is ON\n");
			}
			else if (FStrEq(arg1, "0")) {
				defensive_chatter = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Defensive Chatter is OFF\n");
			}
			return;
		}
		else if (FStrEq(pcmd, "offensive_chatter")) {
			if (FStrEq(arg1, "on")) {
				offensive_chatter = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is ON\n");
			}
			else if (FStrEq(arg1, "off")) {
				offensive_chatter = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is OFF\n");
			}
			else if (FStrEq(arg1, "1")) {
				offensive_chatter = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is ON\n");
			}
			else if (FStrEq(arg1, "0")) {
				offensive_chatter = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "Offensive Chatter is OFF\n");
			}
			return;
		}
	}

	if (!mr_meta)
		(*other_gFunctionTable.pfnClientCommand)(pEntity);
	else {
		SET_META_RESULT(MRES_HANDLED);
	}
}

void StartFrame() { // v7 last frame timing
	last_frame_time = last_frame_time_prev;
	last_frame_time_prev = gpGlobals->time; // Reset pipeCheck frame counter.
	if (pipeCheckFrame < 0)
		pipeCheckFrame = 20;
	if (gpGlobals->deathmatch) {
		edict_t* pPlayer;
		static float check_server_cmd;
		check_server_cmd = gpGlobals->time;
		static int i, index, player_index, bot_index;
		static float previous_time = -1.0f;
		static float client_update_time = 0.0f;
		clientdata_s cd;
		int count;
		// if a new map has started then (MUST BE FIRST IN StartFrame)...
		if (std::strcmp(STRING(gpGlobals->mapname), prevmapname) != 0) {
			first_player = nullptr;
			display_bot_vars = true;
			display_start_time = gpGlobals->time + 10.0f;
			need_to_open_cfg = true;
			// check if mapname_bot.cfg file exists...
			if (gpGlobals->time + 0.1f < previous_time) {
				char mapname[64];
				char filename[256];
				std::strcpy(mapname, STRING(gpGlobals->mapname));
				std::strcat(mapname, "_bot.cfg");
				bot_cfg_fp = nullptr;
				UTIL_BuildFileName(filename, 255, "configs", mapname);
				bot_cfg_fp = std::fopen(filename, "r");
				if (bot_cfg_fp == nullptr) {
					UTIL_BuildFileName(filename, 255, "default.cfg", nullptr);
					bot_cfg_fp = std::fopen(filename, "r");
				}
				if (bot_cfg_fp != nullptr) {
					for (index = 0; index < 32; index++) {
						bots[index].is_used = false;
						bots[index].respawn_state = 0;
						bots[index].f_kick_time = 0.0f;
					}
					std::fclose(bot_cfg_fp);
					bot_cfg_fp = nullptr;
				}
				else {
					count = 0; // mark the bots as needing to be respawned...
					for (index = 0; index < 32; index++) {
						if (count >= prev_num_bots) {
							bots[index].is_used = false;
							bots[index].respawn_state = 0;
							bots[index].f_kick_time = 0.0f;
						}
						if (bots[index].is_used) // is this slot used?
						{
							bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
							count++;
						} // check for any bots that were very recently kicked...
						if (bots[index].f_kick_time + 5.0f > previous_time) {
							bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
							count++;
						}
						else
							bots[index].f_kick_time = 0.0f; // reset to prevent false spawns later
					}                                    // set the respawn time
					if (IS_DEDICATED_SERVER())
						respawn_time = gpGlobals->time + 10.0f;
					else
						respawn_time = gpGlobals->time + 25.0f; //Reduce Listenserver crash? [APG]RoboCop[CL]
				}
			} // start updating client data again
			client_update_time = gpGlobals->time + 10.0f;
			bot_check_time = gpGlobals->time + 30.0f;
		} // end of config map check stuff.
		if (client_update_time <= gpGlobals->time) {
			client_update_time = gpGlobals->time + 1.0f;
			for (i = 0; i < 32; i++) {
				if (bots[i].is_used) {
					bzero(&cd, sizeof cd);
					MDLL_UpdateClientData(bots[i].pEdict, 1, &cd); // see if a weapon was dropped...
					if (bots[i].bot_weapons != cd.weapons) //duplicate expression? [APG]RoboCop[CL]
						bots[i].bot_weapons = cd.weapons;
				}
			}
		}
		count = 0;
		UpdateFlagCarrierList(); // need to do this once per frame
		for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++) {
			// if this bot is active, and the bot is not respawning
			if (bots[bot_index].is_used && bots[bot_index].respawn_state == RESPAWN_IDLE) {
				BotThink(&bots[bot_index]);
				count++;
			}
		}
		if (count > num_bots)
			num_bots = count;
		for (player_index = 0; player_index <= gpGlobals->maxClients; player_index++)
		{
			pPlayer = INDEXENT(player_index);

			if (pPlayer && !pPlayer->free) {
				if ((g_area_def || g_waypoint_on) && FBitSet(pPlayer->v.flags, FL_CLIENT) && !FBitSet(pPlayer->v.flags, FL_FAKECLIENT)) {
					WaypointThink(pPlayer);
				}
			}
		} // are we currently respawning bots and is it time to spawn one yet?
		if (respawn_time > 1.0f && respawn_time <= gpGlobals->time) {
			int index1 = 0; // Not wanted? [APG]RoboCop[CL]
			// find bot needing to be respawned...
			while (index1 < 32 && bots[index1].respawn_state != RESPAWN_NEED_TO_RESPAWN)
				index1++;
			if (index1 < 32) {
				bots[index1].respawn_state = RESPAWN_IS_RESPAWNING;
				bots[index1].is_used = false; // free up this slot
				// respawn 1 bot then wait a while(otherwise engine crashes)
				if (mod_id != TFC_DLL) {
					char c_skill[2];
					std::sprintf(c_skill, "%d", bots[index1].bot_skill);
					BotCreate(nullptr, bots[index1].skin, bots[index1].name, c_skill, nullptr);
				}
				else {
					char c_skill[2];
					char c_team[2];
					char c_class[3];
					std::sprintf(c_skill, "%d", bots[index1].bot_skill);
					std::sprintf(c_team, "%d", bots[index1].bot_team);
					std::sprintf(c_class, "%d", bots[index1].bot_class);
					if (mod_id == TFC_DLL)
						BotCreate(nullptr, nullptr, nullptr, bots[index1].name, c_skill);
					else
						BotCreate(nullptr, c_team, c_class, bots[index1].name, c_skill);
				}
				respawn_time = gpGlobals->time + 3.0f; // set next respawn time
				bot_check_time = gpGlobals->time + 6.0f;
			}
			else
				respawn_time = 2.0f;
		}
		if (g_GameRules) {
			char msg[256];
			char msg2[512];
			if (need_to_open_cfg) // have we opened foxbot.cfg file yet?
			{
				char filename[256];
				char mapname[64];
				need_to_open_cfg = false; // only do this once!!!
				cfg_file = 1;
				display_bot_vars = true;
				display_start_time = gpGlobals->time + 10; // check if mapname_bot.cfg file exists...
				std::strcpy(mapname, STRING(gpGlobals->mapname));
				std::strcat(mapname, "_bot.cfg");
				bot_cfg_fp = nullptr;
				UTIL_BuildFileName(filename, 255, "foxbot.cfg", nullptr);
				bot_cfg_fp = std::fopen(filename, "r");
				if (bot_cfg_fp == nullptr) {
					if (IS_DEDICATED_SERVER())
						std::printf("foxbot.cfg file not found\n");
					else
						ALERT(at_console, "foxbot.cfg file not found\n");
				}
				else {
					std::sprintf(msg2, "Executing %s\n", filename);
					if (IS_DEDICATED_SERVER())
						std::printf("%s", msg);
					else
						ALERT(at_console, msg);
				}
				if (IS_DEDICATED_SERVER())
					bot_cfg_pause_time = gpGlobals->time + 5.0f;
				else //Required for Listenservers to exec .cfg [APG]RoboCop[CL]
					bot_cfg_pause_time = gpGlobals->time + 10.0f; // was 20
			}
			if (need_to_open_cfg2) // have we opened foxbot.cfg file yet?
			{
				bot_cfg_pause_time = gpGlobals->time + 5.0f;
				char filename[256];
				char mapname[64];
				need_to_open_cfg2 = false; // only do this once!!!
				cfg_file = 2;
				std::strcpy(mapname, STRING(gpGlobals->mapname));
				std::strcat(mapname, "_bot.cfg");
				bot_cfg_fp = nullptr;
				UTIL_BuildFileName(filename, 255, "configs", mapname);
				bot_cfg_fp = std::fopen(filename, "r");
				if (bot_cfg_fp != nullptr) {
					std::sprintf(msg2, "\nExecuting %s\n", filename);
					if (IS_DEDICATED_SERVER())
						std::printf("%s", msg);
					else
						ALERT(at_console, msg);
				}
				else { // first say map config not found
					std::sprintf(msg2, "\n%s not found\n", filename);
					if (IS_DEDICATED_SERVER())
						std::printf("%s", msg);
					else
						ALERT(at_console, msg);
					bot_cfg_fp = nullptr;
					UTIL_BuildFileName(filename, 255, "default.cfg", nullptr);
					bot_cfg_fp = std::fopen(filename, "r");
					if (bot_cfg_fp == nullptr) {
						if (IS_DEDICATED_SERVER())
							std::printf("\ndefault.cfg file not found\n");
						else
							ALERT(at_console, "\ndefault.cfg file not found\n");
					}
					else {
						std::sprintf(msg2, "\nExecuting %s\n", filename);
						if (IS_DEDICATED_SERVER())
							std::printf("%s", msg);
						else
							ALERT(at_console, msg);
					}
				}
			} // end need config
			if (!IS_DEDICATED_SERVER() && !spawn_time_reset) {
				if (first_player != nullptr) {
					if (IsAlive(first_player)) {
						spawn_time_reset = true;
						if (respawn_time >= 1.0f)
							respawn_time = std::min(respawn_time, gpGlobals->time + 1.0f);
						if (bot_cfg_pause_time >= 1.0f)
							bot_cfg_pause_time = std::min(bot_cfg_pause_time, gpGlobals->time + 1.0f);
					}
				}
			}
			if (bot_cfg_pause_time >= 1.0f && bot_cfg_pause_time <= gpGlobals->time) {
				// process bot.cfg file options...
				ProcessBotCfgFile();
				display_start_time = 0;
			}
			else if (bot_cfg_fp == nullptr && display_bot_vars && display_start_time <= gpGlobals->time) {
				DisplayBotInfo();
				display_bot_vars = false;
			}
		} // if time to check for server commands then do so...
		if (check_server_cmd <= gpGlobals->time && IS_DEDICATED_SERVER()) {
			check_server_cmd = gpGlobals->time + 1.0f;
			auto cvar_bot = const_cast<char*>(CVAR_GET_STRING("bot"));
			if (cvar_bot && cvar_bot[0]) {
				char cmd_line[80];
				char* cmd, * arg1, * arg2, * arg3, * arg4;
				std::strcpy(cmd_line, cvar_bot);
				index = 0;
				cmd = cmd_line;
				arg1 = arg2 = arg3 = arg4 = nullptr; // skip to blank or end of string...
				while (cmd_line[index] != ' ' && cmd_line[index] != 0)
					index++;
				if (cmd_line[index] == ' ') {
					cmd_line[index++] = 0;
					arg1 = &cmd_line[index]; // skip to blank or end of string...
					while (cmd_line[index] != ' ' && cmd_line[index] != 0)
						index++;
					if (cmd_line[index] == ' ') {
						cmd_line[index++] = 0;
						arg2 = &cmd_line[index]; // skip to blank or end of string...
						while (cmd_line[index] != ' ' && cmd_line[index] != 0)
							index++;
						if (cmd_line[index] == ' ') {
							cmd_line[index++] = 0;
							arg3 = &cmd_line[index]; // skip to blank or end of string...
							while (cmd_line[index] != ' ' && cmd_line[index] != 0)
								index++;
							if (cmd_line[index] == ' ') {
								cmd_line[index++] = 0;
								arg4 = &cmd_line[index];
							}
						}
					}
				}
				if (std::strcmp(cmd, "addbot") == 0) {
					BotCreate(nullptr, arg1, arg2, arg3, arg4);
					bot_check_time = gpGlobals->time + 5.0f;
				}
				else if (std::strcmp(cmd, "min_bots") == 0) {
					changeBotSetting("min_bots", &min_bots, arg1, -1, 31, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "max_bots") == 0) {
					changeBotSetting("max_bots", &max_bots, arg1, -1, MAX_BOTS, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "bot_total_varies") == 0) {
					changeBotSetting("bot_total_varies", &bot_total_varies, arg1, 0, 3, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "bot_info") == 0) {
					DisplayBotInfo();
				}
				else if (std::strcmp(cmd, "bot_team_balance") == 0) {
					if (arg1 != nullptr) {
						if (*arg1 != 0) {
							int temp;
							temp = atoi(arg1);
							if (temp)
								bot_team_balance = true;
							else
								bot_team_balance = false;
						}
					}
					if (bot_team_balance)
						std::printf("bot_team_balance (1) On\n");
					else
						std::printf("bot_team_balance (0) Off\n");
				}
				else if (std::strcmp(cmd, "bot_bot_balance") == 0) {
					if (arg1 != nullptr) {
						if (*arg1 != 0) {
							int temp;
							temp = atoi(arg1);
							if (temp)
								bot_bot_balance = true;
							else
								bot_bot_balance = false;
						}
					}
					if (bot_bot_balance)
						std::printf("bot_bot_balance (1) On\n");
					else
						std::printf("bot_bot_balance (0) Off\n");
				}
				else if (std::strcmp(cmd, "kickall") == 0) { // kick all bots off of the server after time/frag limit...
					kickBots(MAX_BOTS, -1);
				}
				else if (std::strcmp(cmd, "kickteam") == 0 || std::strcmp(cmd, "foxbot_kickteam") == 0) {
					if (!arg1)
						std::printf("Proper syntax, e.g. bot \"kickteam 1\"\n");
					else {
						int whichTeam;
						if (std::strcmp(arg1, "blue") == 0)
							whichTeam = 1;
						else if (std::strcmp(arg1, "red") == 0)
							whichTeam = 2;
						else if (std::strcmp(arg1, "yellow") == 0)
							whichTeam = 3;
						else if (std::strcmp(arg1, "green") == 0)
							whichTeam = 4;
						else
							whichTeam = atoi(arg1);
						kickBots(MAX_BOTS, whichTeam);
					}
				}
				else if (std::strcmp(cmd, "bot_chat") == 0 && arg1 != nullptr) {
					changeBotSetting("bot_chat", &bot_chat, arg1, 0, 1000, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "botskill_lower") == 0) {
					changeBotSetting("botskill_lower", &botskill_lower, arg1, 1, 5, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "botskill_upper") == 0) {
					changeBotSetting("botskill_upper", &botskill_upper, arg1, 1, 5, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "bot_skill_1_aim") == 0) {
					changeBotSetting("bot_skill_1_aim", &bot_skill_1_aim, arg1, 0, 200, SETTING_SOURCE_SERVER_COMMAND);
					BotUpdateSkillInaccuracy();
					return;
				}
				else if (std::strcmp(cmd, "bot_aim_per_skill") == 0) {
					changeBotSetting("bot_aim_per_skill", &bot_aim_per_skill, arg1, 5, 50, SETTING_SOURCE_SERVER_COMMAND);
					BotUpdateSkillInaccuracy();
					return;
				}
				else if (std::strcmp(cmd, "bot_can_build_teleporter") == 0 && arg1 != nullptr) {
					if (std::strcmp(arg1, "on") == 0) {
						bot_can_build_teleporter = true;
						std::printf("bot_can_build_teleporter is ON\n");
					}
					else if (std::strcmp(arg1, "off") == 0) {
						bot_can_build_teleporter = false;
						std::printf("bot_can_build_teleporter is OFF\n");
					}
				}
				else if (std::strcmp(cmd, "bot_can_use_teleporter") == 0 && arg1 != nullptr) {
					if (std::strcmp(arg1, "on") == 0) {
						bot_can_use_teleporter = true;
						std::printf("bot_can_use_teleporter is ON\n");
					}
					else if (std::strcmp(arg1, "off") == 0) {
						bot_can_use_teleporter = false;
						std::printf("bot_can_use_teleporter is OFF\n");
					}
				}
				else if (std::strcmp(cmd, "bot_xmas") == 0 && arg1 != nullptr) {
					if (std::strcmp(arg1, "on") == 0) {
						bot_xmas = true;
						std::printf("bot xmas is ON\n");
					}
					else if (std::strcmp(arg1, "off") == 0) {
						bot_xmas = false;
						std::printf("bot xmas is OFF\n");
					}
				}
				else if (std::strcmp(cmd, "bot_allow_moods") == 0) {
					changeBotSetting("bot_allow_moods", &bot_allow_moods, arg1, 0, 1, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "bot_allow_humour") == 0) {
					changeBotSetting("bot_allow_humour", &bot_allow_humour, arg1, 0, 1, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "bot_use_grenades") == 0) {
					changeBotSetting("bot_use_grenades", &bot_use_grenades, arg1, 0, 2, SETTING_SOURCE_SERVER_COMMAND);
				}
				else if (std::strcmp(cmd, "dump") == 0) {
					edict_t* pent = nullptr;
					while ((pent = FIND_ENTITY_IN_SPHERE(pent, Vector(0, 0, 0), 8192)) != nullptr && !FNullEnt(pent)) {
						UTIL_SavePent(pent);
					}
				} // dedicated server input
			}    // moved this line down one
			CVAR_SET_STRING("bot", "");
		} // check if time to see if a bot needs to be created...
		if (bot_check_time < gpGlobals->time) {
			bot_check_time = gpGlobals->time + bot_create_interval; // min/max checking and team balance checking..
			playersPerTeam[0] = 0;
			playersPerTeam[1] = 0;
			playersPerTeam[2] = 0;
			playersPerTeam[3] = 0;
			if (max_bots > MAX_BOTS)
				max_bots = MAX_BOTS;
			if (max_bots > gpGlobals->maxClients)
				max_bots = gpGlobals->maxClients;
			if (max_bots < -1)
				max_bots = -1;
			if (min_bots > max_bots)
				min_bots = max_bots;
			if (min_bots < -1)
				min_bots = -1;
			if (min_bots == 0)
				min_bots = -1; // count the number of players, and players per team
			int count1 = 0;   // Not wanted? [APG]RoboCop[CL]
			{
				char cl_name[128];
				for (i = 1; i <= gpGlobals->maxClients; i++) { //TODO: Refactor Loop [APG]RoboCop[CL]
					if (INDEXENT(i) == nullptr)
						continue;
					cl_name[0] = '\0';
					char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(INDEXENT(i));
					std::strcpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
					// is this a valid connected player?
					if (cl_name[0] != '\0') {
						count1++;
						int team = INDEXENT(i)->v.team - 1;
						if (team > -1 && team < 4)
							++playersPerTeam[team];
					}
				}
			} // sort out is_team, just in case the map ents are fucked
			bool active_team_found = false;
			for (i = 0; i < 4; i++) // look for an active team
			{
				if (is_team[i])
					active_team_found = true;
			}
			if (!active_team_found) // assume there are at least two teams
			{
				is_team[0] = true;
				is_team[1] = true;
				is_team[2] = false;
				is_team[3] = false;
			}
			if (is_team[2] == false && is_team[3] == true)
				is_team[3] = false; // check if the teams need balancing
			TeamBalanceCheck();    // random simulation of clients joining/leaving
			if (bot_total_varies)
				varyBotTotal(); // if there are currently less than the maximum number of players
			// then add another bot using the default skill level...
			if ((count1 < interested_bots || bot_total_varies == 0) && count1 < max_bots && max_bots != -1) {
				BotCreate(nullptr, nullptr, nullptr, nullptr, nullptr);
			} // do bot max_bot kick here... if there are currently more than the minimum number of bots running then kick one of the bots off the server...
			if (((count1 > interested_bots && bot_total_varies) || count1 > max_bots) && max_bots != -1) {
				// count the number of bots present
				int bot_count = 0;
				for (i = 0; i <= 31; i++) {
					if (bots[i].is_used)
						++bot_count;
				}
				if (bot_count > min_bots) {
					if (bot_total_varies)
						kickRandomBot();
					else
						kickBots(1, -1);
				}
			}
		}
		else { // make sure the delay is not insanely long
			if (bot_check_time > gpGlobals->time + 100.0f)
				bot_check_time = gpGlobals->time + 10.0f;
		}
		previous_time = gpGlobals->time;
	} // this is where the behaviour config is interpreted... i.e. new lev, load behaviour, and parse it
	if (std::strcmp(STRING(gpGlobals->mapname), prevmapname) != 0) {
		edict_t* pent = nullptr;
		while ((pent = FIND_ENTITY_IN_SPHERE(pent, Vector(0, 0, 0), 8000)) != nullptr && !FNullEnt(pent)) {
			if (static_cast<int>(pent->v.absmin.x) == -1 && static_cast<int>(pent->v.absmin.y) == -1 && static_cast<int>(pent->v.absmin.z) == -1) {
				if (static_cast<int>(pent->v.absmin.x) == 1 && static_cast<int>(pent->v.absmin.y) == 1 && static_cast<int>(pent->v.absmin.z) == 1) {
					global::fp = UTIL_OpenFoxbotLog();
					if (global::fp != nullptr) {
						std::fprintf(global::fp, "Fixing entity current map %s last map %s\n", STRING(gpGlobals->mapname), prevmapname);
						std::fclose(global::fp);
					} // UTIL_SavePent(pent);
					if (std::strcmp(STRING(pent->v.classname), "info_tfgoal") != 0) {
						pent->v.absmin = pent->v.absmin + pent->v.mins;
						pent->v.absmax = pent->v.absmax + pent->v.maxs;
						if (mr_meta)
							MDLL_Spawn(pent);
						else
							DispatchSpawn(pent);
					}
					else { // is info goal
						int max = 0;
						int t = static_cast<int>(pent->v.mins.x);
						if (t < 0)
							t = -t;
						if (t > max)
							max = t;
						t = static_cast<int>(pent->v.mins.y);
						if (t < 0)
							t = -t;
						if (t > max)
							max = t;
						t = static_cast<int>(pent->v.mins.z);
						if (t < 0)
							t = -t;
						if (t > max)
							max = t;
						t = static_cast<int>(pent->v.maxs.x);
						if (t < 0)
							t = -t;
						if (t > max)
							max = t;
						t = static_cast<int>(pent->v.maxs.y);
						if (t < 0)
							t = -t;
						if (t > max)
							max = t;
						t = static_cast<int>(pent->v.maxs.z);
						if (t < 0)
							t = -t;
						if (t > max)
							max = t;
						pent->v.absmin = pent->v.absmin - Vector(max, max, max);
						pent->v.absmax = pent->v.absmax + Vector(max, max, max);
						pent->v.nextthink = 0;
						if (mr_meta)
							MDLL_Spawn(pent);
						else
							DispatchSpawn(pent);
					}
				}
			}
			if (std::strcmp(STRING(pent->v.classname), "func_door") == 0 && static_cast<int>(pent->v.nextthink) != -1)
				pent->v.nextthink = -1;
		}
		script_loaded = false;
		script_parsed = false;
		std::strcpy(prevmapname, STRING(gpGlobals->mapname));
		char filename[256];
		char mapname[64];
		int i; // reset known team data on map change
		for (i = 0; i < 4; i++) {
			attack[i] = false;
			defend[i] = false;
		}
		for (i = 0; i < 8; i++) {
			blue_av[i] = false;
			red_av[i] = false;
			green_av[i] = false;
			yellow_av[i] = false;
		}
		ResetBotHomeInfo();
		msg_com_struct* prev = nullptr;
		msg_com_struct* curr = nullptr;
		for (i = 0; i < MSG_MAX; i++) {
			// assuming i only goes to 64 on next line..see msg_msg[64][msg_max] was [0][i] before...may be a problem
			msg_msg[0][i] = '\0'; // clear the messages, for level changes
			msg_com[i].ifs[0] = '\0';
			// the idea behind this delete function is if the root.next isnt null, then it finds the last item in list (the one with item.next =null) and deletes it.. then repeats it all again.. if root.next etc
			while (msg_com[i].next != nullptr) {
				curr = &msg_com[i];
				while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1) {
					prev = curr;
					curr = curr->next;
				}
				if (prev != nullptr) {
					delete prev->next;
					prev->next = nullptr; // make sure it doesnt go over
				}
			}
			msg_com[i].next = nullptr; // make sure it dont crash..gr
		}                             // check if mapname_bot.cfg file exists...
		std::strcpy(mapname, STRING(gpGlobals->mapname));
		std::strcat(mapname, "_fb.cfg");
		UTIL_BuildFileName(filename, 255, "scripts", mapname);
		FILE* bfp = std::fopen(filename, "r");
		if (bfp != nullptr && mod_id == TFC_DLL) {
			script_loaded = true;
			char msg[293];
			std::sprintf(msg, "\nExecuting FoXBot TFC script file:%s\n\n", filename);
			ALERT(at_console, msg);
			int ch = fgetc(bfp);
			int i1; // Not wanted? [APG]RoboCop[CL]
			char buffer[14097];
			for (i1 = 0; i1 < 14096 && std::feof(bfp) == 0; i1++) {
				buffer[i1] = static_cast<char>(ch);
				if (buffer[i1] == '\t')
					buffer[i1] = ' ';
				ch = fgetc(bfp);
			}
			buffer[i1] = '\0';        // we've read it into buffer, now we need to check the syntax..
			int braces = 0;           // check for an even number of braces i.e. {..}
			bool commentline = false; // used to ignore comment lines
			int start = 0;            // used to check if were in a start section or not
			int havestart = 0;
			int msgsection = 0; // used to check if were in a message section section or not
			int ifsec = 0;      // used to check if were in an if statement
			char* buf = buffer;
			bool random_shit_error = false;
			for (i1 = 0; i1 < 14096 && buffer[i1] != '\0' && !random_shit_error; i1++) {
				// first off... we need to ignore comment lines!
				if (std::strncmp(buf, "//", 2) == 0) {
					commentline = true;
					i1++;
					buf = buf + 1;
				}
				if (buffer[i1] == '\n')
					commentline = false;
				if (commentline == false) {
					// need to check for section definition (on_...)
					if (std::strncmp(buf, "on_start", 8) == 0) {
						if (start > 0)
							start = 99; // check for nested start defs
						else
							start = 1;
						if (havestart > 0)
							havestart = 99; // check for more than one on_start section
						else
							havestart = 1;
						for (int j = 0; j < 8; j++) {
							i1++;
							buf = buf + 1;
						} // try and move to end of on start
					}
					if (std::strncmp(buf, "on_msg", 6) == 0) {
						if (msgsection > 0)
							msgsection = 99; // check for nested msg defs
						else
							msgsection = 1;
						for (int j = 0; j < 6; j++) {
							i1++;
							buf = buf + 1;
						} // try and move to end of on_msg // now check input var!
						if (buffer[i1] != '(')
							msgsection = 99;
						// make sure it starts wif a (
						else {
							i1++;
							buf = buf + 1;
							if (buffer[i1] == ')')
								msgsection = 99; // make sure message isnt empty
							else {              // if it isn't empty, move to end (ignore msg)
								while (buffer[i1] != ')') {
									i1++;
									buf = buf + 1;
								}
							}
						}
					} // attack
					else if (std::strncmp(buf, "blue_attack", 11) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 11; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[0] = 90;
						} // move to end
					}
					else if (std::strncmp(buf, "red_attack", 10) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 10; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[1] = 90;
						} // move to end
					}
					else if (std::strncmp(buf, "yellow_attack", 13) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 13; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[3] = 90;
						} // move to end
					}
					else if (std::strncmp(buf, "green_attack", 12) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 12; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[3] = 90;
						} // move to end
					}    // defend
					else if (std::strncmp(buf, "blue_defend", 11) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 11; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[0] = 15;
						} // move to end
					}
					else if (std::strncmp(buf, "red_defend", 10) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 10; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[1] = 15;
						} // move to end
					}
					else if (std::strncmp(buf, "yellow_defend", 13) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 13; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[2] = 15;
						} // move to end
					}
					else if (std::strncmp(buf, "green_defend", 12) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 12; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[3] = 15;
						} // move to end
					}
					else if (std::strncmp(buf, "blue_normal", 11) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 11; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[0] = 50;
						} // move to end
					}
					else if (std::strncmp(buf, "red_normal", 10) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 10; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[1] = 50;
						} // move to end
					}
					else if (std::strncmp(buf, "yellow_normal", 13) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 13; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[2] = 50;
						} // move to end
					}
					else if (std::strncmp(buf, "green_normal", 12) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 12; j++) {
							i1++;
							buf = buf + 1;
							RoleStatus[3] = 50;
						} // move to end
					}    // point<n> available_only (exclusive)
					else if (std::strncmp(buf, "blue_available_only_point", 25) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 25; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "red_available_only_point", 24) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 24; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "green_available_only_point", 26) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 26; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "yellow_available_only_point", 27) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 27; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}    // point<n> available (set to true)
					else if (std::strncmp(buf, "blue_available_point", 20) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 20; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "red_available_point", 19) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 19; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "green_available_point", 21) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 21; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "yellow_available_point", 22) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 22; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}    // point<n> not_available (set to false)
					else if (std::strncmp(buf, "blue_notavailable_point", 23) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 23; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "red_notavailable_point", 22) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 22; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "green_notavailable_point", 24) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 24; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "yellow_notavailable_point", 25) == 0) {
						// this can only be in a section
						if (start == 0 && msgsection == 0)
							random_shit_error = true;
						for (int j = 0; j < 25; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}    // is point<n> available?
					else if (std::strncmp(buf, "if_blue_point", 13) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 13; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "if_red_point", 12) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 12; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "if_green_point", 14) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 14; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "if_yellow_point", 15) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 15; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}    // is point<n> NOT available?
					else if (std::strncmp(buf, "ifn_blue_point", 14) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 14; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "ifn_red_point", 13) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 13; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "ifn_green_point", 15) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 15; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}
					else if (std::strncmp(buf, "ifn_yellow_point", 16) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						for (int j = 0; j < 16; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
							random_shit_error = true;
						else {
							i1++;
							buf = buf + 1;
						} // move to end
					}    // multipoint ifs
					else if (std::strncmp(buf, "if_blue_mpoint", 14) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						int j;
						for (j = 0; j < 14; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						// check for 8 if vals
						for (j = 0; j < 8; j++) {
							if (buffer[i1] != '1' && buffer[i1] != '0')
								random_shit_error = true;
							else {
								i1++;
								buf = buf + 1;
							} // move to end
						}
					}
					else if (std::strncmp(buf, "if_red_mpoint", 13) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						int j;
						for (j = 0; j < 13; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						// check for 8 if vals
						for (j = 0; j < 8; j++) {
							if (buffer[i1] != '1' && buffer[i1] != '0')
								random_shit_error = true;
							else {
								i1++;
								buf = buf + 1;
							} // move to end
						}
					}
					else if (std::strncmp(buf, "if_green_mpoint", 15) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						int j;
						for (j = 0; j < 15; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						// check for 8 if vals
						for (j = 0; j < 8; j++) {
							if (buffer[i1] != '1' && buffer[i1] != '0')
								random_shit_error = true;
							else {
								i1++;
								buf = buf + 1;
							} // move to end
						}
					}
					else if (std::strncmp(buf, "if_yellow_mpoint", 16) == 0) {
						// this can only be in a section
						if (msgsection == 0)
							random_shit_error = true;
						if (ifsec > 0)
							ifsec = 99; // check for nested if defs
						else
							ifsec = 1;
						int j;
						for (j = 0; j < 16; j++) {
							i1++;
							buf = buf + 1;
						} // move to end
						// check for 8 if vals
						for (j = 0; j < 8; j++) {
							if (buffer[i1] != '1' && buffer[i1] != '0')
								random_shit_error = true;
							else {
								i1++;
								buf = buf + 1;
							} // move to end
						}
					} // end of multipoint ifs
					else if (buffer[i1] != '/' && buffer[i1] != '{' && buffer[i1] != '}' && buffer[i1] != ' ' && buffer[i1] != '\n' && commentline == false && random_shit_error == false) {
						random_shit_error = true;
						ALERT(at_console, "\\/\\/\\/\\/\\/\\/\n");
						ALERT(at_console, buf);
						ALERT(at_console, "\n");
					} // do your magic lexical analysis here. first braces
					switch (buffer[i1]) {
					case '{':
						braces++;
						if (start == 1)
							start = 2;
						else if (start > 0)
							start = 99;
						if (ifsec == 1)
							ifsec = 2;
						else if (ifsec > 0)
							ifsec = 99;
						if (msgsection == 1)
							msgsection = 2;
						else if (msgsection > 0 && ifsec == 0)
							msgsection = 99;
						break;
					case '}':
						braces--;
						if (start == 2)
							start = 0;
						if (msgsection == 2 && ifsec == 0)
							msgsection = 0;
						if (ifsec == 2)
							ifsec = 0; // cancel ifs before ending message section
						break;
					default:;
					}
				}
				if (!random_shit_error)
					buf = buf + 1; // like i++ but for stringcmp stuff
			}
			bool syntax_error;
			syntax_error = false;
			if (braces != 0) {
				ALERT(at_console, "Syntax error, wrong number of braces\n");
				syntax_error = true;
			}
			if (start != 0) {
				ALERT(at_console, "Syntax error, on_start error\n");
				syntax_error = true;
			}
			if (msgsection != 0) {
				ALERT(at_console, "Syntax error, on_msg error\n");
				syntax_error = true;
			}
			if (ifsec != 0) {
				ALERT(at_console, "Syntax error, if error\n");
				syntax_error = true;
			}
			if (havestart > 1) {
				ALERT(at_console, "Syntax error, more than 1 on_start\n");
				syntax_error = true;
			}
			if (random_shit_error) {
				ALERT(at_console, "Syntax error, unrecognised command\n");
				syntax_error = true;

				FILE* fp = UTIL_OpenFoxbotLog();
				if (fp != nullptr) {
					std::fprintf(fp, "Syntax error, unrecognised command\n%s\n", buf);
					std::fclose(fp);
				}
			} // warnings
			if (havestart == 0)
				ALERT(at_console, "Warning, no on_start section\n");
			// after all the lexical stuff, if we dont have any errors, pass the text into the command data types for use
			if (syntax_error == false) {
				script_parsed = true;
				ALERT(at_console, "Passing data to behaviour arrays\n\n");
				braces = 0;          // check for an even number of braces i.e. {..}
				commentline = false; // used to ignore comment lines
				start = 0;           // used to check if were in a start section or not
				havestart = 0;
				msgsection = 0; // used to check if were in a message section section or not
				ifsec = 0;      // used to check if were in an if statement
				buf = buffer;
				random_shit_error = false;
				int pnt = 0;
				char msgtext[64];
				int cnt;
				int current_msg;
				current_msg = -1;
				for (i1 = 0; i1 < 14096 && buffer[i1] != '\0'; i1++) {
					// first off.. need to ignore comment lines!
					if (std::strncmp(buf, "//", 2) == 0) {
						commentline = true;
						i1++;
						buf = buf + 1;
					}
					if (buffer[i1] == '\n')
						commentline = false;
					if (commentline == false) {
						// need to check for section definition (on_...)
						if (std::strncmp(buf, "on_start", 8) == 0) {
							if (start > 0)
								start = 99; // check for nested start defs
							else
								start = 1;
							if (havestart > 0)
								havestart = 99; // check for more than one on_start section
							else
								havestart = 1;
							for (int j = 0; j < 8; j++) {
								i1++;
								buf = buf + 1;
							} // try and move to end of on start
						}
						if (std::strncmp(buf, "on_msg", 6) == 0) {
							current_msg++;
							if (msgsection > 0)
								msgsection = 99; // check for nested msg defs
							else
								msgsection = 1;
							for (int j = 0; j < 6; j++) {
								i1++;
								buf = buf + 1;
							} // try and move to end of on_msg, now check input var!
							if (buffer[i1] != '(')
								msgsection = 99;
							// make sure it starts wif a (
							else {
								i1++;
								buf = buf + 1;
								if (buffer[i1] == ')')
									msgsection = 99;
								// make sure message isnt empty
								else {
									cnt = 0;
									// if it isn't empty, move to end (ignore msg)
									while (buffer[i1] != ')') {
										msgtext[cnt] = buffer[i1];
										cnt++;
										i1++;
										buf = buf + 1;
									}
									msgtext[cnt] = '\0'; // terminate string
									std::strcpy(msg_msg[current_msg], msgtext);
									// now we have the message, we should probably clear out, all the available data
									for (int i2 = 0; i2 < 8; i2++) {
										msg_com[current_msg].blue_av[i2] = -1;
										msg_com[current_msg].red_av[i2] = -1;
										msg_com[current_msg].yellow_av[i2] = -1;
										msg_com[current_msg].green_av[i2] = -1;
									}
									// clear the next pointer to stop it craching out.
									msg_com[current_msg].next = nullptr;
									curr = &msg_com[current_msg];
								}
							}
						} // attack
						else if (std::strncmp(buf, "blue_attack", 11) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 11; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							attack[0] = true;
						}
						else if (std::strncmp(buf, "red_attack", 10) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 10; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							attack[1] = true;
						}
						else if (std::strncmp(buf, "green_attack", 12) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 12; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							attack[2] = true;
						}
						else if (std::strncmp(buf, "yellow_attack", 13) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 13; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							attack[3] = true;
						} // defend
						else if (std::strncmp(buf, "blue_defend", 11) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 11; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							defend[0] = true;
						}
						else if (std::strncmp(buf, "red_defend", 10) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 10; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							defend[1] = true;
						}
						else if (std::strncmp(buf, "green_defend", 12) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 12; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							defend[2] = true;
						}
						else if (std::strncmp(buf, "yellow_defend", 13) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 13; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							defend[3] = true;
						} // normal (defend+attack)?? // not used yet..
						else if (std::strncmp(buf, "blue_normal", 11) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 11; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
						}
						else if (std::strncmp(buf, "red_normal", 10) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 10; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
						}
						else if (std::strncmp(buf, "green_normal", 12) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 12; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
						}
						else if (std::strncmp(buf, "yellow_normal", 13) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 13; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
						}    // point<n> available_only (exclusive)
						else if (std::strncmp(buf, "blue_available_only_point", 25) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 25; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								for (int j = 0; j < 8; j++) {
									blue_av[j] = false;
								}
								blue_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								for (int j = 0; j < 8; j++) {
									msg_com[current_msg].blue_av[j] = 0; // false
								}
								msg_com[current_msg].blue_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								for (int j = 0; j < 8; j++) {
									curr->blue_av[j] = 0; // false
								}
								curr->blue_av[pnt] = 1; // true
							}
						}
						else if (std::strncmp(buf, "red_available_only_point", 24) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 24; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								for (int j = 0; j < 8; j++) {
									red_av[j] = false;
								}
								red_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								for (int j = 0; j < 8; j++) {
									msg_com[current_msg].red_av[j] = 0; // false
								}
								msg_com[current_msg].red_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								for (int j = 0; j < 8; j++) {
									curr->red_av[j] = 0; // false
								}
								curr->red_av[pnt] = 1; // true
							}
						}
						else if (std::strncmp(buf, "green_available_only_point", 26) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 26; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								for (int j = 0; j < 8; j++) {
									green_av[j] = false;
								}
								green_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								for (int j = 0; j < 8; j++) {
									msg_com[current_msg].green_av[j] = 0; // false
								}
								msg_com[current_msg].green_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								for (int j = 0; j < 8; j++) {
									curr->green_av[j] = 0; // false
								}
								curr->green_av[pnt] = 1; // true
							}
						}
						else if (std::strncmp(buf, "yellow_available_only_point", 27) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 27; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								for (int j = 0; j < 8; j++) {
									yellow_av[j] = false;
								}
								yellow_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								for (int j = 0; j < 8; j++) {
									msg_com[current_msg].yellow_av[j] = 0; // false
								}
								msg_com[current_msg].yellow_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								for (int j = 0; j < 8; j++) {
									curr->yellow_av[j] = 0; // false
								}
								curr->yellow_av[pnt] = 1; // true
							}
						} // point<n> available (set to true)
						else if (std::strncmp(buf, "blue_available_point", 20) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 20; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								blue_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].blue_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->blue_av[pnt] = 1; // true
							}
						}
						else if (std::strncmp(buf, "red_available_point", 19) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 19; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								red_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].red_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->red_av[pnt] = 1; // true
							}
						}
						else if (std::strncmp(buf, "green_available_point", 21) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 21; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								green_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].green_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->green_av[pnt] = 1; // true
							}
						}
						else if (std::strncmp(buf, "yellow_available_point", 22) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 22; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								yellow_av[pnt] = true;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].yellow_av[pnt] = 1; // true
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->yellow_av[pnt] = 1; // true
							}
						} // point<n> not_available (set to false)
						else if (std::strncmp(buf, "blue_notavailable_point", 23) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 23; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								blue_av[pnt] = false;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].blue_av[pnt] = 0; // false
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->blue_av[pnt] = 0; // false
							}
						}
						else if (std::strncmp(buf, "red_notavailable_point", 22) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 22; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								red_av[pnt] = false;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].red_av[pnt] = 0; // false
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->red_av[pnt] = 0; // false
							}
						}
						else if (std::strncmp(buf, "green_notavailable_point", 24) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 24; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								green_av[pnt] = false;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].green_av[pnt] = 0; // false
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->green_av[pnt] = 0; // false
							}
						}
						else if (std::strncmp(buf, "yellow_notavailable_point", 25) == 0) {
							// this can only be in a section
							if (start == 0 && msgsection == 0)
								random_shit_error = true;
							for (int j = 0; j < 25; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								pnt--;
								i1++;
								buf = buf + 1;
							} // move to end
							if (start == 2) {
								yellow_av[pnt] = false;
							}
							if (msgsection == 2 && ifsec == 0) {
								msg_com[current_msg].yellow_av[pnt] = 0; // false
							}
							if (msgsection == 2 && ifsec == 2) {
								curr->yellow_av[pnt] = 0; // false
							}
						} // is point<n> availabe?
						else if (std::strncmp(buf, "if_blue_point", 13) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 13; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer..
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "b_p_%d", pnt);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "if_red_point", 12) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 12; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer..
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "r_p_%d", pnt);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "if_green_point", 14) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 14; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer..
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "g_p_%d", pnt);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "if_yellow_point", 15) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 15; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer..
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "y_p_%d", pnt);
							std::strcpy(curr->ifs, msg);
						} // is point<n> NOT availabe?
						else if (std::strncmp(buf, "ifn_blue_point", 14) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 14; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next;
							// clear next pointer..
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "b_pn_%d", pnt);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "ifn_red_point", 13) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 13; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer...
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "r_pn_%d", pnt);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "ifn_green_point", 15) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 15; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer...
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "g_pn_%d", pnt);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "ifn_yellow_point", 16) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							for (int j = 0; j < 16; j++) {
								i1++;
								buf = buf + 1;
							} // move to end
							if (buffer[i1] != '1' && buffer[i1] != '2' && buffer[i1] != '3' && buffer[i1] != '4' && buffer[i1] != '5' && buffer[i1] != '6' && buffer[i1] != '7' && buffer[i1] != '8')
								random_shit_error = true;
							else {
								pnt = atoi(&buffer[i1]); // get the var
								i1++;
								buf = buf + 1;
							} // move to end
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer...
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							std::sprintf(msg, "y_pn_%d", pnt);
							std::strcpy(curr->ifs, msg);
						}
						// multipoint ifs
						else if (std::strncmp(buf, "if_blue_mpoint", 14) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							int j;
							for (j = 0; j < 14; j++) {
								i1++;
								buf = buf + 1;
							} // move to end  // check for 8 if vals
							char pnts[9];
							for (j = 0; j < 8; j++) {
								if (buffer[i1] != '1' && buffer[i1] != '0')
									random_shit_error = true;
								else {
									pnts[j] = buffer[i1];
									i1++;
									buf = buf + 1;
								} // move to end
							}
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer...
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							pnts[8] = '\0';
							std::sprintf(msg, "b_mp_%s", pnts);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "if_red_mpoint", 13) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							int j;
							for (j = 0; j < 13; j++) {
								i1++;
								buf = buf + 1;
							} // move to end  // check for 8 if vals
							char pnts[9];
							for (j = 0; j < 8; j++) {
								if (buffer[i1] != '1' && buffer[i1] != '0')
									random_shit_error = true;
								else {
									pnts[j] = buffer[i1];
									i1++;
									buf = buf + 1;
								} // move to end
							}
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer...
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							pnts[8] = '\0';
							std::sprintf(msg, "r_mp_%s", pnts);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "if_green_mpoint", 15) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							int j;
							for (j = 0; j < 15; j++) {
								i1++;
								buf = buf + 1;
							} // move to end // check for 8 if vals
							char pnts[9];
							for (j = 0; j < 8; j++) {
								if (buffer[i1] != '1' && buffer[i1] != '0')
									random_shit_error = true;
								else {
									pnts[j] = buffer[i1];
									i1++;
									buf = buf + 1;
								} // move to end
							}
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer...
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							pnts[8] = '\0';
							std::sprintf(msg, "g_mp_%s", pnts);
							std::strcpy(curr->ifs, msg);
						}
						else if (std::strncmp(buf, "if_yellow_mpoint", 16) == 0) {
							// this can only be in a section
							if (msgsection == 0)
								random_shit_error = true;
							if (ifsec > 0)
								ifsec = 99; // check for nested if defs
							else
								ifsec = 1;
							int j;
							for (j = 0; j < 16; j++) {
								i1++;
								buf = buf + 1;
							} // move to end  // check for 8 if vals
							char pnts[9];
							for (j = 0; j < 8; j++) {
								if (buffer[i1] != '1' && buffer[i1] != '0')
									random_shit_error = true;
								else {
									pnts[j] = buffer[i1];
									i1++;
									buf = buf + 1;
								} // move to end
							}
							while (curr->next != nullptr && reinterpret_cast<int>(curr->next) != -1)
								curr = curr->next; // get to null
							curr->next = new msg_com_struct;
							curr = curr->next; // clear next pointer...
							curr->next = nullptr;
							for (int i2 = 0; i2 < 8; i2++) {
								curr->blue_av[i2] = -1;
								curr->red_av[i2] = -1;
								curr->yellow_av[i2] = -1;
								curr->green_av[i2] = -1;
							}
							pnts[8] = '\0';
							std::sprintf(msg, "y_mp_%s", pnts);
							std::strcpy(curr->ifs, msg);
						} // end of multipoint ifs
						else if (buffer[i1] != '/' && buffer[i1] != '{' && buffer[i1] != '}' && buffer[i1] != ' ' && buffer[i1] != '\n' && commentline == false && random_shit_error == false) {
							random_shit_error = true;
							ALERT(at_console, "\\/\\/\\/\\/\\/\\/\n");
							ALERT(at_console, buf);
							ALERT(at_console, "\n");
						} // do your magic lexical analysis here.. // first braces
						switch (buffer[i1]) {
						case '{':
							braces++;
							if (start == 1)
								start = 2;
							else if (start > 0)
								start = 99;
							// need to check that more braces aernt put in than nesesarry i.e. start=2 then generate error, cus only have variable setting in start section
							if (ifsec == 1)
								ifsec = 2;
							else if (ifsec > 0)
								ifsec = 99;
							if (msgsection == 1)
								msgsection = 2;
							else if (msgsection > 0 && ifsec == 0)
								msgsection = 99;
							break;
						case '}':
							braces--;
							if (start == 2)
								start = 0;
							if (ifsec == 2)
								ifsec = 0;
							if (msgsection == 2)
								msgsection = 0;
							break;
						default:;
						}
					}
					buf = buf + 1; // like i++ but for stringcmp stuff
				}
			}
			else {
				ALERT(at_console, "\nScript will not be used until there are no syntax errors\n\n");
			}
		}
		if (bfp != nullptr) {
			std::fclose(bfp);
			bfp = nullptr;
		}
	}
	if (!mr_meta)
		(*other_gFunctionTable.pfnStartFrame)();
	else
		SET_META_RESULT(MRES_HANDLED);
}

gamedll_funcs_t gGameDLLFunc;
C_DLLEXPORT int GetEntityAPI(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion) {
	// check if engine's pointer is valid and version is correct...
	std::memset(pFunctionTable, 0, sizeof(DLL_FUNCTIONS));
	if (!mr_meta) {
		// pass other DLLs engine callbacks to function table...
		if (!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION)) {
			return false; // error initializing function table!!!
		}
		gGameDLLFunc.dllapi_table = &other_gFunctionTable;
		gpGamedllFuncs = &gGameDLLFunc;
		std::memcpy(pFunctionTable, &other_gFunctionTable, sizeof(DLL_FUNCTIONS));
	}
	pFunctionTable->pfnStartFrame = StartFrame;
	pFunctionTable->pfnGameInit = GameDLLInit;
	pFunctionTable->pfnSpawn = DispatchSpawn;
	pFunctionTable->pfnThink = DispatchThink;
	pFunctionTable->pfnKeyValue = DispatchKeyValue;
	pFunctionTable->pfnClientConnect = ClientConnect;
	pFunctionTable->pfnClientDisconnect = ClientDisconnect;
	pFunctionTable->pfnClientCommand = ClientCommand;
	return true;
}

C_DLLEXPORT int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion) {
	if (other_GetNewDLLFunctions == nullptr)
		return false;
	if (!mr_meta) { // pass other DLLs engine callbacks to function table...
		if (!(*other_GetNewDLLFunctions)(pFunctionTable, interfaceVersion)) {
			return false; // error initializing function table!!!
		}
		gGameDLLFunc.newapi_table = pFunctionTable;
	}
	return true;
}

void FakeClientCommand(edict_t* pBot, const char* arg1, const char* arg2, const char* arg3) {
	int length;
	int i = 0;
	while (i < 256) {
		g_argv[i] = '\0';
		i++;
	}
	if (arg1 == nullptr || *arg1 == 0)
		return;

	if (std::strncmp(arg1, "kill", 4) == 0) {
		MDLL_ClientKill(pBot);
		return;
	}

	if (arg2 == nullptr || *arg2 == 0) {
		length = snprintf(&g_argv[0], 250, "%s", arg1);
		fake_arg_count = 1;
	}
	else if (arg3 == nullptr || *arg3 == 0) {
		length = snprintf(&g_argv[0], 250, "%s %s", arg1, arg2);
		fake_arg_count = 2;
	}
	else {
		length = snprintf(&g_argv[0], 250, "%s %s %s", arg1, arg2, arg3);
		fake_arg_count = 3;
	}
	isFakeClientCommand = 1;
	g_argv[length] = '\0'; // null terminate just in case

	if (debug_engine) {
		global::fp = UTIL_OpenFoxbotLog();
		std::fprintf(global::fp, "FakeClientCommand=%s %p\n", g_argv, static_cast<void*>(pBot));
		std::fclose(global::fp);
	}

	// allow the MOD DLL to execute the ClientCommand...
	if (mr_meta) {
		MDLL_ClientCommand(pBot);
	}
	else
		ClientCommand(pBot);

	isFakeClientCommand = 0;
}

const char* Cmd_Args() {
	if (!mr_meta) {
		if (isFakeClientCommand) {
			if (debug_engine) {
				global::fp = UTIL_OpenFoxbotLog();
				std::fprintf(global::fp, "fake cmd_args%s\n", &g_argv[0]);
				std::fclose(global::fp);
			}

			// is it a "say" or "say_team" client command ?
			if (std::strncmp("say ", g_argv, 4) == 0)
				return &g_argv[0] + 4; // skip the "say" bot client command (bug in HL engine)
			// std::sprintf(g_argv,"%s",&g_argv[0] + 4);
			if (std::strncmp("say_team ", g_argv, 9) == 0)
				return &g_argv[0] + 9;
			// skip the "say_team" bot client command (bug in HL engine)
			// std::sprintf(g_argv,"%s",&g_argv[0] + 9);

			return &g_argv[0];
		}
		return (*g_engfuncs.pfnCmd_Args)();
	}
	if (isFakeClientCommand) {
		if (debug_engine) {
			global::fp = UTIL_OpenFoxbotLog();
			std::fprintf(global::fp, "fake cmd_args%s\n", &g_argv[0]);
			std::fclose(global::fp);
		}

		// is it a "say" or "say_team" client command ?
		if (std::strncmp("say ", g_argv, 4) == 0)
			RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine)
		if (std::strncmp("say_team ", g_argv, 9) == 0)
			RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine)
		/*if(strncmp ("say ", g_argv, 4) == 0)
			  //return (&g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine)
			  std::sprintf(g_argv,"%s",&g_argv[0] + 4);
			  else if(strncmp ("say_team ", g_argv, 9) == 0)
			  //return (&g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine)
			  std::sprintf(g_argv,"%s",&g_argv[0] + 9);*/
		RETURN_META_VALUE(MRES_SUPERCEDE, &g_argv[0]);
	}
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

const char* GetArg(const char* command, unsigned int arg_number) {
	// the purpose of this function is to provide fakeclients (bots) with
	// the same Cmd_Argv convenience the engine provides to real clients.
	// This way the handling of real client commands and bot client
	// commands is exactly the same, just have a look in engine.cpp for
	// the hooking of pfnCmd_Argc, pfnCmd_Args and pfnCmd_Argv, which
	// redirects the call either to the actual engine functions
	// (when the caller is a real client), either on our function here,
	// which does the same thing, when the caller is a bot.
	unsigned int i, index = 0, arg_count = 0, fieldstart, fieldstop;
	arg[0] = 0;                         // reset arg
	const unsigned int length = std::strlen(command); // get length of command
	// while we have not reached end of line
	while (index < length && arg_count <= arg_number) {
		while (index < length && command[index] == ' ')
			index++; // ignore spaces
		// is this field multi-word between quotes or single word ?
		if (command[index] == '"') {
			index++;            // move one step further to bypass the quote
			fieldstart = index; // save field start position
			while (index < length && command[index] != '"')
				index++;            // reach end of field
			fieldstop = index - 1; // save field stop position
			index++;               // move one step further to bypass the quote
		}
		else {
			fieldstart = index; // save field start position
			while (index < length && command[index] != ' ')
				index++;            // reach end of field
			fieldstop = index - 1; // save field stop position
		}
		// is this argument we just processed the wanted one ?
		if (arg_count == arg_number) {
			for (i = fieldstart; i <= fieldstop; i++)
				arg[i - fieldstart] = command[i]; // store the field value in a string
			arg[i - fieldstart] = 0;             // terminate the string
		}
		arg_count++; // we have processed one argument more
	}
	return &arg[0]; // returns the wanted argument
}

const char* Cmd_Argv(int argc) {
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

	if (isFakeClientCommand) {
		if (!mr_meta)
			return GetArg(g_argv, argc); // if so, then return the wanted argument we know
		RETURN_META_VALUE(MRES_SUPERCEDE, GetArg(g_argv, argc));
	}
	if (!mr_meta) {
		return (*g_engfuncs.pfnCmd_Argv)(argc);
	}
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

int Cmd_Argc() {
	if (!mr_meta) {
		if (debug_engine)
			UTIL_BotLogPrintf("fake cmd_argc %d\n", fake_arg_count);

		if (isFakeClientCommand) {
			return fake_arg_count;
		}
		return (*g_engfuncs.pfnCmd_Argc)();
	}
	if (isFakeClientCommand) {
		RETURN_META_VALUE(MRES_SUPERCEDE, fake_arg_count);
	}
	RETURN_META_VALUE(MRES_IGNORED, 0);
}

// meta mod post functions

void DispatchKeyValue_Post(edict_t* pentKeyvalue, const KeyValueData* pkvd) {
	// fp=UTIL_OpenFoxbotLog(); std::fprintf(fp, "DispatchKeyValue: %x %s=%s\n",pentKeyvalue,pkvd->szKeyName,pkvd->szValue);
	// std::fclose(fp);

	if (mod_id == TFC_DLL) {
		static int flag_index;
		static edict_t* temp_pent;
		if (pentKeyvalue == pent_info_tfdetect) {
			if (std::strcmp(pkvd->szKeyName, "ammo_medikit") == 0) // max BLUE players
				max_team_players[0] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "ammo_detpack") == 0) // max RED players
				max_team_players[1] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_medikit") == 0) // max YELLOW players
				max_team_players[2] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_detpack") == 0) // max GREEN players
				max_team_players[3] = atoi(pkvd->szValue);

			else if (std::strcmp(pkvd->szKeyName, "maxammo_shells") == 0) // BLUE class limits
				team_class_limits[0] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_nails") == 0) // RED class limits
				team_class_limits[1] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_rockets") == 0) // YELLOW class limits
				team_class_limits[2] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "maxammo_cells") == 0) // GREEN class limits
				team_class_limits[3] = atoi(pkvd->szValue);

			else if (std::strcmp(pkvd->szKeyName, "team1_allies") == 0) // BLUE allies
				team_allies[0] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "team2_allies") == 0) // RED allies
				team_allies[1] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "team3_allies") == 0) // YELLOW allies
				team_allies[2] = atoi(pkvd->szValue);
			else if (std::strcmp(pkvd->szKeyName, "team4_allies") == 0) // GREEN allies
				team_allies[3] = atoi(pkvd->szValue);
		}
		else if (pent_info_tfdetect == nullptr) {
			if (std::strcmp(pkvd->szKeyName, "classname") == 0 && std::strcmp(pkvd->szValue, "info_tfdetect") == 0) {
				pent_info_tfdetect = pentKeyvalue;
			}
		}

		if (pentKeyvalue == pent_item_tfgoal) {
			if (std::strcmp(pkvd->szKeyName, "team_no") == 0)
				flags[flag_index].team_no = atoi(pkvd->szValue);

			if (std::strcmp(pkvd->szKeyName, "mdl") == 0 && (std::strcmp(pkvd->szValue, "models/flag.mdl") == 0 || std::strcmp(pkvd->szValue, "models/keycard.mdl") == 0 || std::strcmp(pkvd->szValue, "models/ball.mdl") == 0)) {
				flags[flag_index].mdl_match = true;
				num_flags++;
			}
		}
		else if (pent_item_tfgoal == nullptr) {
			if (std::strcmp(pkvd->szKeyName, "classname") == 0 && std::strcmp(pkvd->szValue, "item_tfgoal") == 0) {
				if (num_flags < MAX_FLAGS) {
					pent_item_tfgoal = pentKeyvalue;

					flags[num_flags].mdl_match = false;
					flags[num_flags].team_no = 0; // any team unless specified
					flags[num_flags].edict = pentKeyvalue;

					// in case the mdl comes before team_no
					flag_index = num_flags;
				}
			}
		}
		else {
			pent_item_tfgoal = nullptr; // reset for non-flag item_tfgoal's
		}

		if (std::strcmp(pkvd->szKeyName, "classname") == 0 && (std::strcmp(pkvd->szValue, "info_player_teamspawn") == 0 || std::strcmp(pkvd->szValue, "info_tf_teamcheck") == 0 || std::strcmp(pkvd->szValue, "i_p_t") == 0)) {
			temp_pent = pentKeyvalue;
		}
		else if (pentKeyvalue == temp_pent) {
			if (std::strcmp(pkvd->szKeyName, "team_no") == 0) {
				// int value = atoi(pkvd->szValue);

				// is_team[value-1]=true;
				// if(value > max_teams)
				// max_teams = value;
			}
		}
	}
	SET_META_RESULT(MRES_HANDLED);
}

C_DLLEXPORT int GetEntityAPI_Post(DLL_FUNCTIONS* pFunctionTable, const int interfaceVersion) {
	if (!pFunctionTable) {
		return false;
	}
	if (interfaceVersion != INTERFACE_VERSION) {
		return false;
	}
	pFunctionTable->pfnClientConnect = ClientConnect_Post;
	return true;
}

static void ProcessBotCfgFile() {
	char cmd_line[512];
	static char server_cmd[512];
	char* arg2, * arg3, * arg4;
	char msg[256];
	char msg2[524];

	if (bot_cfg_pause_time > gpGlobals->time)
		return;

	if (bot_cfg_fp == nullptr) {
		if (cfg_file == 1)
			need_to_open_cfg2 = true;
		if (cfg_file == 2) {
			cfg_file = 0;
			bot_cfg_pause_time = 0.0f; // stop it checking the file again :)
		}
		return;
	}

	int cmd_index = 0;
	cmd_line[cmd_index] = 0;
	cmd_line[510] = 0;

	int ch = fgetc(bot_cfg_fp);

	// skip any leading blanks
	while (ch == ' ')
		ch = fgetc(bot_cfg_fp);

	while (ch != EOF && ch != '\r' && ch != '\n' && cmd_index < 510) {
		if (ch == '\t') // convert tabs to spaces
			ch = ' ';

		//{fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"cfg %d %i\n",cmd_index,ch); std::fclose(fp); }
		cmd_line[cmd_index] = static_cast<char>(ch);

		ch = fgetc(bot_cfg_fp);

		// skip multiple spaces in input file
		while (cmd_line[cmd_index] == ' ' && ch == ' ')
			ch = fgetc(bot_cfg_fp);

		cmd_index++;
	}

	if (ch == '\r') // is it a carriage return?
	{
		ch = fgetc(bot_cfg_fp); // skip the linefeed
	}

	// if reached end of file, then close it
	if (ch == EOF) {
		std::fclose(bot_cfg_fp);

		bot_cfg_fp = nullptr;
		/*if(debug_engine)
						{fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"close cfg\n"); std::fclose(fp);}*/

						// dam reset
						// bot_cfg_pause_time = 0.0f;

						// sort new cfg shiznit out
		if (cfg_file == 1)
			need_to_open_cfg2 = true;
		if (cfg_file == 2) {
			cfg_file = 3; // 0 is for if it was null when we passed in
			bot_cfg_pause_time = 0.0f;
		}
	}

	cmd_line[cmd_index] = 0; // terminate the command line

	if (cmd_line[0] == '#' || cmd_line[0] == 0)
		return; // return if comment or blank line

	// DREVIL
	if (std::strncmp("echo", cmd_line, 4) == 0)
		return;

	// copy the command line to a server command buffer...
	std::strcpy(server_cmd, cmd_line);
	std::strcat(server_cmd, "\n");

	cmd_index = 0;
	const char* cmd = cmd_line;
	char* arg1 = arg2 = arg3 = arg4 = nullptr;

	// skip to blank or end of string...
	while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0 && cmd_index < 510)
		cmd_index++;
	if (cmd_line[cmd_index] == ' ' && cmd_index < 510) {
		cmd_line[cmd_index++] = 0;
		arg1 = &cmd_line[cmd_index];

		// skip to blank or end of string...
		while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0 && cmd_index < 510)
			cmd_index++;
		if (cmd_line[cmd_index] == ' ' && cmd_index < 510) {
			cmd_line[cmd_index++] = 0;
			arg2 = &cmd_line[cmd_index];

			while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0 && cmd_index < 510)
				cmd_index++;
			if (cmd_line[cmd_index] == ' ' && cmd_index < 510) {
				cmd_line[cmd_index++] = 0;
				arg3 = &cmd_line[cmd_index];

				while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0 && cmd_index < 510)
					cmd_index++;
				if (cmd_line[cmd_index] == ' ' && cmd_index < 510) {
					cmd_line[cmd_index++] = 0;
					arg4 = &cmd_line[cmd_index];
				}
			}
		}
	}

	if (std::strcmp(cmd, "addbot") == 0) {
		if (IS_DEDICATED_SERVER()) {
			std::printf("[Config] add bot (%s,%s,%s,%s)\n", arg1, arg2, arg3, arg4);
		}
		else {
			std::sprintf(msg, "[Config] add bot (%s,%s,%s,%s)\n", arg1, arg2, arg3, arg4);
			ALERT(at_console, msg);
		}
		BotCreate(nullptr, arg1, arg2, arg3, arg4);

		// have to delay here or engine gives "Tried to write to
		// uninitialized sizebuf_t" error and crashes...
		bot_cfg_pause_time = gpGlobals->time + 8.0f;
		bot_check_time = gpGlobals->time + 8.0f;

		return;
	}

	if (std::strcmp(cmd, "bot_chat") == 0) {
		changeBotSetting("bot_chat", &bot_chat, arg1, 0, 1000, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "botskill_lower") == 0) {
		changeBotSetting("botskill_lower", &botskill_lower, arg1, 1, 5, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "botskill_upper") == 0) {
		changeBotSetting("botskill_upper", &botskill_upper, arg1, 1, 5, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "bot_skill_1_aim") == 0) {
		changeBotSetting("bot_skill_1_aim", &bot_skill_1_aim, arg1, 0, 200, SETTING_SOURCE_CONFIG_FILE);

		BotUpdateSkillInaccuracy();
		return;
	}

	if (std::strcmp(cmd, "bot_aim_per_skill") == 0) {
		changeBotSetting("bot_aim_per_skill", &bot_aim_per_skill, arg1, 5, 50, SETTING_SOURCE_CONFIG_FILE);

		BotUpdateSkillInaccuracy();
		return;
	}

	if (std::strcmp(cmd, "observer") == 0) {
		int temp;
		temp = atoi(arg1);

		if (temp)
			observer_mode = true;
		else
			observer_mode = false;

		return;
	}

	if (std::strcmp(cmd, "bot_allow_moods") == 0) {
		changeBotSetting("bot_allow_moods", &bot_allow_moods, arg1, 0, 1, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "bot_allow_humour") == 0) {
		changeBotSetting("bot_allow_humour", &bot_allow_humour, arg1, 0, 1, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "bot_use_grenades") == 0) {
		changeBotSetting("bot_use_grenades", &bot_use_grenades, arg1, 0, 2, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "min_bots") == 0) {
		changeBotSetting("min_bots", &min_bots, arg1, -1, 31, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "max_bots") == 0) {
		changeBotSetting("max_bots", &max_bots, arg1, -1, MAX_BOTS, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "bot_total_varies") == 0) {
		changeBotSetting("bot_total_varies", &bot_total_varies, arg1, 0, 3, SETTING_SOURCE_CONFIG_FILE);
		return;
	}

	if (std::strcmp(cmd, "bot_team_balance") == 0) {
		if (arg1 != nullptr) {
			int temp;
			temp = atoi(arg1);
			if (temp)
				bot_team_balance = true;
			else
				bot_team_balance = false;
		}

		if (IS_DEDICATED_SERVER()) {
			if (bot_team_balance)
				std::printf("[Config] bot_team_balance (1) On\n");
			else
				std::printf("[Config] bot_team_balance (0) Off\n");
		}
		else {
			if (bot_team_balance)
				ALERT(at_console, "[Config] bot_team_balance (1) On\n");
			else
				ALERT(at_console, "[Config] bot_team_balance (0) Off\n");
		}
		return;
	}
	if (std::strcmp(cmd, "bot_bot_balance") == 0) {
		if (arg1 != nullptr) {
			int temp;
			temp = atoi(arg1);
			if (temp)
				bot_bot_balance = true;
			else
				bot_bot_balance = false;
		}

		if (IS_DEDICATED_SERVER()) {
			if (bot_bot_balance)
				std::printf("[Config] bot_bot_balance (1) On\n");
			else
				std::printf("[Config] bot_bot_balance (0) Off\n");
		}
		else {
			if (bot_bot_balance)
				ALERT(at_console, "[Config] bot_bot_balance (1) On\n");
			else
				ALERT(at_console, "[Config] bot_bot_balance (0) Off\n");
		}
		return;
	}

	if (std::strcmp(cmd, "bot_xmas") == 0) {
		int temp;
		temp = atoi(arg1);
		bot_xmas = true;
		if (temp == 0) {
			bot_xmas = false;
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] bot xmas (0) off\n");
			else {
				std::sprintf(msg, "[Config] bot xmas (0) off\n");
				ALERT(at_console, msg);
			}
		}
		else {
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] bot xmas (1) on\n");
			else {
				std::sprintf(msg, "[Config] bot xmas (1) on\n");
				ALERT(at_console, msg);
			}
		}
		return;
	}

	if (std::strcmp(cmd, "botdontshoot") == 0) {
		int temp;
		temp = atoi(arg1);
		botdontshoot = true;
		if (temp == 0) {
			botdontshoot = false;
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] botdontshoot (0) off\n");
			else {
				std::sprintf(msg, "[Config] botdontshoot (0) off\n");
				ALERT(at_console, msg);
			}
		}
		else {
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] botdontshoot (1) on\n");
			else {
				std::sprintf(msg, "[Config] botdontshoot (1) on\n");
				ALERT(at_console, msg);
			}
		}
		return;
	}

	if (std::strcmp(cmd, "botdontmove") == 0) {
		int temp;
		temp = atoi(arg1);
		botdontmove = true;
		if (temp == 0) {
			botdontmove = false;
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] botdontmove (0) off\n");
			else {
				std::sprintf(msg, "[Config] botdontmove (0) off\n");
				ALERT(at_console, msg);
			}
		}
		else {
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] botdontmove (1) on\n");
			else {
				std::sprintf(msg, "[Config] botdontmove (1) on\n");
				ALERT(at_console, msg);
			}
		}
		return;
	}

	if (std::strcmp(cmd, "bot_can_build_teleporter") == 0) {
		if (std::strcmp(arg1, "on") == 0) {
			bot_can_build_teleporter = true;
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] bot_can_build_teleporter on\n");
			else {
				std::sprintf(msg, "[Config] bot_can_build_teleporter on\n");
				ALERT(at_console, msg);
			}
		}
		else if (std::strcmp(arg1, "off") == 0) {
			bot_can_build_teleporter = false;
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] bot_can_build_teleporter off\n");
			else {
				std::sprintf(msg, "[Config] bot_can_build_teleporter off\n");
				ALERT(at_console, msg);
			}
		}
		return;
	}

	if (std::strcmp(cmd, "bot_can_use_teleporter") == 0) {
		if (std::strcmp(arg1, "on") == 0) {
			bot_can_use_teleporter = true;
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] bot_can_use_teleporter on\n");
			else {
				std::sprintf(msg, "[Config] bot_can_use_teleporter on\n");
				ALERT(at_console, msg);
			}
		}
		else if (std::strcmp(arg1, "off") == 0) {
			bot_can_use_teleporter = false;
			if (IS_DEDICATED_SERVER())
				std::printf("[Config] bot_can_use_teleporter off\n");
			else {
				std::sprintf(msg, "[Config] bot_can_use_teleporter off\n");
				ALERT(at_console, msg);
			}
		}
		return;
	}

	if (std::strcmp(cmd, "pause") == 0) {
		bot_cfg_pause_time = gpGlobals->time + atoi(arg1);
		bot_check_time = bot_cfg_pause_time; // bot_check_time governs bot spawn time too
		if (IS_DEDICATED_SERVER())
			std::printf("[Config] pause has been set to %s\n", arg1);
		else {
			std::sprintf(msg, "[Config] pause has been set to %s\n", arg1);
			ALERT(at_console, msg);
		}
		return;
	}

	if (std::strcmp(cmd, "bot_create_interval") == 0) {
		bot_create_interval = static_cast<float>(atoi(arg1));
		if (bot_create_interval < 1.0f || bot_create_interval > 8.0f)
			bot_create_interval = 3.0f;

		if (IS_DEDICATED_SERVER())
			std::printf("[Config] bot_create_interval has been set to %s\n", arg1);
		else {
			std::sprintf(msg, "[Config] bot_create_interval has been set to %s\n", arg1);
			ALERT(at_console, msg);
		}
		return;
	}

	if (std::strcmp(cmd, "defensive_chatter") == 0) {
		const int temp = atoi(arg1);
		if (temp == 0)
			defensive_chatter = false;
		else
			defensive_chatter = true;

		if (IS_DEDICATED_SERVER()) {
			std::printf("[Config] defensive chatter is %s\n", arg1);
		}
		else {
			std::sprintf(msg, "[Config] defensive chatter is %s\n", arg1);
			ALERT(at_console, msg);
		}
		return;
	}
	if (std::strcmp(cmd, "offensive_chatter") == 0) {
		const int temp = atoi(arg1);
		if (temp == 0)
			offensive_chatter = false;
		else
			offensive_chatter = true;

		if (IS_DEDICATED_SERVER()) {
			std::printf("[Config] offensive chatter is %s\n", arg1);
		}
		else {
			std::sprintf(msg, "[Config] offensive chatter is %s\n", arg1);
			ALERT(at_console, msg);
		}
		return;
	}
	/*	if(std::strcmp(cmd, "frag_commander") == 0)
					{
									int temp = atoi( arg1 );
									if(temp == 0)
													frag_commander = false;
									else
													frag_commander = true;

									if(IS_DEDICATED_SERVER())
									{
													std::printf("[Config] Score-based commander is %s\n",arg1);
									}
									else
									{
													std::sprintf(msg,"[Config] Score-based commander is %s\n",arg1);
													ALERT( at_console, msg);
									}
									return;
					}*/

	std::sprintf(msg2, "executing: %s\n", server_cmd);
	ALERT(at_console, msg);

	if (IS_DEDICATED_SERVER())
		std::printf("%s", msg);

	SERVER_COMMAND(server_cmd);
}

void UTIL_SavePent(edict_t* pent) {
	FILE* fp = UTIL_OpenFoxbotLog();
	if (fp == nullptr)
		return;

	std::fprintf(fp, "*edict_t %p\n", static_cast<void*>(pent));
	std::fprintf(fp, "classname %s\n", STRING(pent->v.classname));
	std::fprintf(fp, "globalname %s\n", STRING(pent->v.globalname));
	std::fprintf(fp, "origin %f %f %f\n", pent->v.origin.x, pent->v.origin.y, pent->v.origin.z);
	std::fprintf(fp, "oldorigin %f %f %f\n", pent->v.oldorigin.x, pent->v.oldorigin.y, pent->v.oldorigin.z);
	std::fprintf(fp, "velocity %f %f %f\n", pent->v.velocity.x, pent->v.velocity.y, pent->v.velocity.z);
	std::fprintf(fp, "basevelocity %f %f %f\n", pent->v.basevelocity.x, pent->v.basevelocity.y, pent->v.basevelocity.z);
	std::fprintf(fp, "clbasevelocity %f %f %f\n", pent->v.clbasevelocity.x, pent->v.clbasevelocity.y, pent->v.clbasevelocity.z);
	std::fprintf(fp, "movedir %f %f %f\n", pent->v.movedir.x, pent->v.movedir.y, pent->v.movedir.z);
	std::fprintf(fp, "angles %f %f %f\n", pent->v.angles.x, pent->v.angles.y, pent->v.angles.z);
	std::fprintf(fp, "avelocity %f %f %f\n", pent->v.avelocity.x, pent->v.avelocity.y, pent->v.avelocity.z);
	std::fprintf(fp, "punchangle %f %f %f\n", pent->v.punchangle.x, pent->v.punchangle.y, pent->v.punchangle.z);
	std::fprintf(fp, "v_angles %f %f %f\n", pent->v.v_angle.x, pent->v.v_angle.y, pent->v.v_angle.z);
	std::fprintf(fp, "endpos %f %f %f\n", pent->v.endpos.x, pent->v.endpos.y, pent->v.endpos.z);
	std::fprintf(fp, "startpos %f %f %f\n", pent->v.startpos.x, pent->v.startpos.y, pent->v.startpos.z);
	std::fprintf(fp, "impacttime %f\n", pent->v.impacttime);
	std::fprintf(fp, "starttime %f\n", pent->v.starttime);
	std::fprintf(fp, "fixangle %d\n", pent->v.fixangle);
	std::fprintf(fp, "idealpitch %f\n", pent->v.idealpitch);
	std::fprintf(fp, "pitch_speed %f\n", pent->v.pitch_speed);
	std::fprintf(fp, "ideal_yaw %f\n", pent->v.ideal_yaw);
	std::fprintf(fp, "yaw_speed %f\n", pent->v.yaw_speed);
	std::fprintf(fp, "modelindex %d\n", pent->v.modelindex);
	std::fprintf(fp, "model %s\n", STRING(pent->v.model));
	std::fprintf(fp, "viewmodel %d\n", pent->v.viewmodel);
	std::fprintf(fp, "weaponmodel %d\n", pent->v.weaponmodel);
	std::fprintf(fp, "absmin %f %f %f\n", pent->v.absmin.x, pent->v.absmin.y, pent->v.absmin.z);
	std::fprintf(fp, "absmax %f %f %f\n", pent->v.absmax.x, pent->v.absmax.y, pent->v.absmax.z);
	std::fprintf(fp, "mins %f %f %f\n", pent->v.mins.x, pent->v.mins.y, pent->v.mins.z);
	std::fprintf(fp, "maxs %f %f %f\n", pent->v.maxs.x, pent->v.maxs.y, pent->v.maxs.z);
	std::fprintf(fp, "size %f %f %f\n", pent->v.size.x, pent->v.size.y, pent->v.size.z);
	std::fprintf(fp, "ltime %f\n", pent->v.ltime);
	std::fprintf(fp, "nextthink %f\n", pent->v.nextthink);
	std::fprintf(fp, "movetype %d\n", pent->v.movetype);
	std::fprintf(fp, "solid %d\n", pent->v.solid);
	std::fprintf(fp, "skin %d\n", pent->v.skin);
	std::fprintf(fp, "body %d\n", pent->v.body);
	std::fprintf(fp, "effects %d\n", pent->v.effects);
	std::fprintf(fp, "gravity %f\n", pent->v.gravity);
	std::fprintf(fp, "friction %f\n", pent->v.friction);
	std::fprintf(fp, "light_level %d %d\n", pent->v.light_level, GETENTITYILLUM(pent));
	if (pent->v.pContainingEntity != nullptr)
		std::fprintf(fp, "cont light_level %d\n", GETENTITYILLUM(pent->v.pContainingEntity));
	std::fprintf(fp, "health %f\n", pent->v.health);
	std::fprintf(fp, "frags %f\n", pent->v.frags);
	std::fprintf(fp, "weapons %d\n", pent->v.weapons);
	std::fprintf(fp, "takedamage %f\n", pent->v.takedamage);
	std::fprintf(fp, "deadflag %d\n", pent->v.deadflag);
	std::fprintf(fp, "view_ofs %f %f %f\n", pent->v.view_ofs.x, pent->v.view_ofs.y, pent->v.view_ofs.z);
	std::fprintf(fp, "button %d\n", pent->v.button);
	std::fprintf(fp, "impulse %d\n", pent->v.impulse);
	std::fprintf(fp, "*chain %p\n", static_cast<void*>(pent->v.chain));
	std::fprintf(fp, "*dmg_inflictor %p\n", static_cast<void*>(pent->v.dmg_inflictor));
	std::fprintf(fp, "*enemy %p\n", static_cast<void*>(pent->v.enemy));
	std::fprintf(fp, "*aiment %p\n", static_cast<void*>(pent->v.aiment));
	std::fprintf(fp, "*owner %p\n", static_cast<void*>(pent->v.owner));
	std::fprintf(fp, "*grounentity %p\n", static_cast<void*>(pent->v.groundentity));
	std::fprintf(fp, "spawnflags %d\n", pent->v.spawnflags);
	std::fprintf(fp, "flags %d\n", pent->v.flags);
	std::fprintf(fp, "colormap %d\n", pent->v.colormap);
	std::fprintf(fp, "team %d\n", pent->v.team);
	std::fprintf(fp, "max_health %f\n", pent->v.max_health);
	std::fprintf(fp, "teleport_time %f\n", pent->v.teleport_time);
	std::fprintf(fp, "armortype %f\n", pent->v.armortype);
	std::fprintf(fp, "armorvalue %f\n", pent->v.armorvalue);
	std::fprintf(fp, "waterlevel %d\n", pent->v.waterlevel);
	std::fprintf(fp, "watertype %d\n", pent->v.watertype);
	std::fprintf(fp, "target %s\n", STRING(pent->v.target));
	std::fprintf(fp, "targetname %s\n", STRING(pent->v.targetname));
	std::fprintf(fp, "netname %s\n", STRING(pent->v.netname));
	std::fprintf(fp, "message %s\n", STRING(pent->v.message));
	std::fprintf(fp, "dmg_take %f\n", pent->v.dmg_take);
	std::fprintf(fp, "dmg_save %f\n", pent->v.dmg_save);
	std::fprintf(fp, "dmg %f\n", pent->v.dmg);
	std::fprintf(fp, "dmgtime %f\n", pent->v.dmgtime);
	std::fprintf(fp, "noise %s\n", STRING(pent->v.noise));
	std::fprintf(fp, "noise1 %s\n", STRING(pent->v.noise1));
	std::fprintf(fp, "noise2 %s\n", STRING(pent->v.noise2));
	std::fprintf(fp, "noise3 %s\n", STRING(pent->v.noise3));
	std::fprintf(fp, "speed %f\n", pent->v.speed);
	std::fprintf(fp, "air_finished %f\n", pent->v.air_finished);
	std::fprintf(fp, "pain_finished %f\n", pent->v.pain_finished);
	std::fprintf(fp, "pContainingEntity %p\n", static_cast<void*>(pent->v.pContainingEntity));
	std::fprintf(fp, "playerclass %d\n", pent->v.playerclass);
	std::fprintf(fp, "maxspeed %f\n", pent->v.maxspeed);
	std::fprintf(fp, "fov %f\n", pent->v.fov);
	std::fprintf(fp, "weaponanim %d\n", pent->v.weaponanim);
	std::fprintf(fp, "pushmsec %d\n", pent->v.pushmsec);
	std::fprintf(fp, "bInDuck %d\n", pent->v.bInDuck);
	std::fprintf(fp, "flTimeStepSound %d\n", pent->v.flTimeStepSound);
	std::fprintf(fp, "flSwimTime %d\n", pent->v.flSwimTime);
	std::fprintf(fp, "flDuckTime %d\n", pent->v.flDuckTime);
	std::fprintf(fp, "iStepLeft %d\n", pent->v.iStepLeft);
	std::fprintf(fp, "flFallVelocity %f\n", pent->v.flFallVelocity);
	std::fprintf(fp, "gamestate %d\n", pent->v.gamestate);
	std::fprintf(fp, "oldbuttons %d\n", pent->v.oldbuttons);
	std::fprintf(fp, "groupinfo %d\n", pent->v.groupinfo);
	std::fprintf(fp, "iuser1 %d\n", pent->v.iuser1);
	std::fprintf(fp, "iuser2 %d\n", pent->v.iuser2);
	std::fprintf(fp, "iuser3 %d\n", pent->v.iuser3);
	std::fprintf(fp, "iuser4 %d\n", pent->v.iuser4);
	std::fprintf(fp, "fuser1 %f\n", pent->v.fuser1);
	std::fprintf(fp, "fuser2 %f\n", pent->v.fuser2);
	std::fprintf(fp, "fuser3 %f\n", pent->v.fuser3);
	std::fprintf(fp, "fuser4 %f\n", pent->v.fuser4);
	std::fprintf(fp, "vuser1 %f %f %f\n", pent->v.vuser1.x, pent->v.vuser1.y, pent->v.vuser1.z);
	std::fprintf(fp, "vuser2 %f %f %f\n", pent->v.vuser2.x, pent->v.vuser2.y, pent->v.vuser2.z);
	std::fprintf(fp, "vuser3 %f %f %f\n", pent->v.vuser3.x, pent->v.vuser3.y, pent->v.vuser3.z);
	std::fprintf(fp, "vuser4 %f %f %f\n", pent->v.vuser4.x, pent->v.vuser4.y, pent->v.vuser4.z);
	std::fprintf(fp, "euser1 %p\n", static_cast<void*>(pent->v.euser1));
	std::fprintf(fp, "euser2 %p\n", static_cast<void*>(pent->v.euser2));
	std::fprintf(fp, "euser3 %p\n", static_cast<void*>(pent->v.euser3));
	std::fprintf(fp, "euser4 %p\n", static_cast<void*>(pent->v.euser4));

	std::fprintf(fp, "-info buffer %s\n", g_engfuncs.pfnGetInfoKeyBuffer(pent));
	std::fclose(fp);
}

static void DisplayBotInfo() {
	char msg[255];
	char msg2[512];

	if (IS_DEDICATED_SERVER()) {
		// tell the console all the bot vars

		std::snprintf(msg2, 511, "--FoxBot Loaded--\n--Visit 'www.apg-clan.org' for updates and info--\n");
		msg2[511] = '\0'; // just in case
		std::printf("%s", msg2);

		/*	std::sprintf(msg,"--* foxbot v%d.%d build# %d *--\n",
						VER_MAJOR,VER_MINOR,VER_BUILD);*/

		std::sprintf(msg, "--* foxbot v%d.%d *--\n", VER_MAJOR, VER_MINOR);
		std::printf("%s", msg);

		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		std::sprintf(msg, "\n--FoxBot info--\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		// waypoints
		if (num_waypoints > 0)
			std::sprintf(msg, "Waypoints loaded\n");
		else
			std::sprintf(msg, "Waypoints NOT loaded\n--Warning bots will not navigate correctly!--\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		// area file
		if (num_areas > 0)
			std::sprintf(msg, "Areas loaded\n");
		else
			std::sprintf(msg, "Areas not loaded\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		// scripts...loaded/passed?
		if (script_loaded) {
			if (script_parsed)
				std::sprintf(msg, "Script loaded and parsed\n");
			else
				std::sprintf(msg, "Script loaded and NOT parsed\n--Warning script file has an error in it, will NOT be used!--\n");
		}
		else
			std::sprintf(msg, "No script file loaded\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// now bots vars
		std::sprintf(msg, "\n--FoxBot vars--\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// bot skill levels
		std::sprintf(msg, "botskill_lower %d\nbotskill_upper %d\n", botskill_lower, botskill_upper);
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		std::sprintf(msg, "max_bots %d\nmin_bots %d\n", max_bots, min_bots);
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// bot chat
		std::sprintf(msg, "Bot chat %d\n", bot_chat);
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		if (bot_team_balance)
			std::sprintf(msg, "Bot auto team balance On\n");
		else
			std::sprintf(msg, "Bot auto team balance Off\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		if (bot_bot_balance)
			std::sprintf(msg, "Bot per team balance On\n");
		else
			std::sprintf(msg, "Bot per team balance Off\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		std::sprintf(msg, "\n--All bot commands must be enclosed in quotes--\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		std::sprintf(msg, "e.g. bot \"bot_chat 20\"\n\n");
		std::printf("%s", msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		ALERT(at_logged, "[FOXBOT]: %s", msg2);
		// clear it up if it was 0
	}
	else {
		// have to switch developer 'on' if its not already
		const char* cvar_dev = const_cast<char*>(CVAR_GET_STRING("developer"));
		int dev;
		if (std::strcmp(cvar_dev, "0") == 0)
			dev = 0;
		else
			dev = 1;
		// tell the console all the bot vars
		if (dev == 0)
			CVAR_SET_STRING("developer", "1");

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
		std::sprintf(msg, "--FoxBot Loaded--\n--Visit 'www.apg-clan.org' for updates and info--\n");
		ALERT(at_console, msg);
		std::printf("%s", msg2);

		/*	std::sprintf(msg,"--* foxbot v%d.%d build# %d *--\n",
						VER_MAJOR,VER_MINOR,VER_BUILD);*/

		std::sprintf(msg, "--* foxbot v%d.%d *--\n", VER_MAJOR, VER_MINOR);
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		std::sprintf(msg, "\n--FoxBot info--\n");
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// waypoints
		if (num_waypoints > 0)
			std::sprintf(msg, "Waypoints loaded\n");
		else
			std::sprintf(msg, "Waypoints NOT loaded\n--Warning, bots will not navigate correctly!--\n");
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// area file
		if (num_areas > 0)
			std::sprintf(msg, "Areas loaded\n");
		else
			std::sprintf(msg, "Areas not loaded\n");

		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// scripts...loaded/passed?
		if (script_loaded) {
			if (script_parsed)
				std::sprintf(msg, "Script loaded and parsed\n");
			else
				std::sprintf(msg, "Script loaded and NOT parsed\n--Warning script file has an error in it and will NOT be used!--\n");
		}
		else
			std::sprintf(msg, "No script file loaded\n");
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// now bots vars
		std::sprintf(msg, "\n--FoxBot vars--\n");
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// bot skill levels
		std::sprintf(msg, "botskill_lower %d\nbotskill_upper %d\n", botskill_lower, botskill_upper);
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		std::sprintf(msg, "max_bots %d\nmin_bots %d\n", max_bots, min_bots);
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		// bot chat
		std::sprintf(msg, "Bot chat %d\n", bot_chat);
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		if (bot_team_balance)
			std::sprintf(msg, "Bot auto team balance On\n");
		else
			std::sprintf(msg, "Bot auto team balance Off\n");
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));

		if (bot_bot_balance)
			std::sprintf(msg, "Bot per team balance On\n");
		else
			std::sprintf(msg, "Bot per team balance Off\n");
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		std::sprintf(msg, "\n");
		ALERT(at_console, msg);
		std::strncat(msg2, msg, 511 - std::strlen(msg2));
		ALERT(at_logged, "[FOXBOT]: %s", msg2);
		ALERT(at_console, "\n\n\n");
		// clear it up if it was 0
		if (dev == 0)
			CVAR_SET_STRING("developer", "0");
		if (!display_bot_vars)
			FOX_HudMessage(INDEXENT(1), h, msg2);
		if (waypoint_author[0] != '\0') {
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
			h.y = 0.8f;
			std::sprintf(msg2, "-- Waypoint author: %s --", waypoint_author);
			FOX_HudMessage(INDEXENT(1), h, msg);
		}
	}
}

// This function can be used to process a specified bot setting command.
// It can handle bot options specified in a config file or from the
// command line or console.
// e.g. the max_bots setting.
static void changeBotSetting(const char* settingName, int* setting, const char* arg1, const int minValue, const int maxValue, const int settingSource) {
	bool settingWasChanged = false;

	char msg[128] = "";

	// if the setting change is from a config file set up a message about it
	char configMessage[] = "[Config] ";
	if (settingSource != SETTING_SOURCE_CONFIG_FILE)
		configMessage[0] = '\0';

	// if an argument was supplied change the setting with it
	if (arg1 != nullptr && *arg1 != 0) {
		const int temp = atoi(arg1);
		if (temp >= minValue && temp <= maxValue) {
			*setting = temp;
			settingWasChanged = true;
		}
		else {
			// report that a bad setting was requested

			snprintf(msg, 128, "%s%s should be set from %d to %d\n", configMessage, settingName, minValue, maxValue);
			msg[127] = '\0';

			if (settingSource == SETTING_SOURCE_CLIENT_COMMAND)
				ClientPrint(INDEXENT(1), HUD_PRINTNOTIFY, msg);
			else if (settingSource == SETTING_SOURCE_SERVER_COMMAND)
				std::printf("%s", msg);
			else if (settingSource == SETTING_SOURCE_CONFIG_FILE) {
				if (IS_DEDICATED_SERVER())
					std::printf("%s", msg);
				else
					ALERT(at_console, msg);
			}
		}
	}

	// report if the setting was actually changed or not
	if (settingWasChanged) {
		snprintf(msg, 128, "%s%s has been set to %d\n", configMessage, settingName, *setting);
		msg[127] = '\0';
	}
	else {
		snprintf(msg, 128, "%s%s is currently set to %d\n", configMessage, settingName, *setting);
		msg[127] = '\0';
	}

	if (settingSource == SETTING_SOURCE_CLIENT_COMMAND)
		ClientPrint(INDEXENT(1), HUD_PRINTNOTIFY, msg);
	else if (settingSource == SETTING_SOURCE_SERVER_COMMAND)
		std::printf("%s", msg);
	else if (settingSource == SETTING_SOURCE_CONFIG_FILE) {
		if (IS_DEDICATED_SERVER())
			std::printf("%s", msg);
		else
			ALERT(at_console, msg);
	}
}

// This a general purpose function for kicking bots.
// Set team to -1 if you don't want to specify a team.
// Set totalToKick to MAX_BOTS if you want to kick all of them.
static void kickBots(int totalToKick, const int team) {
	// sanity check
	if (totalToKick < 1 || team > 3)
		return;

	if (totalToKick >= MAX_BOTS)
		totalToKick = MAX_BOTS - 1;

	for (int index = MAX_BOTS - 1; index >= 0 && totalToKick > 0; index--) {
		if (bots[index].is_used // is this slot used?
			&& !FNullEnt(bots[index].pEdict) && (team < 0 || bots[index].bot_team == team)) {
			char cmd[80];

			snprintf(cmd, 80, "kick \"%s\"\n", bots[index].name);
			cmd[79] = '\0';
			SERVER_COMMAND(cmd); // kick the bot using (kick "name")
			ClearKickedBotsData(index, true);
			bots[index].is_used = false;
			--totalToKick;
		}
	}
}

// This function will kick a random bot.
static void kickRandomBot() {
	int bot_list[MAX_BOTS];
	int bot_total = 0;
	int index;

	// count and list the bots
	for (index = 0; index < MAX_BOTS; index++) {
		if (bots[index].is_used // is this slot used?
			&& !FNullEnt(bots[index].pEdict)) {
			bot_list[bot_total] = index;
			++bot_total;
		}
	}

	// sanity checking
	if (bot_total < 1 || bot_total >= MAX_BOTS)
		return;

	// kick a random bot from the list
	// double check we're kicking an active bot too
	index = bot_list[random_long(0, bot_total - 1)];
	if (index > -1 && index < MAX_BOTS && bots[index].is_used && !FNullEnt(bots[index].pEdict)) {
		char cmd[80];

		snprintf(cmd, 80, "kick \"%s\"\n", bots[index].name);
		cmd[79] = '\0';
		SERVER_COMMAND(cmd); // kick the bot using (kick "name")
		ClearKickedBotsData(index, true);
		bots[index].is_used = false;
	}
}

// This function can be called when a bot has been kicked and will attempt
// to clear all the bots data that might be unsafe if used.  e.g. pointers.
// Set eraseBotsName to true if you want the bots name to be forgotten too.
static void ClearKickedBotsData(const int botIndex, const bool eraseBotsName) {
	bots[botIndex].current_wp = -1; // just for the hell of it

	// the unfeasibly scary pointers of great doom!
	bots[botIndex].enemy.ptr = nullptr;
	bots[botIndex].lastEnemySentryGun = nullptr;
	bots[botIndex].suspectedSpy = nullptr;
	bots[botIndex].killer_edict = nullptr;
	bots[botIndex].killed_edict = nullptr;
	bots[botIndex].has_sentry = false;
	bots[botIndex].sentry_edict = nullptr;
	bots[botIndex].has_dispenser = false;
	bots[botIndex].dispenser_edict = nullptr;
	bots[botIndex].tpEntrance = nullptr;
	bots[botIndex].tpExit = nullptr;

	bots[botIndex].sentryWaypoint = -1;
	bots[botIndex].tpEntranceWP = -1;
	bots[botIndex].tpExitWP = -1;

	// clear the messaging data
	bots[botIndex].newmsg = false;
	bots[botIndex].message[0] = '\0';
	bots[botIndex].msgstart[0] = '\0';

	// clear the bots knowledge of any teleporters
	for (int teleIndex = 0; teleIndex < MAX_BOT_TELEPORTER_MEMORY; teleIndex++) {
		BotForgetTeleportPair(&bots[botIndex], teleIndex);
	}

	// we don't like ya, so sod off!
	if (eraseBotsName) {
		botJustJoined[botIndex] = true;
		bots[botIndex].name[0] = '\0';
		bots[botIndex].bot_team = -1; // no team chosen
	}
}

#if 0 // TODO: this function is not useful enough yet
// This function scans a maps waypoints for flag waypoints that belong to
// one team or another.  It then tries to find out which teams are trying
// to carry which teams flags.
static void FlagTeamCheck()
{
	if (num_waypoints < 1)
		return;

	int flagTeam[4] = { -2, -2, -2, -2 };

	// find the nearest waypoint with the matching flags...
	for (int index = 0; index < num_waypoints; index++)
	{
		if ((waypoints[index].flags & W_FL_TFC_FLAG) != W_FL_TFC_FLAG)
			continue;  // skip this waypoint if the flags don't match

		// skip any deleted waypoints or aiming waypoints
		if (waypoints[index].flags & W_FL_DELETED
			|| waypoints[index].flags & W_FL_AIMING)
			continue;

		// only check team specific flag waypoints
		// (so as to avoid potential confusion)
		if (!(waypoints[index].flags & W_FL_TEAM_SPECIFIC))
			continue;

		// look for a flag very near to this flag waypoint
		edict_t* pent = nullptr;
		while ((pent = FIND_ENTITY_BY_CLASSNAME(pent, "item_tfgoal")) != nullptr
			&& !FNullEnt(pent))
		{
			if (!(pent->v.effects & EF_NODRAW)
				&& VectorsNearerThan(waypoints[index].origin, pent->v.origin, 80.0))
			{
				for (int j = 0; j < 4; j++)
				{
					if ((waypoints[index].flags & W_FL_TEAM) == j)
					{
						flagTeam[j] = UTIL_GetFlagsTeam(pent);
						break;
					}
				}
			}
		}
	}
}
#endif