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

#include "ctype.h"
#include "extdll.h"
#include <time.h>
#include <util.h>
#include "engine.h"

#include <math.h>

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

static unsigned long lseed = static_cast<long>(time(0));

// FUNCTION PROTOTYPES
static void UTIL_FindFoxbotPath(void);

#ifdef __BORLANDC__
// The Borland 5.0 compiler doesn't ignore maths errors(unlike a few other compilers).
// So here's a maths error handler to cope with problems such as: sqrt(-9).
int _RTLENTRY _EXPFUNC _matherr(struct _exception* except_ptr)
{
    static bool errorReported = FALSE;

    // make one log report about the maths error reported
    if(!errorReported) {
        errorReported = TRUE;

        if(except_ptr->name != NULL) // just being neurotic
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
long random_long(const long lowval, const long highval)
{
    const long spread = (highval - lowval) + 1;

    // make sure a sensible number was requested
    if(spread < 2) {
        //	UTIL_BotLogPrintf("dumb random request: %d, %d\n", lowval, highval);
        return lowval;
    }

    lseed = (lseed * 1103515245 + 12345) % 2147483647;

    return ((lseed / 3) % spread) + lowval;
}

//	This is a float version of rand().
//	This function seems to give good variation even with smaller
//	number spreads.
float random_float(const float lowval, const float highval)
{
    if(highval <= lowval)
        return lowval;

    lseed = (lseed * 1103515245 + 12345) % 2147483647;

    return (lowval + (float)lseed / ((float)LONG_MAX / (highval - lowval)));
}

// This function is a quick simple way of testing the distance between two
// vectors against some distance value.
// This should be faster than using the Vector classes .Length() function
// because it requires no function call to sqrt().
bool VectorsNearerThan(const Vector& r_vOne, const Vector& r_vTwo, double value)
{
    value = value * value;

    Vector distance = r_vOne - r_vTwo;
    double temp = (distance.x * distance.x) + (distance.y * distance.y);

    // perform an early 2 dimensional check, because most maps
    // are wider/longer than they are tall
    if(temp > value)
        return FALSE;
    else
        temp += (distance.z * distance.z);

    // final check(3 dimensional)
    if(temp < value)
        return TRUE;

    return FALSE;
}

#if 0 // this function is unused so far
int UTIL_SentryLevel ( edict_t *pEntity )
{
	char *model = (char*)STRING(pEntity->v.model);

	if( model[13] == '1' )
		return 1;
	else if( model[13] == '2' )
		return 2;
	else if( model[13] == '3' )
		return 3;

	return 0;
}
#endif

Vector UTIL_VecToAngles(const Vector& vec)
{
    float rgflVecOut[3];
    VEC_TO_ANGLES(vec, rgflVecOut);
    return Vector(rgflVecOut);
}

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine(const Vector& vecStart,
    const Vector& vecEnd,
    IGNORE_MONSTERS igmon,
    IGNORE_GLASS ignoreGlass,
    edict_t* pentIgnore,
    TraceResult* ptr)
{
    TRACE_LINE(
        vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass ? 0x100 : 0), pentIgnore, ptr);
}

void UTIL_TraceLine(const Vector& vecStart,
    const Vector& vecEnd,
    IGNORE_MONSTERS igmon,
    edict_t* pentIgnore,
    TraceResult* ptr)
{
    TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr);
}

void UTIL_MakeVectors(const Vector& vecAngles)
{
    MAKE_VECTORS(vecAngles);
}

edict_t* UTIL_FindEntityInSphere(edict_t* pentStart, const Vector& vecCenter, float flRadius)
{
    edict_t* pentEntity = FIND_ENTITY_IN_SPHERE(pentStart, vecCenter, flRadius);

    if(!FNullEnt(pentEntity))
        return pentEntity;

    return NULL;
}

edict_t* UTIL_FindEntityByString(edict_t* pentStart, const char* szKeyword, const char* szValue)
{
    edict_t* pentEntity = FIND_ENTITY_BY_STRING(pentStart, szKeyword, szValue);

    if(!FNullEnt(pentEntity))
        return pentEntity;
    return NULL;
}

edict_t* UTIL_FindEntityByClassname(edict_t* pentStart, const char* szName)
{
    return UTIL_FindEntityByString(pentStart, "classname", szName);
}

edict_t* UTIL_FindEntityByTargetname(edict_t* pentStart, const char* szName)
{
    return UTIL_FindEntityByString(pentStart, "targetname", szName);
}

int UTIL_PointContents(const Vector& vec)
{
    return POINT_CONTENTS(vec);
}

void UTIL_SetSize(entvars_t* pev, const Vector& vecMin, const Vector& vecMax)
{
    SET_SIZE(ENT(pev), vecMin, vecMax);
}

void UTIL_SetOrigin(entvars_t* pev, const Vector& vecOrigin)
{
    SET_ORIGIN(ENT(pev), vecOrigin);
}

void HUDNotify(edict_t* pEntity, const char* msg_name)
{
    if(gmsgHUDNotify == 0)
        gmsgHUDNotify = REG_USER_MSG("HudText", -1);

    MESSAGE_BEGIN(MSG_ONE, gmsgHUDNotify, NULL, pEntity);
    WRITE_STRING(msg_name);
    MESSAGE_END();
}

void ClientPrint(edict_t* pEntity, int msg_dest, const char* msg_name)
{
    if(gmsgTextMsg == 0)
        gmsgTextMsg = REG_USER_MSG("TextMsg", -1);

    MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, NULL, pEntity);

    WRITE_BYTE(msg_dest);
    WRITE_STRING(msg_name);
    MESSAGE_END();
}

void UTIL_SayText(const char* pText, edict_t* pEdict)
{
    if(gmsgSayText == 0)
        gmsgSayText = REG_USER_MSG("SayText", -1);

    MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pEdict);
    WRITE_BYTE(ENTINDEX(pEdict));
    // if(mod_id == FRONTLINE_DLL)
    //    WRITE_SHORT(0);
    WRITE_STRING(pText);
    MESSAGE_END();
}

void UTIL_HostSay(edict_t* pEntity, const int teamonly, char* message)
{
    char* pc;

    // make sure the text has content
    for(pc = message; pc != NULL && *pc != 0; pc++) {
        if(isprint(*pc) && !isspace(*pc)) {
            // we've found an alphanumeric character, so text is valid
            pc = NULL;
            break;
        }
    }

    if(pc != NULL)
        return; // no character found, so say nothing

    char text[128];

    // turn on color set 2  (color on,  no sound)
    if(teamonly)
        _snprintf(text, 127, "%c(TEAM) %s: %s\n", 2, STRING(pEntity->v.netname), message);
    else
        _snprintf(text, 127, "%c%s: %s\n", 2, STRING(pEntity->v.netname), message);

    text[127] = '\0';

    // loop through all players, starting with the first player.
    // This may return the world in single player if the client types
    // something between levels or during spawn
    // so check it, or it will infinite loop

    if(gmsgSayText == 0)
        gmsgSayText = REG_USER_MSG("SayText", -1);

    int sender_team = UTIL_GetTeam(pEntity);

    edict_t* client = NULL;
    while(((client = UTIL_FindEntityByClassname(client, "player")) != NULL) && (!FNullEnt(client))) {
        if(client == pEntity) // skip sender of message
            continue;

        if(teamonly && (sender_team != UTIL_GetTeam(client)))
            continue;

        MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, client);
        WRITE_BYTE(ENTINDEX(pEntity));
        // if(mod_id == FRONTLINE_DLL)
        //    WRITE_SHORT(0);
        WRITE_STRING(text);
        MESSAGE_END();
    }

    // print to the sending client
    MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pEntity);
    WRITE_BYTE(ENTINDEX(pEntity));
    // if(mod_id == FRONTLINE_DLL)
    //    WRITE_SHORT(0);
    WRITE_STRING(text);
    MESSAGE_END();

    if(IS_DEDICATED_SERVER())
        printf(text); // print bot message on dedicated server
}

#ifdef DEBUG
edict_t* DBG_EntOfVars(const entvars_t* pev)
{
    if(pev->pContainingEntity != NULL)
        return pev->pContainingEntity;

    ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine");
    edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
    if(pent == NULL)
        ALERT(at_console, "DAMN!  Even the engine couldn't FindEntityByVars!");
    ((entvars_t*)pev)->pContainingEntity = pent;
    return pent;
}
#endif // DEBUG

int UTIL_GetTeamColor(edict_t* pEntity)
{
    // crash check here
    if(!pEntity)
        return -1;

    if(mod_id == TFC_DLL) {
        char* infobuffer;
        char topcolor[32];

        infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEntity);
        strcpy(topcolor, (g_engfuncs.pfnInfoKeyValue(infobuffer, "topcolor")));

        // used for spy checking
        if(strcmp(topcolor, "150") == 0 || strcmp(topcolor, "153") == 0 || strcmp(topcolor, "148") == 0 ||
            strcmp(topcolor, "140") == 0)
            return 0; // blue
        if(strcmp(topcolor, "250") == 0 || strcmp(topcolor, "255") == 0 || strcmp(topcolor, "5") == 0)
            return 1; // red
        if(strcmp(topcolor, "45") == 0)
            return 2; // yellow
        if(strcmp(topcolor, "100") == 0 || strcmp(topcolor, "80") == 0)
            return 3; // green

        return pEntity->v.team - 1; // TFC teams are 1-4 based
    }
    return -1;
}

// This function returns team number 0s through 3 based on what the MOD
// uses for team numbers.
// You can call this function to find the team of players and Sentry Guns.
int UTIL_GetTeam(edict_t* pEntity)
{
    // crash check
    if(!pEntity)
        return -1;

    if(mod_id == TFC_DLL) {
        // Check if this entity is a player and return the team number
        // of that player
        if((pEntity->v.team - 1) > -1)
            return pEntity->v.team - 1; // TFC teams are 0-3 based

        // the team number was invalid, check if this entity is a
        // Sentry Gun with a colormap, and report its team
        if(pEntity->v.colormap == 0xA096)
            return 0; // blue team's sentry
        if(pEntity->v.colormap == 0x04FA)
            return 1; // red team's sentry
        if(pEntity->v.colormap == 0x372D)
            return 2; // yellow team's sentry
        if(pEntity->v.colormap == 0x6E64)
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
                            strcpy(teamlist, CVAR_GET_STRING("mp_teamlist"));
                            pName = teamlist;
                            pName = strtok(pName, ";");

                            while(pName != NULL && *pName)
                            {
                                    // check that team isn't defined twice
                                    for(i=0; i < num_teams; i++)
                                            if(strcmp(pName, team_names[i]) == 0)
                                                    break;
                                    if(i == num_teams)
                                    {
                                            strcpy(team_names[num_teams], pName);
                                            num_teams++;
                                    }
                                    pName = strtok(NULL, ";");
                            }
                    }

                    infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEntity );
                    strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

                    for(int index=0; index < num_teams; index++)
                    {
                            if(strcmp(model_name, team_names[index]) == 0)
                                    return index;
                    }
            }*/

    return 0;
}

// Give this function a pointer to a flag entity and it will return which
// team the flag belongs to(0 - 3).
// This function may be only appropriate for TFC flags.
// It returns -1 if the flag is neutral or on failure to recognise the team color.
int UTIL_GetFlagsTeam(const edict_t* flag_edict)
{
    switch(flag_edict->v.skin) {
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
int UTIL_GetClass(edict_t* pEntity)
{
    char* infobuffer;
    char model_name[32];

    infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEntity);
    strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

    return 0;
}

int UTIL_GetBotIndex(edict_t* pEdict)
{
    for(int index = 0; index < 32; index++) {
        if(bots[index].pEdict == pEdict)
            return index;
    }

    // return -1 if edict is not a bot
    return -1;
}

bot_t* UTIL_GetBotPointer(edict_t* pEdict)
{
    for(int index = 0; index < 32; index++) {
        if(bots[index].pEdict == pEdict)
            return (&bots[index]);
    }

    return NULL; // return NULL if edict is not a bot
}

bool IsAlive(edict_t* pEdict)
{
    return (pEdict->v.deadflag == DEAD_NO && pEdict->v.health > 0 && !(pEdict->v.flags & FL_NOTARGET) &&
        pEdict->v.movetype != MOVETYPE_NOCLIP);
}

// Returns true if the specified entity can see the specified vector in
// its field of view.
bool FInViewCone(const Vector& r_pOrigin, const edict_t* pEdict)
{
    UTIL_MakeVectors(pEdict->v.angles);

    Vector2D vec2LOS = (r_pOrigin - pEdict->v.origin).Make2D();
    vec2LOS = vec2LOS.Normalize();

    const float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

    if(flDot > 0.50) // 60 degree field of view
        return TRUE;
    else
        return FALSE;
}

// This function is a variant of FInViewCone().  It returns a measure of
// how much the entities view is facing away from the given origin.
// 0.99999 means the bot is looking right at the given origin.
// The lower the number the more the bot is looking away from it.
float BotViewAngleDiff(Vector& r_pOrigin, const edict_t* pEdict)
{
    UTIL_MakeVectors(pEdict->v.angles);

    Vector2D vec2LOS = (r_pOrigin - pEdict->v.origin).Make2D();
    vec2LOS = vec2LOS.Normalize();

    return DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());
}

bool BotCanSeeOrigin(bot_t* pBot, Vector& r_dest)
{
    TraceResult tr;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs, r_dest, ignore_monsters,
        pBot->pEdict->v.pContainingEntity, &tr);

    // check if line of sight to the object is not blocked
    if(tr.flFraction >= 1.0)
        return TRUE;

    return FALSE;
}

// BotInFieldOfView - This function returns the absolute value of angle
// to the destination.  Zero degrees means the destination is straight
// ahead, 45 degrees to the left or 45 degrees to the right
// is the limit of the normal view angle.
int BotInFieldOfView(bot_t* pBot, Vector dest)
{
    // find angles from source to destination...
    Vector entity_angles = UTIL_VecToAngles(dest);

    // make yaw angle 0 to 360 degrees if negative...
    if(entity_angles.y < 0.0)
        entity_angles.y += 360.0;

    // get bot's current view angle...
    float view_angle = pBot->pEdict->v.v_angle.y;

    // make view angle 0 to 360 degrees if negative...
    if(view_angle < 0.0)
        view_angle += 360.0;

    // rsm - START angle bug fix
    int angle = abs(static_cast<int>(view_angle) - static_cast<int>(entity_angles.y));

    if(angle > 180)
        angle = 360 - angle;

    return angle;
    // rsm - END
}

bool FVisible(const Vector& r_vecOrigin, edict_t* pEdict)
{
    // look through caller's eyes
    Vector vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

    int bInWater = (UTIL_PointContents(r_vecOrigin) == CONTENTS_WATER);
    int bLookerInWater = (UTIL_PointContents(vecLookerOrigin) == CONTENTS_WATER);

    // don't look through surface of water
    if(bInWater != bLookerInWater)
        return FALSE;

    TraceResult tr;

    // was ignore glass
    UTIL_TraceLine(vecLookerOrigin, r_vecOrigin, ignore_monsters, dont_ignore_glass, pEdict, &tr);

    if(tr.flFraction < 1.0)
        return FALSE; // Line of sight is not established
    else
        return TRUE; // line of sight is valid.
}

Vector GetGunPosition(const edict_t* pEdict)
{
    return (pEdict->v.origin + pEdict->v.view_ofs);
}

void UTIL_SelectItem(edict_t* pEdict, char* item_name)
{
    // UTIL_HostSay(pEdict, 0, item_name);
    FakeClientCommand(pEdict, item_name, NULL, NULL);
}

// Some entities have a Vector of (0, 0, 0), so you need this function.
Vector VecBModelOrigin(edict_t* pEdict)
{
    return pEdict->v.absmin + (pEdict->v.size * 0.5);
}

// This function checks if footstep sounds are on and if the indicated player
// is moving fast enough to make footstep sounds.
bool UTIL_FootstepsHeard(edict_t* pEdict, edict_t* pPlayer)
{
    static bool check_footstep_sounds = TRUE;
    static bool footstep_sounds_on = FALSE;

    if(check_footstep_sounds == TRUE) {
        check_footstep_sounds = FALSE;

        if(CVAR_GET_FLOAT("mp_footsteps") > 0.0)
            footstep_sounds_on = TRUE;
    }

    // update sounds made by this player, alert bots if they are nearby...
    if(footstep_sounds_on == TRUE) {
        // check if this player is near enough and moving fast enough on
        // the ground to make sounds
        if(pPlayer->v.flags & FL_ONGROUND && VectorsNearerThan(pPlayer->v.origin, pEdict->v.origin, 600.0) &&
            pPlayer->v.velocity.Length2D() > 220.0) {
            return TRUE;
        }
    }

    return FALSE;
}

void UTIL_ShowMenu(edict_t* pEdict, int slots, int displaytime, bool needmore, char* pText)
{
    if(gmsgShowMenu == 0)
        gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);

    MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pEdict);

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
FILE* UTIL_OpenFoxbotLog(void)
{
    static bool log_creation_attempted = FALSE;

    UTIL_FindFoxbotPath();

    FILE* file_ptr;
    if(log_creation_attempted) {
        // append to the existing log file
        file_ptr = fopen(foxbot_logname, "a");
    } else // it's time to create a new log from scratch
    {
        // rename the last log as old if it exists
        file_ptr = fopen(foxbot_logname, "r");
        if(file_ptr != NULL) {
            fclose(file_ptr);

            char old_logname[160];
            strcpy(old_logname, foxbot_logname);
            strncat(old_logname, ".old", 159 - strlen(old_logname)); // give it a suffix
            old_logname[159] = '\0';
            remove(old_logname);                 // delete the last old log file
            rename(foxbot_logname, old_logname); // good ol' ANSI C
        }

        // create a new log file
        file_ptr = fopen(foxbot_logname, "w");

        // warn the player if the log file couldn't be created
        if(file_ptr == NULL) {
            if(IS_DEDICATED_SERVER())
                printf("\nWARNING: Couldn't create log file: foxbot.log\n");
            else
                ALERT(at_console, "\nWARNING: Couldn't create log file: foxbot.log\n");
        }

        log_creation_attempted = TRUE;
    }

    return file_ptr;
}

// This function is a variant of UTIL_LogPrintf() as used in the SDK.
// It lets you print messages straight to the Foxbot log file.
void UTIL_BotLogPrintf(char* fmt, ...)
{
    FILE* lfp = UTIL_OpenFoxbotLog();
    if(lfp == NULL)
        return;

    va_list argptr;

    va_start(argptr, fmt);
    vfprintf(lfp, fmt, argptr);
    va_end(argptr);

    fclose(lfp);
}

// This function tries to find out where the Foxbot directory is and
// attempts to piece together the path and name of the specified file
// and/or directory.
void UTIL_BuildFileName(char* filename, const int max_fn_length, char* arg1, char* arg2)
{
    filename[0] = '\0';

    UTIL_FindFoxbotPath();

    // only support TFC, as it's what Foxbot does best
    if(mod_id != TFC_DLL)
        return;

    // add the foxbot directory path, unless it is not valid
    if(strcmp(foxbot_path, "") != 0 && strlen(foxbot_path) < (unsigned)max_fn_length) {
        strncpy(filename, foxbot_path, max_fn_length);
        filename[max_fn_length - 1] = '\0';
    } else
        return;

    // add on the directory and or filename
    if((arg1) && (*arg1) && (arg2) && (*arg2)) {
        strcat(filename, arg1);

#ifndef __linux__
        strcat(filename, "\\");
#else
        strcat(filename, "/");
#endif

        strcat(filename, arg2);
    } else if((arg1) && (*arg1)) {
        strcat(filename, arg1);
    }

    filename[max_fn_length - 1] = '\0'; // just to be sure
}

// This function tries to find the Foxbot directory path by testing for
// the existence of foxbot.cfg in expected relative locations.
// e.g. /tfc/addons/  or the main Half-Life directory.
static void UTIL_FindFoxbotPath(void)
{
    // only do this test once
    static bool dir_path_checked = FALSE;
    if(dir_path_checked)
        return;
    else
        dir_path_checked = TRUE;

// find out where the foxbot directory is, by trying to open and
// close the foxbot.cfg file just once
#ifndef __linux__ // must be a Windows machine
    if(strcmp(foxbot_path, "") == 0) {
        // try the addons directory first(for Foxbot 0.76 and newer)
        FILE* fptr = fopen("tfc\\addons\\foxbot\\tfc\\foxbot.cfg", "r");
        if(fptr != NULL) {
            strcpy(foxbot_path, "tfc\\addons\\foxbot\\tfc\\");
            strcpy(foxbot_logname, "tfc\\addons\\foxbot\\foxbot.log");
            fclose(fptr);
        } else // try the older directory location(Foxbot 0.75 and older)
        {
            fptr = fopen("foxbot\\tfc\\foxbot.cfg", "r");
            if(fptr != NULL) {
                strcpy(foxbot_path, "foxbot\\tfc\\");
                strcpy(foxbot_logname, "foxbot\\foxbot.log");
                fclose(fptr);
            }
        }
    }
#else // must be a Linux machine
    if(strcmp(foxbot_path, "") == 0) {
        // try the addons directory first(for Foxbot 0.76 and newer)
        FILE* fptr = fopen("tfc/addons/foxbot/tfc/foxbot.cfg", "r");
        if(fptr != NULL) {
            strcpy(foxbot_path, "tfc/addons/foxbot/tfc/");
            strcpy(foxbot_logname, "tfc/addons/foxbot/foxbot.log");
            fclose(fptr);
        } else // try the older directory location(Foxbot 0.75 and older)
        {
            fptr = fopen("foxbot/tfc/foxbot.cfg", "r");
            if(fptr != NULL) {
                strcpy(foxbot_path, "foxbot/tfc/");
                strcpy(foxbot_logname, "foxbot/foxbot.log");
                fclose(fptr);
            }
        }
    }
#endif

    // report a problem if the Foxbot directory wasn't found
    if(strcmp(foxbot_path, "") == 0) {
        if(IS_DEDICATED_SERVER()) {
            printf("\nfoxbot.cfg should be in the \\foxbot\\tfc\\ directory\n");
            printf("--Check your Foxbot installation is correct--\n\n");
        } else {
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
bool UTIL_ReadFileLine(char* string, const unsigned int max_length, FILE* file_ptr)
{
    unsigned int a;
    bool line_end_found = FALSE;

    if(fgets(string, max_length, file_ptr) == NULL)
        return FALSE;

    // check if the string read contains a line terminator of some sort
    for(a = 0; a < max_length; a++) {
        if(string[a] == '\n' || string[a] == '\r')
            line_end_found = TRUE;
    }

    // if the end of the current line in the file was not found,
    // go look for it
    if(!line_end_found) {
        // printf("finding line end\n");
        int c;
        do {
            c = fgetc(file_ptr);
        } while(c != '\n' && c != '\r' && c != EOF);
    } else /* make sure the string is null terminated */
        string[max_length - 1] = '\0';

    return TRUE;
}
