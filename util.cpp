// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/***
 *
 *  Copyright (c) 1999, Valve LLC. All rights reserved.
 *
 *  This product contains software technology licensed from Id
 *  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *  All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/

 //
 // FoXBot
 //
 // (http://foxbot.net)
 //
 // util.cpp
 //

#include "extdll.h"
#include <util.h>

#include <ctime>
#include <cmath>

#include "bot.h"
#include "bot_func.h"

// this is used to store the current Foxbot directory path
static char foxbot_path[160] = "";

// the full path and name of foxbots log file
static char foxbot_logname[160] = "";

extern int mod_id;
extern bot_t bots[32];
extern edict_t* pent_info_ctfdetect;

int gmsgHUDNotify = 0;

int gmsgTextMsg = 0;
int gmsgSayText = 0;
int gmsgShowMenu = 0;

static unsigned long lseed = static_cast<unsigned long>(std::time(nullptr));

// FUNCTION PROTOTYPES
static void UTIL_FindFoxbotPath();

#ifdef __BORLANDC__
// The Borland 5.0 compiler doesn't ignore maths errors(unlike a few other compilers).
// So here's a maths error handler to cope with problems such as: sqrt(-9).
int _RTLENTRY _EXPFUNC _matherr(struct _exception* except_ptr) {
	static bool errorReported = false;

	// make one log report about the maths error reported
	if (!errorReported) {
		errorReported = true;

		if (except_ptr->name != NULL) // just being neurotic
			UTIL_BotLogPrintf("Maths error reported: %s %f %f\n", except_ptr->name, except_ptr->arg1, except_ptr->arg2);
	}

	// report that everything is A-OK even though it isn't really
	// (so that Foxbot users don't have to put up with math error warning dialogs)
	except_ptr->retval = 1.0;
	return 1;
}
#endif

// This is a long version of rand().
// This function seems to give good variation even when asked for a
// smaller spread of numbers.
long random_long(const long lowval, const long highval) {
	const long spread = highval - lowval + 1;

	// make sure a sensible number was requested
	if (spread < 2) {
		//	UTIL_BotLogPrintf("dumb random request: %d, %d\n", lowval, highval);
		return lowval;
	}

	lseed = (lseed * 1103515245 + 12345) % 2147483647;

	return static_cast<long>(lseed) / 3 % spread + lowval;
}

//	This is a float version of rand().
//	This function seems to give good variation even with smaller
//	number spreads.
float random_float(const float lowval, const float highval) {
	if (highval <= lowval)
		return lowval;

	lseed = (lseed * 1103515245 + 12345) % 2147483647;

	return lowval + static_cast<float>(lseed) / (static_cast<float>(LONG_MAX) / (highval - lowval));
}

// This function is a quick simple way of testing the distance between two
// vectors against some distance value.
// This should be faster than using the Vector classes .Length() function
// because it requires no function call to sqrt().
bool VectorsNearerThan(const Vector& r_vOne, const Vector& r_vTwo, double value) {
	value = value * value;

	const Vector distance = r_vOne - r_vTwo;
	double temp = static_cast<double>(distance.x * distance.x) + static_cast<double>(distance.y * distance.y);

	// perform an early 2 dimensional check, because most maps
	// are wider/longer than they are tall
	if (temp > value)
		return false;
	temp += static_cast<double>(distance.z * distance.z);

	// final check(3 dimensional)
	if (temp < value)
		return true;

	return false;
}

#if FALSE // this function is unused so far
int UTIL_SentryLevel(edict_t* pEntity)
{
	char* model = (char*)STRING(pEntity->v.model);

	if (model[13] == '1')
		return 1;
	else if (model[13] == '2')
		return 2;
	else if (model[13] == '3')
		return 3;

	return 0;
}
#endif

Vector UTIL_VecToAngles(const Vector& vec) {
	float rgflVecOut[3];
	VEC_TO_ANGLES(vec, rgflVecOut);
	return {rgflVecOut};
}

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, const IGNORE_MONSTERS igmon, const IGNORE_GLASS ignoreGlass, edict_t* pentIgnore, TraceResult* ptr) {
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0) | (ignoreGlass ? 0x100 : 0), pentIgnore, ptr);
}

void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, const IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr) { TRACE_LINE(vecStart, vecEnd, igmon == ignore_monsters, pentIgnore, ptr); }

void UTIL_MakeVectors(const Vector& vecAngles) { MAKE_VECTORS(vecAngles); }

edict_t* UTIL_FindEntityInSphere(edict_t* pentStart, const Vector& vecCenter, const float flRadius) {
	edict_t* pentEntity = FIND_ENTITY_IN_SPHERE(pentStart, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return pentEntity;

	return nullptr;
}

edict_t* UTIL_FindEntityByString(edict_t* pentStart, const char* szKeyword, const char* szValue) {
	edict_t* pentEntity = FIND_ENTITY_BY_STRING(pentStart, szKeyword, szValue);

	if (!FNullEnt(pentEntity))
		return pentEntity;
	return nullptr;
}

edict_t* UTIL_FindEntityByClassname(edict_t* pentStart, const char* szName) { return UTIL_FindEntityByString(pentStart, "classname", szName); }

edict_t* UTIL_FindEntityByTargetname(edict_t* pentStart, const char* szName) { return UTIL_FindEntityByString(pentStart, "targetname", szName); }

int UTIL_PointContents(const Vector& vec) { return POINT_CONTENTS(vec); }

void UTIL_SetSize(entvars_t* pev, const Vector& vecMin, const Vector& vecMax) { SET_SIZE(ENT(pev), vecMin, vecMax); }

void UTIL_SetOrigin(entvars_t* pev, const Vector& vecOrigin) { SET_ORIGIN(ENT(pev), vecOrigin); }

void HUDNotify(edict_t* pEntity, const char* msg_name) {
	if (gmsgHUDNotify == 0)
		gmsgHUDNotify = REG_USER_MSG("HudText", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgHUDNotify, nullptr, pEntity);
	WRITE_STRING(msg_name);
	MESSAGE_END();
}

void ClientPrint(edict_t* pEntity, const int msg_dest, const char* msg_name) {
	if (gmsgTextMsg == 0)
		gmsgTextMsg = REG_USER_MSG("TextMsg", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, nullptr, pEntity);

	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);
	MESSAGE_END();
}

void UTIL_SayText(const char* pText, edict_t* pEdict) {
	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pEdict);
	WRITE_BYTE(ENTINDEX(pEdict));
	// if(mod_id == FRONTLINE_DLL)
	//    WRITE_SHORT(0);
	WRITE_STRING(pText);
	MESSAGE_END();
}

void UTIL_HostSay(edict_t* pEntity, const int teamonly, const char* message) {
	const char* pc;

	// make sure the text has content
	for (pc = message; pc != nullptr && *pc != 0; pc++) {
		if (isprint(*pc) && !isspace(*pc)) {
			// we've found an alphanumeric character, so text is valid
			pc = nullptr;
			break;
		}
	}

	if (pc != nullptr)
		return; // no character found, so say nothing

	char text[128];

	// turn on color set 2  (color on,  no sound)
	if (teamonly)
		snprintf(text, 127, "%c(TEAM) %s: %s\n", 2, STRING(pEntity->v.netname), message);
	else
		snprintf(text, 127, "%c%s: %s\n", 2, STRING(pEntity->v.netname), message);

	text[127] = '\0';

	// loop through all players, starting with the first player.
	// This may return the world in single player if the client types
	// something between levels or during spawn
	// so check it, or it will infinite loop

	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	const int sender_team = UTIL_GetTeam(pEntity);

	edict_t* client = nullptr;
	while ((client = UTIL_FindEntityByClassname(client, "player")) != nullptr && !FNullEnt(client)) {
		if (client == pEntity) // skip sender of message
			continue;

		if (teamonly && sender_team != UTIL_GetTeam(client))
			continue;

		MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, client);
		WRITE_BYTE(ENTINDEX(pEntity));
		// if(mod_id == FRONTLINE_DLL)
		//    WRITE_SHORT(0);
		WRITE_STRING(text);
		MESSAGE_END();
	}

	// print to the sending client
	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pEntity);
	WRITE_BYTE(ENTINDEX(pEntity));
	// if(mod_id == FRONTLINE_DLL)
	//    WRITE_SHORT(0);
	WRITE_STRING(text);
	MESSAGE_END();

	if (IS_DEDICATED_SERVER())
		std::printf("%s", text); // print bot message on dedicated server
}

#ifdef DEBUG
edict_t* DBG_EntOfVars(const entvars_t* pev) {
	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;

	ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine");
	edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
	if (pent == NULL)
		ALERT(at_console, "DAMN!  Even the engine couldn't FindEntityByVars!");
	((entvars_t*)pev)->pContainingEntity = pent;
	return pent;
}
#endif // DEBUG

int UTIL_GetTeamColor(edict_t* pEntity) {
	// crash check here
	if (!pEntity)
		return -1;

	if (mod_id == TFC_DLL) {
		char topcolor[32];

		const char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEntity);
		std::strcpy(topcolor, g_engfuncs.pfnInfoKeyValue(infobuffer, "topcolor"));

		// used for spy checking
		if (std::strcmp(topcolor, "150") == 0 || std::strcmp(topcolor, "153") == 0 || std::strcmp(topcolor, "148") == 0 || std::strcmp(topcolor, "140") == 0)
			return 0; // blue
		if (std::strcmp(topcolor, "250") == 0 || std::strcmp(topcolor, "255") == 0 || std::strcmp(topcolor, "5") == 0)
			return 1; // red
		if (std::strcmp(topcolor, "45") == 0)
			return 2; // yellow
		if (std::strcmp(topcolor, "100") == 0 || std::strcmp(topcolor, "80") == 0)
			return 3; // green

		return pEntity->v.team - 1; // TFC teams are 1-4 based
	}
	return -1;
}

// This function returns team number 0s through 3 based on what the MOD
// uses for team numbers.
// You can call this function to find the team of players and Sentry Guns.
int UTIL_GetTeam(const edict_t* pEntity) {
	// crash check
	if (!pEntity)
		return -1;

	if (mod_id == TFC_DLL) {
		// Check if this entity is a player and return the team number
		// of that player
		if (pEntity->v.team - 1 > -1)
			return pEntity->v.team - 1; // TFC teams are 0-3 based

		// the team number was invalid, check if this entity is a
		// Sentry Gun with a colormap, and report its team
		if (pEntity->v.colormap == 0xA096)
			return 0; // blue team's sentry
		if (pEntity->v.colormap == 0x04FA)
			return 1; // red team's sentry
		if (pEntity->v.colormap == 0x372D)
			return 2; // yellow team's sentry
		if (pEntity->v.colormap == 0x6E64)
			return 3; // green team's sentry

		// Unrecognised colormap, report a total failure
		return -1;
	}
	/*	else  // must be HL or OpFor deathmatch...
					{
									char *infobuffer;
									char model_name[32];

									if(team_names[0][0] == 0)
									{
													char *pName;
													char teamlist[MAX_TEAMS*MAX_TEAMNAME_LENGTH];
													int i;

													num_teams = 0;
													std::strcpy(teamlist, CVAR_GET_STRING("mp_teamlist"));
													pName = teamlist;
													pName = strtok(pName, ";");

													while(pName != NULL && *pName)
													{
																	// check that team isn't defined twice
																	for(i=0; i < num_teams; i++)
																					if(std::strcmp(pName, team_names[i]) == 0)
																									break;
																	if(i == num_teams)
																	{
																					std::strcpy(team_names[num_teams], pName);
																					num_teams++;
																	}
																	pName = strtok(NULL, ";");
													}
									}

									infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEntity );
									std::strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

									for(int index=0; index < num_teams; index++)
									{
													if(std::strcmp(model_name, team_names[index]) == 0)
																	return index;
									}
					}*/

	return 0;
}

// Give this function a pointer to a flag entity and it will return which
// team the flag belongs to(0 - 3).
// This function may be only appropriate for TFC flags.
// It returns -1 if the flag is neutral or on failure to recognise the team color.
int UTIL_GetFlagsTeam(const edict_t* flag_edict) {
	switch (flag_edict->v.skin) {
	case 1: // team skin 1 is Red(confusingly)
		return 1;
	case 2: // team skin 2 is Blue(confusingly)
		return 0;
	case 3: // team skin 3 is probably Yellow(not tested this out though)
		return 2;
	case 4: // team skin 4 is probably Green(not tested this out though)
		return 3;
	default:
		return -1; // neutral flags have skin 0
	}
}

// return class number 0 through N
int UTIL_GetClass(edict_t* pEntity) {
	char model_name[32];

	const char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEntity);
	std::strcpy(model_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "model"));

	return 0;
}

int UTIL_GetBotIndex(const edict_t* pEdict) {
   for (int index = 0; index < MAX_BOTS; index++) {
		if (bots[index].pEdict == pEdict)
			return index;
	}

	// return -1 if edict is not a bot
	return -1;
}

bot_t* UTIL_GetBotPointer(const edict_t* pEdict) {
	for (bot_t &bot : bots) {
		if (bot.pEdict == pEdict)
			return &bot;
	}

	return nullptr; // return NULL if edict is not a bot
}

bool IsAlive(const edict_t* pEdict) { return pEdict->v.deadflag == DEAD_NO && pEdict->v.health > 0 && !(pEdict->v.flags & FL_NOTARGET) && pEdict->v.movetype != MOVETYPE_NOCLIP; }

// Returns true if the specified entity can see the specified vector in
// its field of view.
bool FInViewCone(const Vector& r_pOrigin, const edict_t* pEdict) {
	UTIL_MakeVectors(pEdict->v.angles);

	Vector2D vec2LOS = (r_pOrigin - pEdict->v.origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	const float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > 0.50f) // 60 degree field of view
		return true;
	return false;
}

// This function is a variant of FInViewCone().  It returns a measure of
// how much the entities view is facing away from the given origin.
// 0.99999 means the bot is looking right at the given origin.
// The lower the number the more the bot is looking away from it.
float BotViewAngleDiff(const Vector& r_pOrigin, const edict_t* pEdict) {
	UTIL_MakeVectors(pEdict->v.angles);

	Vector2D vec2LOS = (r_pOrigin - pEdict->v.origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	return DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());
}

bool BotCanSeeOrigin(const bot_t* pBot, const Vector& r_dest) {
	TraceResult tr;

	// trace a line from bot's eyes to destination...
	UTIL_TraceLine(pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs, r_dest, ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

	// check if line of sight to the object is not blocked
	if (tr.flFraction >= 1.0f)
		return true;

	return false;
}

// BotInFieldOfView - This function returns the absolute value of angle
// to the destination.  Zero degrees means the destination is straight
// ahead, 45 degrees to the left or 45 degrees to the right
// is the limit of the normal view angle.
int BotInFieldOfView(const bot_t* pBot, const Vector& dest) {
	// find angles from source to destination...
	Vector entity_angles = UTIL_VecToAngles(dest);

	// make yaw angle 0 to 360 degrees if negative...
	if (entity_angles.y < 0.0f)
		entity_angles.y += 360.0f;

	// get bot's current view angle...
	float view_angle = pBot->pEdict->v.v_angle.y;

	// make view angle 0 to 360 degrees if negative...
	if (view_angle < 0.0f)
		view_angle += 360.0f;

	// rsm - START angle bug fix
	float angle = std::fabs(std::round(view_angle) - std::round(entity_angles.y));

	if (angle > 180.0f)
		angle = 360.0f - angle;

	return static_cast<int>(angle);
	// rsm - END
}

bool FVisible(const Vector& r_vecOrigin, edict_t* pEdict) {
	// look through caller's eyes
	const Vector vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	const int bInWater = UTIL_PointContents(r_vecOrigin) == CONTENTS_WATER;
	const int bLookerInWater = UTIL_PointContents(vecLookerOrigin) == CONTENTS_WATER;

	// don't look through surface of water
	if (bInWater != bLookerInWater)
		return false;

	TraceResult tr;

	// was ignore glass
	UTIL_TraceLine(vecLookerOrigin, r_vecOrigin, ignore_monsters, dont_ignore_glass, pEdict, &tr);

	if (tr.flFraction < 1.0f)
		return false; // Line of sight is not established
	return true;
	// line of sight is valid.
}

Vector GetGunPosition(const edict_t* pEdict) { return pEdict->v.origin + pEdict->v.view_ofs; }

void UTIL_SelectItem(edict_t* pEdict, const char* item_name) {
	// UTIL_HostSay(pEdict, 0, item_name);
	FakeClientCommand(pEdict, item_name, nullptr, nullptr);
}

// Some entities have a Vector of (0, 0, 0), so you need this function.
Vector VecBModelOrigin(const edict_t* pEdict) { return pEdict->v.absmin + pEdict->v.size * 0.5; }

// This function checks if footstep sounds are on and if the indicated player
// is moving fast enough to make footstep sounds.
bool UTIL_FootstepsHeard(const edict_t* pEdict, const edict_t* pPlayer) {
	static bool check_footstep_sounds = true;
	static bool footstep_sounds_on = false;

	if (check_footstep_sounds == true) {
		check_footstep_sounds = false;

		if (CVAR_GET_FLOAT("mp_footsteps") > 0.0f)
			footstep_sounds_on = true;
	}

	// update sounds made by this player, alert bots if they are nearby...
	if (footstep_sounds_on == true) {
		// check if this player is near enough and moving fast enough on
		// the ground to make sounds
		if (pPlayer->v.flags & FL_ONGROUND && VectorsNearerThan(pPlayer->v.origin, pEdict->v.origin, 600.0) && pPlayer->v.velocity.Length2D() > 220.0f) {
			return true;
		}
	}

	return false;
}

void UTIL_ShowMenu(edict_t* pEdict, const int slots, const int displaytime, const bool needmore, const char* pText) {
	if (gmsgShowMenu == 0)
		gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, nullptr, pEdict);

	WRITE_SHORT(slots);
	WRITE_CHAR(displaytime);
	WRITE_BYTE(needmore);
	WRITE_STRING(pText);

	MESSAGE_END();
}

// This function will try to create a new log file in the main Foxbot
// directory on it's first call.
// Then it will open the log file for appending thereafter.
// NOTE: Writing to a NULL file pointer can crash the server.
std::FILE* UTIL_OpenFoxbotLog() {
	static bool log_creation_attempted = false;

	UTIL_FindFoxbotPath();

	std::FILE* file_ptr;
	if (log_creation_attempted) {
		// append to the existing log file
		file_ptr = std::fopen(foxbot_logname, "a");
	}
	else // it's time to create a new log from scratch
	{
		// rename the last log as old if it exists
		file_ptr = std::fopen(foxbot_logname, "r");
		if (file_ptr != nullptr) {
			std::fclose(file_ptr);

			char old_logname[160];
			std::strcpy(old_logname, foxbot_logname);
			std::strncat(old_logname, ".old", 159 - std::strlen(old_logname)); // give it a suffix
			old_logname[159] = '\0';
			std::remove(old_logname);                 // delete the last old log file
			std::rename(foxbot_logname, old_logname); // good ol' ANSI C
		}

		// create a new log file
		file_ptr = std::fopen(foxbot_logname, "w");

		// warn the player if the log file couldn't be created
		if (file_ptr == nullptr) {
			if (IS_DEDICATED_SERVER())
				std::printf("\nWARNING: Couldn't create log file: foxbot.log\n");
			else
				ALERT(at_console, "\nWARNING: Couldn't create log file: foxbot.log\n");
		}

		log_creation_attempted = true;
	}

	return file_ptr;
}

// This function is a variant of UTIL_LogPrintf() as used in the SDK.
// It lets you print messages straight to the Foxbot log file.
void UTIL_BotLogPrintf(const char* fmt, ...) {
	std::FILE* lfp = UTIL_OpenFoxbotLog();
	if (lfp == nullptr)
		return;

	va_list argptr;
	static char string[1024]; //String fix from RCBot

	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	std::fclose(lfp);
}

// This function tries to find out where the Foxbot directory is and
// attempts to piece together the path and name of the specified file
// and/or directory.
void UTIL_BuildFileName(char* filename, const int max_fn_length, const char* arg1, const char* arg2) {
	filename[0] = '\0';

	UTIL_FindFoxbotPath();

	// only support TFC, as it's what Foxbot does best
	if (mod_id != TFC_DLL)
		return;

	// add the foxbot directory path, unless it is not valid
	if (std::strcmp(foxbot_path, "") != 0 && std::strlen(foxbot_path) < static_cast<unsigned>(max_fn_length)) {
		std::strncpy(filename, foxbot_path, max_fn_length);
		filename[max_fn_length - 1] = '\0';
	}
	else
		return;

	// add on the directory and or filename
	if (arg1 && *arg1 && arg2 && *arg2) {
		std::strcat(filename, arg1);

#ifndef __linux__
		std::strcat(filename, "\\");
#else
		std::strcat(filename, "/");
#endif

		std::strcat(filename, arg2);
	}
	else if (arg1 && *arg1) {
		std::strcat(filename, arg1);
	}

	filename[max_fn_length - 1] = '\0'; // just to be sure
}

// This function tries to find the Foxbot directory path by testing for
// the existence of foxbot.cfg in expected relative locations.
// e.g. /tfc/addons/  or the main Half-Life directory.
static void UTIL_FindFoxbotPath() {
	// only do this test once
	static bool dir_path_checked = false;
	if (dir_path_checked)
		return;
	dir_path_checked = true;

	// find out where the foxbot directory is, by trying to open and
	 // close the foxbot.cfg file just once
#ifndef __linux__ // must be a Windows machine
	if (std::strcmp(foxbot_path, "") == 0) {
		// try the addons directory first(for Foxbot 0.76 and newer)
		std::FILE* fptr = std::fopen(R"(tfc\addons\foxbot\tfc\foxbot.cfg)", "r");
		if (fptr != nullptr) {
			std::strcpy(foxbot_path, R"(tfc\addons\foxbot\tfc\)");
			std::strcpy(foxbot_logname, R"(tfc\addons\foxbot\foxbot.log)");
			std::fclose(fptr);
		}
		else // try the older directory location(Foxbot 0.75 and older)
		{
			fptr = std::fopen("foxbot\\tfc\\foxbot.cfg", "r");
			if (fptr != nullptr) {
				std::strcpy(foxbot_path, "foxbot\\tfc\\");
				std::strcpy(foxbot_logname, "foxbot\\foxbot.log");
				std::fclose(fptr);
			}
		}
	}
#else // must be a Linux machine
	if (std::strcmp(foxbot_path, "") == 0) {
		// try the addons directory first(for Foxbot 0.76 and newer)
		std::FILE* fptr = std::fopen("tfc/addons/foxbot/tfc/foxbot.cfg", "r");
		if (fptr != nullptr) {
			std::strcpy(foxbot_path, "tfc/addons/foxbot/tfc/");
			std::strcpy(foxbot_logname, "tfc/addons/foxbot/foxbot.log");
			std::fclose(fptr);
		}
		else // try the older directory location(Foxbot 0.75 and older)
		{
			fptr = std::fopen("foxbot/tfc/foxbot.cfg", "r");
			if (fptr != nullptr) {
				std::strcpy(foxbot_path, "foxbot/tfc/");
				std::strcpy(foxbot_logname, "foxbot/foxbot.log");
				std::fclose(fptr);
			}
		}
	}
#endif

	// report a problem if the Foxbot directory wasn't found
	if (std::strcmp(foxbot_path, "") == 0) {
		if (IS_DEDICATED_SERVER()) {
			std::printf("\nfoxbot.cfg should be in the \\foxbot\\tfc\\ directory\n");
			std::printf("--Check your Foxbot installation is correct--\n\n");
		}
		else {
			ALERT(at_console, "\nfoxbot.cfg should be in the \\foxbot\\tfc\\ directory\n");
			ALERT(at_console, "--Check your Foxbot installation is correct--\n\n");
		}
	}
}

// This function is a sort of wrapper around fgets().
// Unlike fgets() it always attempts to find the end of the current line
// in the file.
// It also makes sure that the string is null terminated on success.
// It returns false if fgets() returned NULL.
bool UTIL_ReadFileLine(char* string, const int max_length, FILE* file_ptr) {
	bool line_end_found = false;

	if (fgets(string, max_length, file_ptr) == nullptr)
		return false;

	// check if the string read contains a line terminator of some sort
	for (int a = 0; a < max_length; a++) {
		if (string[a] == '\n' || string[a] == '\r')
			line_end_found = true;
	}

	// if the end of the current line in the file was not found,
	// go look for it
	if (!line_end_found) {
		//std::printf("finding line end\n");
		int c;
		do {
			c = fgetc(file_ptr);
		} while (c != '\n' && c != '\r' && c != EOF);
	}
	else /* make sure the string is null terminated */
		string[max_length - 1] = '\0';

	return true;
}