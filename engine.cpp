//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// engine.cpp
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
#include "bot_client.h"
#include "engine.h"
#include "bot_func.h"

#include <meta_api.h> //meta mod

#include "cbase.h"

extern bool mr_meta;

extern enginefuncs_t g_engfuncs;
extern bot_t bots[32];
extern int mod_id;

extern int a;

extern char prevmapname[32];

extern edict_t* clients[32];

static bool name_message_check(const char* msg_string, const char* name_string);

// my external stuff for scripted message intercept
/*extern bool blue_av[8];
   extern bool red_av[8];
   extern bool green_av[8];
   extern bool yellow_av[8];

   extern struct msg_com_struct
   {
   char ifs[32];
   int blue_av[8];
   int red_av[8];
   int green_av[8];
   int yellow_av[8];
   struct msg_com_struct *next;
   } msg_com[MSG_MAX];
   extern char msg_msg[64][MSG_MAX]; */

int debug_engine = 0;

bool spawn_check_crash = FALSE;
int spawn_check_crash_count = 0;
edict_t* spawn_check_crash_edict = NULL;

void (*botMsgFunction)(void*, int) = NULL;
void (*botMsgEndFunction)(void*, int) = NULL;
int botMsgIndex;

// g_state from bot_clients
extern int g_state;

// messages created in RegUserMsg which will be "caught"
int message_VGUI = 0;
int message_ShowMenu = 0;
int message_WeaponList = 0;
int message_CurWeapon = 0;
int message_AmmoX = 0;
int message_WeapPickup = 0;
int message_AmmoPickup = 0;
int message_ItemPickup = 0;
int message_Health = 0;
int message_Battery = 0; // Armor
int message_Damage = 0;
// int message_Money = 0;  // for Counter-Strike
int message_DeathMsg = 0;
int message_TextMsg = 0;
int message_WarmUp = 0;     // for Front Line Force
int message_WinMessage = 0; // for Front Line Force
int message_ScreenFade = 0;
int message_StatusIcon = 0; // flags in tfc

// mine
int message_TeamScores = 0;
int message_StatusText = 0;
int message_StatusValue = 0;
int message_Detpack = 0;
int message_SecAmmoVal = 0;

bool MM_func = FALSE;
static FILE* fp;

bool dont_send_packet = FALSE;

char sz_error_check[255];

int pfnPrecacheModel(char* s)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnPrecacheModel: %s\n", s);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnPrecacheModel: %s\n",s);
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnPrecacheModel)(s);
}

int pfnPrecacheSound(char* s)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnPrecacheSound: %s\n", s);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnPrecacheSound: %s\n",s);
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnPrecacheSound)(s);
}

void pfnSetModel(edict_t* e, const char* m)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSetModel: edict=%p %s\n", (void*)e, m);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnSetModel: edict=%x %s\n",e,m);
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetModel)(e, m);
}

int pfnModelIndex(const char* m)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnModelIndex: %s\n",m); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnModelIndex)(m);
}

int pfnModelFrames(int modelIndex)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnModelFrames:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnModelFrames)(modelIndex);
}

void pfnSetSize(edict_t* e, const float* rgflMin, const float* rgflMax)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSetSize: %p (%f %f %f) (%f %f %f)\n", (void*)e, (*(Vector*)rgflMin).x, (*(Vector*)rgflMin).y,
            (*(Vector*)rgflMin).z, (*(Vector*)rgflMax).x, (*(Vector*)rgflMax).y, (*(Vector*)rgflMax).z);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnSetSize: %x (%f %f %f) (%f %f %f)\n",e,
    // (*(Vector*)rgflMin).x,(*(Vector*)rgflMin).y,(*(Vector*)rgflMin).z,
    // (*(Vector*)rgflMax).x,(*(Vector*)rgflMax).y,(*(Vector*)rgflMax).z);
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetSize)(e, rgflMin, rgflMax);
}

void pfnChangeLevel(char* s1, char* s2)
{
    // prevmapname[0]=NULL; //make sure that even the same level makes
    // it reload behaviour
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnChangeLevel:\n");
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnChangeLevel:\n");

    // kick any bot off of the server after time/frag limit...
    /*for(int index = 0; index < 32; index++)
       {
       if(bots[index].is_used)  // is this slot used?
       {
       char cmd[40];

       sprintf(cmd, "kick \"%s\"\n", bots[index].name);

       //bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
       //if(mod_id == TFC_DLL) FakeClientCommand(bots[index].pEdict,"spectate",NULL,NULL);
       SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
       //bots[index].pEdict->v.team=6;
       bots[index].has_sentry =false;
       }
       }*/

    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnChangeLevel)(s1, s2);
}

void pfnGetSpawnParms(edict_t* ent)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnGetSpawnParms:\n");
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnGetSpawnParms:\n");
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnGetSpawnParms)(ent);
}

void pfnSaveSpawnParms(edict_t* ent)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSaveSpawnParms:\n");
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnSaveSpawnParms:\n");
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSaveSpawnParms)(ent);
}

float pfnVecToYaw(const float* rgflVector)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnVecToYaw:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnVecToYaw)(rgflVector);
}

void pfnVecToAngles(const float* rgflVectorIn, float* rgflVectorOut)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnVecToAngles:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnVecToAngles)(rgflVectorIn, rgflVectorOut);
}

void pfnMoveToOrigin(edict_t* ent, const float* pflGoal, float dist, int iMoveType)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnMoveToOrigin:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnMoveToOrigin)(ent, pflGoal, dist, iMoveType);
}

void pfnChangeYaw(edict_t* ent)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnChangeYaw:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnChangeYaw)(ent);
}

void pfnChangePitch(edict_t* ent)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnChangePitch:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnChangePitch)(ent);
}

edict_t* pfnFindEntityByString(edict_t* pEdictStartSearchAfter, const char* pszField, const char* pszValue)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnFindEntityByString: %s\n",pszValue); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnFindEntityByString)(pEdictStartSearchAfter, pszField, pszValue);
}

int pfnGetEntityIllum(edict_t* pEnt)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnGetEntityIllum:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnGetEntityIllum)(pEnt);
}

edict_t* pfnFindEntityInSphere(edict_t* pEdictStartSearchAfter, const float* org, float rad)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnFindEntityInSphere:%p (%f %f %f) %f %d\n", (void*)pEdictStartSearchAfter, (*(Vector*)org).x,
            (*(Vector*)org).y, (*(Vector*)org).z, rad, spawn_check_crash_count);

        if(pEdictStartSearchAfter != NULL)
            if(pEdictStartSearchAfter->v.classname != 0)
                fprintf(fp, "classname %s\n", STRING(pEdictStartSearchAfter->v.classname));
        fclose(fp);
    }
    if(spawn_check_crash && rad == 96) {
        spawn_check_crash_count++;
        if(spawn_check_crash_count > 512) {
            // pfnSetSize: 958fd0 (-16.000000 -16.000000 -36.000000) (16.000000 16.000000 36.000000)
            SET_ORIGIN(spawn_check_crash_edict, org);
            {
                fp = UTIL_OpenFoxbotLog();
                fprintf(fp, "spawn crash fix!: \n");
                fclose(fp);
            }
            /*		   if(mr_meta) RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
               return NULL;
               edict_t *pEdict;
               if(mr_meta) FIND_ENTITY_IN_SPHERE(pEdictStartSearchAfter, org, rad);
               else pEdict=(*g_engfuncs.pfnFindEntityInSphere)(pEdictStartSearchAfter, org, rad);
               if(pEdict!=NULL)
               if(!FNullEnt(pEdict))
               if(pEdict->v.classname!=0)
               if(strcmp(STRING(pEdict->v.classname),"info_player_teamspawn")!=0)
               {
               //clear counter if going on for too long :(
               if(spawn_check_crash_count>1024)
               spawn_check_crash_count=0;
               if(mr_meta) RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
               return NULL;
               }*/
        }
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnFindEntityInSphere)(pEdictStartSearchAfter, org, rad);
}

edict_t* pfnFindClientInPVS(edict_t* pEdict)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnFindClientInPVS:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnFindClientInPVS)(pEdict);
}

edict_t* pfnEntitiesInPVS(edict_t* pplayer)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnEntitiesInPVS:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnEntitiesInPVS)(pplayer);
}

void pfnMakeVectors(const float* rgflVector)
{
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnMakeVectors)(rgflVector);
}

void pfnAngleVectors(const float* rgflVector, float* forward, float* right, float* up)
{
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnAngleVectors)(rgflVector, forward, right, up);
}

edict_t* pfnCreateEntity_Post(void)
{
    edict_t* pent = META_RESULT_ORIG_RET(edict_t*);

    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnCreateEntity_Post: %p %d\n", (void*)pent, pent->v.spawnflags);

            if(pent->v.classname != 0)
                fprintf(fp, " name=%s\n", STRING(pent->v.classname));
            if(pent->v.target != 0)
                fprintf(fp, " target=%s\n", STRING(pent->v.target));
            if(pent->v.owner != NULL)
                fprintf(fp, " owner=%p\n", (void*)pent->v.owner);
            if(pent->v.chain != NULL)
                fprintf(fp, " chain=%p\n", (void*)pent->v.chain);
            fclose(fp);
        }
    }

    RETURN_META_VALUE(MRES_HANDLED, NULL);
}

edict_t* pfnCreateEntity(void)
{
    edict_t* pent = (*g_engfuncs.pfnCreateEntity)();

    // look for the bot that made the sentry gun(doesn't work!)
    /*	char classname[20];
            strncpy(classname, STRING(pent->v.classname), 19);
            if(strncmp(classname, "building_sentrygun", 18) == 0)
            {
                    for(int i = 0; i < 32; i++)
                    {
                            if(bots[i].is_used
                                    && bots[i].pEdict->v.playerclass == TFC_CLASS_ENGINEER
                                    && VectorsNearerThan(bots[i].pEdict->v.origin, pent->v.origin, 75.0))
                            {
                                    UTIL_BotLogPrintf("pfnCreateEntity: found sentry owner\n");
                            }
                    }
            }*/

    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnCreateEntity: %p\n", (void*)pent);
            if(pent->v.classname != 0)
                fprintf(fp, " name=%s\n", STRING(pent->v.classname));
            if(pent->v.target != 0)
                fprintf(fp, " target=%s\n", STRING(pent->v.target));
            if(pent->v.owner != NULL)
                fprintf(fp, " owner=%p\n", (void*)pent->v.owner);
            if(pent->v.chain != NULL)
                fprintf(fp, " chain=%p\n", (void*)pent->v.chain);
            fclose(fp);
        }
    }

    return pent;
}

void pfnRemoveEntity(edict_t* e)
{
    // tell each bot to forget about the removed entity
    for(int i = 0; i < 32; i++) {
        if(bots[i].is_used) {
            if(bots[i].lastEnemySentryGun == e)
                bots[i].lastEnemySentryGun = NULL;
            if(bots[i].enemy.ptr == e)
                bots[i].enemy.ptr = NULL;

            if(bots[i].pEdict->v.playerclass == TFC_CLASS_ENGINEER) {
                if(bots[i].sentry_edict == e) {
                    bots[i].has_sentry = FALSE;
                    bots[i].sentry_edict = NULL;
                    bots[i].SGRotated = FALSE;
                }

                if(bots[i].dispenser_edict == e) {
                    bots[i].has_dispenser = FALSE;
                    bots[i].dispenser_edict = NULL;
                }

                if(bots[i].tpEntrance == e) {
                    bots[i].tpEntrance = NULL;
                    bots[i].tpEntranceWP = -1;
                }

                if(bots[i].tpExit == e) {
                    bots[i].tpExit = NULL;
                    bots[i].tpExitWP = -1;
                }
            }
        }
    }
    /*if(strncmp(STRING(e->v.classname),"func_",5)==0)
       {
       if(e->v.globalname!=NULL)
       {
       char msg[255];
       //TYPEDESCRIPTION		*pField;
       //pField = &gEntvarsDescription[36];
       //(*(float *)((char *)pev + pField->fieldOffset))
       sprintf(msg,"name %s, toggle %.0f",STRING(e->v.globalname),e->v.frame);
       script(msg);
       }
       }*/

    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnRemoveEntity: %p %d\n", (void*)e, e->v.spawnflags);
            if(e->v.model != 0)
                fprintf(fp, " model=%s\n", STRING(e->v.model));
            if(e->v.classname != 0)
                fprintf(fp, " name=%s\n", STRING(e->v.classname));
            if(e->v.target != 0)
                fprintf(fp, " target=%s\n", STRING(e->v.target));
            if(e->v.owner != NULL)
                fprintf(fp, " owner=%p\n", (void*)e->v.owner);
            if(e->v.chain != NULL)
                fprintf(fp, " chain=%p\n", (void*)e->v.chain);
            fclose(fp);
        }
    }
    // _snprintf(sz_error_check,250,"pfnRemoveEntity: %x %d\n",e,e->v.spawnflags);

    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnRemoveEntity)(e);
}

edict_t* pfnCreateNamedEntity_Post(int className)
{
    edict_t* pent = META_RESULT_ORIG_RET(edict_t*);

    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnCreateNamedEntity: edict=%p name=%s\n", (void*)pent, STRING(className));
            fclose(fp);
        }
    }
    RETURN_META_VALUE(MRES_HANDLED, NULL);
}

edict_t* pfnCreateNamedEntity(int className)
{
    edict_t* pent = (*g_engfuncs.pfnCreateNamedEntity)(className);

    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnCreateNamedEntity: edict=%p name=%s\n", (void*)pent, STRING(className));
            fclose(fp);
        }
    }
    return pent;
}

void pfnMakeStatic(edict_t* ent)
{
    if(debug_engine)
        UTIL_BotLogPrintf("pfnMakeStatic:\n");

    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnMakeStatic)(ent);
}

int pfnEntIsOnFloor(edict_t* e)
{
    if(debug_engine)
        UTIL_BotLogPrintf("pfnEntIsOnFloor:\n");

    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnEntIsOnFloor)(e);
}

int pfnDropToFloor(edict_t* e)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnDropToFloor:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnDropToFloor)(e);
}

int pfnWalkMove(edict_t* ent, float yaw, float dist, int iMode)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnWalkMove:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnWalkMove)(ent, yaw, dist, iMode);
}

void pfnSetOrigin(edict_t* e, const float* rgflOrigin)
{
    if(strcmp(STRING(e->v.classname), "player") == 0) {
        // teleport at new round start
        // clear up current wpt
        for(int bot_index = 0; bot_index < 32; bot_index++) {
            // only consider existing bots that haven't died very recently
            if(bots[bot_index].pEdict == e && bots[bot_index].is_used &&
                (bots[bot_index].f_killed_time + 3.0) < gpGlobals->time) {
                // see if a teleporter pad moved the bot
                const edict_t* teleExit =
                    BotEntityAtPoint("building_teleporter", bots[bot_index].pEdict->v.origin, 90.0);

                if(teleExit == NULL) {
                    //	UTIL_BotLogPrintf("%s Non-teleport translocation, time %f\n",
                    //		bots[bot_index].name, gpGlobals->time);

                    bots[bot_index].current_wp = -1;
                    bots[bot_index].f_snipe_time = 0;
                    bots[bot_index].f_primary_charging = 0;
                }
                /*	else
                        {
                                UTIL_BotLogPrintf("%s Teleported somewhere, time %f\n",
                                                bots[bot_index].name, gpGlobals->time);
                        }*/

                break; // must have found the right bot
            }
        }
    } else if(strcmp(STRING(e->v.classname), "building_sentrygun") == 0) {
        // ok, we have the 'base' entity pointer
        // we want the pointer to the sentry itself

        for(int bot_index = 0; bot_index < 32; bot_index++) {
            if(bots[bot_index].sentry_edict != NULL && bots[bot_index].has_sentry) {
                edict_t* pent = e;
                int l = static_cast<int>(bots[bot_index].sentry_edict->v.origin.z - (*(Vector*)rgflOrigin).z);
                if(l < 0)
                    l = -l;

                int xa = (int)(*(Vector*)rgflOrigin).x;
                int ya = (int)(*(Vector*)rgflOrigin).y;
                int xb = static_cast<int>(bots[bot_index].sentry_edict->v.origin.x);
                int yb = static_cast<int>(bots[bot_index].sentry_edict->v.origin.y);
                // FILE *fp;
                //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"l %d xa %d xb %d ya %d yb %d\n",l,xa,xb,ya,yb); fclose(fp); }
                if(l >= 8 && l <= 60
                    //&& (xa<xb+2 && xa+2>xb)
                    //&& (ya<yb+2 && ya+2>yb))
                    && xa == xb && ya == yb) {
                    bots[bot_index].sentry_edict = pent;
                }
            }
        }
    } else if(strncmp(STRING(e->v.classname), "func_button", 11) == 0 ||
        strncmp(STRING(e->v.classname), "func_rot_button", 15) == 0) {
        if(e->v.target != 0) {
            char msg[255];
            // TYPEDESCRIPTION		*pField;
            // pField = &gEntvarsDescription[36];
            //(*(float *)((char *)pev + pField->fieldOffset))
            sprintf(msg, "target %s, toggle %.0f", STRING(e->v.target), e->v.frame);
            script(msg);
        }
    }
    /*else if(strncmp(STRING(e->v.classname),"func_",5)==0)
       {
       if(e->v.globalname!=NULL)
       {
       char msg[255];
       //TYPEDESCRIPTION		*pField;
       //pField = &gEntvarsDescription[36];
       //(*(float *)((char *)pev + pField->fieldOffset))
       sprintf(msg,"name %s, toggle %.0f",STRING(e->v.globalname),e->v.frame);
       script(msg);
       }
       }*/

    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSetOrigin: %p (%f %f %f)\n", (void*)e, (*(Vector*)rgflOrigin).x, (*(Vector*)rgflOrigin).y,
            (*(Vector*)rgflOrigin).z);

        if(e->v.classname != 0)
            fprintf(fp, " name=%s\n", STRING(e->v.classname));
        if(e->v.target != 0)
            fprintf(fp, " target=%s\n", STRING(e->v.target));
        if((e->v.ltime) < (e->v.nextthink))
            fprintf(fp, " 1\n");
        else
            fprintf(fp, " 0\n");

        fprintf(fp, " t %f %f\n", (e->v.ltime), (e->v.nextthink));
        fprintf(fp, " button=%d\n", e->v.button);
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetOrigin)(e, rgflOrigin);
}

void pfnEmitSound(edict_t* entity,
    int channel,
    const char* sample,
    float volume,
    float attenuation,
    int fFlags,
    int pitch)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnEmitSound: %s\n", sample);
        fclose(fp);
    }

    BotSoundSense(entity, sample, volume);

    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnEmitSound)(entity, channel, sample, volume, attenuation, fFlags, pitch);
}

void pfnEmitAmbientSound(edict_t* entity,
    float* pos,
    const char* samp,
    float vol,
    float attenuation,
    int fFlags,
    int pitch)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnEmitAmbientSound: %s\n", samp);
        fclose(fp);
    }

    script(samp);
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnEmitAmbientSound)(entity, pos, samp, vol, attenuation, fFlags, pitch);
}

void pfnTraceLine(const float* v1, const float* v2, int fNoMonsters, edict_t* pentToSkip, TraceResult* ptr)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnTraceLine:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnTraceLine)(v1, v2, fNoMonsters, pentToSkip, ptr);
}

void pfnTraceToss(edict_t* pent, edict_t* pentToIgnore, TraceResult* ptr)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnTraceToss:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnTraceToss)(pent, pentToIgnore, ptr);
}

int pfnTraceMonsterHull(edict_t* pEdict,
    const float* v1,
    const float* v2,
    int fNoMonsters,
    edict_t* pentToSkip,
    TraceResult* ptr)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnTraceMonsterHull:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnTraceMonsterHull)(pEdict, v1, v2, fNoMonsters, pentToSkip, ptr);
}

void pfnTraceHull(const float* v1,
    const float* v2,
    int fNoMonsters,
    int hullNumber,
    edict_t* pentToSkip,
    TraceResult* ptr)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnTraceHull:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnTraceHull)(v1, v2, fNoMonsters, hullNumber, pentToSkip, ptr);
}

void pfnTraceModel(const float* v1, const float* v2, int hullNumber, edict_t* pent, TraceResult* ptr)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnTraceModel:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnTraceModel)(v1, v2, hullNumber, pent, ptr);
}

const char* pfnTraceTexture(edict_t* pTextureEntity, const float* v1, const float* v2)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnTraceTexture:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnTraceTexture)(pTextureEntity, v1, v2);
}

void pfnTraceSphere(const float* v1,
    const float* v2,
    int fNoMonsters,
    float radius,
    edict_t* pentToSkip,
    TraceResult* ptr)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnTraceSphere:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnTraceSphere)(v1, v2, fNoMonsters, radius, pentToSkip, ptr);
}

void pfnGetAimVector(edict_t* ent, float speed, float* rgflReturn)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnGetAimVector:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnGetAimVector)(ent, speed, rgflReturn);
}

void pfnServerCommand(char* str)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnServerCommand: %s\n", str);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnServerCommand: %s\n",str);
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnServerCommand)(str);
}

void pfnServerExecute(void)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnServerExecute:\n");
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnServerExecute:\n");
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnServerExecute)();
}

void pfnClientCommand(edict_t* pEdict, char* szFmt, ...)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "-pfnClientCommand=%s %p\n", szFmt, (void*)pEdict);
        fclose(fp);
    }
    _snprintf(sz_error_check, 250, "-pfnClientCommand=%s %p\n", szFmt, (void*)pEdict);
    /*if(pEdict!=NULL)
       {
       if((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT)
       {
       //admin mod fix here! ...maybee clientprintf aswell..dunno
       FakeClientCommand(pEdict,szFmt,NULL,NULL);
       //if(mr_meta) RETURN_META(MRES_SUPERCEDE);
       }
       }*/
    char tempFmt[1024];

    va_list argp;
    va_start(argp, szFmt);
    vsprintf(tempFmt, szFmt, argp);

    if(pEdict != NULL) {
        // if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
        bool b = FALSE;
        if(!((pEdict->v.flags & FL_FAKECLIENT) == FL_FAKECLIENT)) {
            for(int i = 0; i < 32; i++) {
                // if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
                // bots[i].is_used &&
                if(clients[i] == pEdict)
                    b = TRUE;
                /*if(bots[i].pEdict==pEdict && (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1 ||
                   (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
                   {
                   b=false;
                   //_snprintf(sz_error_check,250,"%s %d",sz_error_check,i);
                   }*/
            }
        }
        if(b) {
            char cl_name[128];
            cl_name[0] = '\0';

            char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);
            strncpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120);
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"cl %d name %s\n",i,cl_name); fclose(fp); }
            if(cl_name[0] == '\0' || infobuffer == NULL)
                b = FALSE;
            //	unsigned int u=GETPLAYERWONID(pEdict);
            //	if((u==0 || ENTINDEX(pEdict)==-1))
            //		b=false;
            // _snprintf(sz_error_check,250,"%s %d",sz_error_check,GETPLAYERWONID(pEdict));
        }
        if(b) {
            //	_snprintf(sz_error_check,250,"%s b = %d %d\n",sz_error_check,GETPLAYERWONID(pEdict),ENTINDEX(pEdict));
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"b\n"); fclose(fp); }
            // _snprintf(sz_error_check,250,"%s -executing",sz_error_check);
            (*g_engfuncs.pfnClientCommand)(pEdict, tempFmt);
            va_end(argp);
            return;
        } else {
            strncat(sz_error_check, " !b\n", 250 - strlen(sz_error_check));
            return;
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"!b\n"); fclose(fp); }
        }
    }
    (*g_engfuncs.pfnClientCommand)(pEdict, tempFmt);
    va_end(argp);
    // if(mr_meta) RETURN_META(MRES_HANDLED);
    return;
}

void pfnClCom(edict_t* pEdict, char* szFmt, ...)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "-pfnClientCom=%s %p\n", szFmt, (void*)pEdict);
        fclose(fp);
    }
    _snprintf(sz_error_check, 250, "-pfnClientCom=%s %p\n", szFmt, (void*)pEdict);
    if(pEdict != NULL) {
        bool b = FALSE;

        if(!((pEdict->v.flags & FL_FAKECLIENT) == FL_FAKECLIENT)) {
            for(int i = 0; i < 32; i++) {
                // if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
                // bots[i].is_used &&
                if(clients[i] == pEdict)
                    b = TRUE;
                /*if(bots[i].pEdict==pEdict && (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1 ||
                   (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
                   b=false;*/
            }
        }
        if(b) {
            char cl_name[128];
            cl_name[0] = '\0';

            char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);
            strncpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120);
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"cl %d name %s\n",i,cl_name); fclose(fp); }
            if(cl_name[0] == '\0' || infobuffer == NULL)
                b = FALSE;
            // unsigned int u=GETPLAYERWONID(pEdict);
            // if((u==0 || ENTINDEX(pEdict)==-1))
            //	b=false;
        }
        // if its a bot (b=false) we need to override
        if(!b) {
            strncat(sz_error_check, " !b\n", 250 - strlen(sz_error_check));
            // admin mod fix here! ...maybee clientprintf aswell..dunno
            // FakeClientCommand(pEdict,szFmt,NULL,NULL);
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"!b\n"); fclose(fp); }
            if(mr_meta)
                RETURN_META(MRES_SUPERCEDE);
            return;
        } else
            //	_snprintf(sz_error_check,250,"%s b = %d %d\n",sz_error_check,GETPLAYERWONID(pEdict),ENTINDEX(pEdict));
            return;

    } else {
        if(mr_meta)
            RETURN_META(MRES_SUPERCEDE);
        return;
    }
    //	if(mr_meta) RETURN_META(MRES_HANDLED);  // unreachable code
    //	return;
}

void pfnParticleEffect(const float* org, const float* dir, float color, float count)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnParticleEffect:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnParticleEffect)(org, dir, color, count);
}

void pfnLightStyle(int style, char* val)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnLightStyle:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnLightStyle)(style, val);
}

int pfnDecalIndex(const char* name)
{
    /*if(debug_engine)
            { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnDecalIndex:\n"); fclose(fp); }*/
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnDecalIndex)(name);
}

int pfnPointContents(const float* rgflVector)
{
    /*if(debug_engine)
            { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnPointContents:\n"); fclose(fp); }*/

    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnPointContents)(rgflVector);
}

void MessageBegin(int msg_dest, int msg_type, const float* pOrigin, edict_t* ed)
{
    MM_func = TRUE;
    pfnMessageBegin(msg_dest, msg_type, pOrigin, ed);
    MM_func = FALSE;
}

void pfnMessageBegin(int msg_dest, int msg_type, const float* pOrigin, edict_t* ed)
{
    /*if(ed!=NULL)
       if(ed->v.classname==NULL || ed->v.netname==NULL)
       dont_send_packet=true;*/
    if(gpGlobals->deathmatch) {
        if(debug_engine /* || dont_send_packet*/) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnMessageBegin: edict=%p dest=%d type=%d\n", (void*)ed, msg_dest, msg_type);
            fclose(fp);
        }

        /*_snprintf(sz_error_check,250,
                "pfnMessageBegin: edict=%x dest=%d type=%d id=%d %d\n",
                ed,msg_dest,msg_type,GETPLAYERWONID(ed),ENTINDEX(ed));*/

        if(ed) {
            int index = UTIL_GetBotIndex(ed);

            // is this message for a bot?
            if(index != -1) {
                g_state = 0;              // reset global message state..where we at!
                botMsgFunction = NULL;    // no msg function until known otherwise
                botMsgEndFunction = NULL; // no msg end function until known otherwise
                botMsgIndex = index;      // index of bot receiving message

                if(mod_id == VALVE_DLL) {
                    if(msg_type == message_WeaponList)
                        botMsgFunction = BotClient_Valve_WeaponList;
                    else if(msg_type == message_CurWeapon)
                        botMsgFunction = BotClient_Valve_CurrentWeapon;
                    else if(msg_type == message_AmmoX)
                        botMsgFunction = BotClient_Valve_AmmoX;
                    else if(msg_type == message_AmmoPickup)
                        botMsgFunction = BotClient_Valve_AmmoPickup;
                    else if(msg_type == message_WeapPickup)
                        botMsgFunction = BotClient_Valve_WeaponPickup;
                    else if(msg_type == message_ItemPickup)
                        botMsgFunction = BotClient_Valve_ItemPickup;
                    else if(msg_type == message_Health)
                        botMsgFunction = BotClient_Valve_Health;
                    else if(msg_type == message_Battery)
                        botMsgFunction = BotClient_Valve_Battery;
                    else if(msg_type == message_Damage)
                        botMsgFunction = BotClient_Valve_Damage;
                    else if(msg_type == message_ScreenFade)
                        botMsgFunction = BotClient_Valve_ScreenFade;
                } else if(mod_id == TFC_DLL) {
                    if(msg_type == message_VGUI)
                        botMsgFunction = BotClient_TFC_VGUI;
                    else if(msg_type == message_WeaponList)
                        botMsgFunction = BotClient_TFC_WeaponList;
                    else if(msg_type == message_CurWeapon)
                        botMsgFunction = BotClient_TFC_CurrentWeapon;
                    else if(msg_type == message_AmmoX)
                        botMsgFunction = BotClient_TFC_AmmoX;
                    else if(msg_type == message_AmmoPickup)
                        botMsgFunction = BotClient_TFC_AmmoPickup;
                    else if(msg_type == message_WeapPickup)
                        botMsgFunction = BotClient_TFC_WeaponPickup;
                    else if(msg_type == message_ItemPickup)
                        botMsgFunction = BotClient_TFC_ItemPickup;
                    else if(msg_type == message_Health)
                        botMsgFunction = BotClient_TFC_Health;
                    else if(msg_type == message_Battery)
                        botMsgFunction = BotClient_TFC_Battery;
                    else if(msg_type == message_Damage)
                        botMsgFunction = BotClient_TFC_Damage;
                    else if(msg_type == message_ScreenFade)
                        botMsgFunction = BotClient_TFC_ScreenFade;
                    else if(msg_type == message_StatusIcon)
                        botMsgFunction = BotClient_TFC_StatusIcon;
                    // not all these messages are used for..but all I am checking for
                    else if(msg_type == message_TextMsg || msg_type == message_StatusText)
                        botMsgFunction = BotClient_Engineer_BuildStatus;
                    else if(msg_type == message_StatusValue)
                        botMsgFunction = BotClient_TFC_SentryAmmo;
                    else if(msg_type == message_Detpack)
                        botMsgFunction = BotClient_TFC_DetPack;
                    // menu! prolly from admin mod
                    else if(msg_type == message_ShowMenu)
                        botMsgFunction = BotClient_Menu;
                    else if(msg_type == message_SecAmmoVal)
                        botMsgFunction = BotClient_TFC_Grens;
                } /*else if(mod_id == CSTRIKE_DLL) { // Not required for TFC [APG]RoboCop[CL]
                    if(msg_type == message_VGUI)
                        botMsgFunction = BotClient_CS_VGUI;
                    else if(msg_type == message_ShowMenu)
                        botMsgFunction = BotClient_CS_ShowMenu;
                    else if(msg_type == message_WeaponList)
                        botMsgFunction = BotClient_CS_WeaponList;
                    else if(msg_type == message_CurWeapon)
                        botMsgFunction = BotClient_CS_CurrentWeapon;
                    else if(msg_type == message_AmmoX)
                        botMsgFunction = BotClient_CS_AmmoX;
                    else if(msg_type == message_WeapPickup)
                        botMsgFunction = BotClient_CS_WeaponPickup;
                    else if(msg_type == message_AmmoPickup)
                        botMsgFunction = BotClient_CS_AmmoPickup;
                    else if(msg_type == message_ItemPickup)
                        botMsgFunction = BotClient_CS_ItemPickup;
                    else if(msg_type == message_Health)
                        botMsgFunction = BotClient_CS_Health;
                    else if(msg_type == message_Battery)
                        botMsgFunction = BotClient_CS_Battery;
                    else if(msg_type == message_Damage)
                        botMsgFunction = BotClient_CS_Damage;
                    //		else if(msg_type == message_Money)
                    //			botMsgFunction = BotClient_CS_Money;
                    else if(msg_type == message_ScreenFade)
                        botMsgFunction = BotClient_CS_ScreenFade;
                } else if(mod_id == GEARBOX_DLL) {
                    if(msg_type == message_VGUI)
                        botMsgFunction = BotClient_Gearbox_VGUI;
                    else if(msg_type == message_WeaponList)
                        botMsgFunction = BotClient_Gearbox_WeaponList;
                    else if(msg_type == message_CurWeapon)
                        botMsgFunction = BotClient_Gearbox_CurrentWeapon;
                    else if(msg_type == message_AmmoX)
                        botMsgFunction = BotClient_Gearbox_AmmoX;
                    else if(msg_type == message_AmmoPickup)
                        botMsgFunction = BotClient_Gearbox_AmmoPickup;
                    else if(msg_type == message_WeapPickup)
                        botMsgFunction = BotClient_Gearbox_WeaponPickup;
                    else if(msg_type == message_ItemPickup)
                        botMsgFunction = BotClient_Gearbox_ItemPickup;
                    else if(msg_type == message_Health)
                        botMsgFunction = BotClient_Gearbox_Health;
                    else if(msg_type == message_Battery)
                        botMsgFunction = BotClient_Gearbox_Battery;
                    else if(msg_type == message_Damage)
                        botMsgFunction = BotClient_Gearbox_Damage;
                    else if(msg_type == message_ScreenFade)
                        botMsgFunction = BotClient_Gearbox_ScreenFade;
                } else if(mod_id == FRONTLINE_DLL) {
                    if(msg_type == message_VGUI)
                        botMsgFunction = BotClient_FLF_VGUI;
                    else if(msg_type == message_WeaponList)
                        botMsgFunction = BotClient_FLF_WeaponList;
                    else if(msg_type == message_CurWeapon)
                        botMsgFunction = BotClient_FLF_CurrentWeapon;
                    else if(msg_type == message_AmmoX)
                        botMsgFunction = BotClient_FLF_AmmoX;
                    else if(msg_type == message_AmmoPickup)
                        botMsgFunction = BotClient_FLF_AmmoPickup;
                    else if(msg_type == message_WeapPickup)
                        botMsgFunction = BotClient_FLF_WeaponPickup;
                    else if(msg_type == message_ItemPickup)
                        botMsgFunction = BotClient_FLF_ItemPickup;
                    else if(msg_type == message_Health)
                        botMsgFunction = BotClient_FLF_Health;
                    else if(msg_type == message_Battery)
                        botMsgFunction = BotClient_FLF_Battery;
                    else if(msg_type == message_Damage)
                        botMsgFunction = BotClient_FLF_Damage;
                    //	else if(msg_type == message_TextMsg)
                    //		botMsgFunction = BotClient_FLF_TextMsg;
                    //	else if(msg_type == message_WarmUp)
                    //		botMsgFunction = BotClient_FLF_WarmUp;
                    else if(msg_type == message_ScreenFade)
                        botMsgFunction = BotClient_FLF_ScreenFade;
                    //	else if(msg_type == 23)         // SVC_TEMPENTITY
                    //	{
                    //		botMsgFunction = BotClient_FLF_TempEntity;
                    //		botMsgEndFunction = BotClient_FLF_TempEntity;
                    //	}
                }*/
            } else {
                // (index == -1)
                g_state = 0;              // reset global message state..where we at!
                botMsgFunction = NULL;    // no msg function until known otherwise
                botMsgEndFunction = NULL; // no msg end function until known otherwise
                botMsgIndex = index;      // index of bot receiving message
                if(mod_id == TFC_DLL) {
                    if(msg_type == message_TextMsg || msg_type == message_StatusText)
                        botMsgFunction = BotClient_Engineer_BuildStatus;
                }
            }
        } else if(msg_dest == MSG_ALL) {
            botMsgFunction = NULL; // no msg function until known otherwise
            botMsgIndex = -1;      // index of bot receiving message (none)

            if(mod_id == VALVE_DLL) {
                if(msg_type == message_DeathMsg)
                    botMsgFunction = BotClient_Valve_DeathMsg;
            } else if(mod_id == TFC_DLL) {
                if(msg_type == message_DeathMsg)
                    botMsgFunction = BotClient_TFC_DeathMsg;

                //	else if(msg_type == message_TeamScores)
                //		botMsgFunction = BotClient_TFC_Scores;

                // put the new message here
            } /*else if(mod_id == CSTRIKE_DLL) {
                if(msg_type == message_DeathMsg)
                    botMsgFunction = BotClient_CS_DeathMsg;
            } else if(mod_id == GEARBOX_DLL) {
                if(msg_type == message_DeathMsg)
                    botMsgFunction = BotClient_Gearbox_DeathMsg;
            } else if(mod_id == FRONTLINE_DLL) {
                if(msg_type == message_DeathMsg)
                    botMsgFunction = BotClient_FLF_DeathMsg;
                //	else if(msg_type == message_WarmUp)
                //		botMsgFunction = BotClient_FLF_WarmUpAll;
                //	else if(msg_type == message_WinMessage)
                //		botMsgFunction = BotClient_FLF_WinMessage;
            }*/
        }
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}

void MessageEnd(void)
{
    MM_func = TRUE;
    pfnMessageEnd();
    MM_func = FALSE;
}

void pfnMessageEnd(void)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnMessageEnd:\n"); fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnMessageEnd:\n");
            fclose(fp);
        }

        if(botMsgEndFunction)
            (*botMsgEndFunction)(NULL, botMsgIndex); // NULL indicated msg end

        // clear out the bot message function pointers...
        botMsgFunction = NULL;
        botMsgEndFunction = NULL;
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet) {
            dont_send_packet = FALSE;
            RETURN_META(MRES_SUPERCEDE);
        } else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet) {
        dont_send_packet = FALSE;
        return;
    } else
        (*g_engfuncs.pfnMessageEnd)();
}

void WriteByte(int iValue)
{
    MM_func = TRUE;
    pfnWriteByte(iValue);
    MM_func = FALSE;
}

void pfnWriteByte(int iValue)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteByte: %d\n",iValue);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteByte: %d\n", iValue);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)&iValue, botMsgIndex);
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteByte)(iValue);
}

void WriteChar(int iValue)
{
    MM_func = TRUE;
    pfnWriteChar(iValue);
    MM_func = FALSE;
}

void pfnWriteChar(int iValue)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteChar: %d\n",iValue);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteChar: %d\n", iValue);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)&iValue, botMsgIndex);
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteChar)(iValue);
}

void WriteShort(int iValue)
{
    MM_func = TRUE;
    pfnWriteShort(iValue);
    MM_func = FALSE;
}

void pfnWriteShort(int iValue)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteShort: %d\n",iValue);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteShort: %d\n", iValue);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)&iValue, botMsgIndex);
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteShort)(iValue);
}

void WriteLong(int iValue)
{
    MM_func = TRUE;
    pfnWriteLong(iValue);
    MM_func = FALSE;
}

void pfnWriteLong(int iValue)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteLong: %d\n",iValue);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteLong: %d\n", iValue);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)&iValue, botMsgIndex);
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteLong)(iValue);
}

void WriteAngle(float flValue)
{
    MM_func = TRUE;
    pfnWriteAngle(flValue);
    MM_func = FALSE;
}

void pfnWriteAngle(float flValue)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteAngle: %f\n",flValue);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteAngle: %f\n", flValue);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)&flValue, botMsgIndex);
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteAngle)(flValue);
}

void WriteCoord(float flValue)
{
    MM_func = TRUE;
    pfnWriteCoord(flValue);
    MM_func = FALSE;
}

void pfnWriteCoord(float flValue)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteCoord: %f\n",flValue);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteCoord: %f\n", flValue);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)&flValue, botMsgIndex);
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteCoord)(flValue);
}

void WriteString(const char* sz)
{
    MM_func = TRUE;
    pfnWriteString(sz);
    MM_func = FALSE;
}

void pfnWriteString(const char* sz)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteString: %s\n",sz);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteString: %s\n", sz);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)sz, botMsgIndex);
    }
    script(sz);

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteString)(sz);
}

void WriteEntity(int iValue)
{
    MM_func = TRUE;
    pfnWriteEntity(iValue);
    MM_func = FALSE;
}

void pfnWriteEntity(int iValue)
{
    if(gpGlobals->deathmatch) {
        // if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnWriteEntity: %d\n",iValue);
        // fclose(fp); }
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnWriteEntity: %d\n", iValue);
            fclose(fp);
        }

        // if this message is for a bot, call the client message function...
        if(botMsgFunction)
            (*botMsgFunction)((void*)&iValue, botMsgIndex);
    }

    if(mr_meta && MM_func) {
        if(dont_send_packet)
            RETURN_META(MRES_SUPERCEDE);
        else
            RETURN_META(MRES_HANDLED);
    }
    if(dont_send_packet)
        return;
    else
        (*g_engfuncs.pfnWriteEntity)(iValue);
}

void pfnCVarRegister(cvar_t* pCvar)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCVarRegister:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCVarRegister)(pCvar);
}

float pfnCVarGetFloat(const char* szVarName)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnCVarGetFloat: %s\n",szVarName); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnCVarGetFloat)(szVarName);
}

const char* pfnCVarGetString(const char* szVarName)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCVarGetString: v%s\n", szVarName);
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnCVarGetString)(szVarName);
}

void pfnCVarSetFloat(const char* szVarName, float flValue)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnCVarSetFloat:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCVarSetFloat)(szVarName, flValue);
}

void pfnCVarSetString(const char* szVarName, const char* szValue)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCVarSetString: v%s c%s\n", szVarName, szValue);
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCVarSetString)(szVarName, szValue);
}

void* pfnPvAllocEntPrivateData(edict_t* pEdict, int32 cb)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnPvAllocEntPrivateData: %p %d\n", (void*)pEdict, cb);
        /*if(pEdict->v.model != 0)
           fprintf(fp," model=%s\n", STRING(pEdict->v.model));
           if(pEdict->v.classname != 0)
           fprintf(fp," name=%s\n", STRING(pEdict->v.classname));
           if(pEdict->v.target != 0)
           fprintf(fp," target=%s\n", STRING(pEdict->v.target));
           if(pEdict->v.owner!=NULL)
           fprintf(fp," owner=%x\n", pEdict->v.owner);
           if(pEdict->v.chain!=NULL)
           fprintf(fp," chain=%x\n", pEdict->v.chain);*/
        fclose(fp);
        // UTIL_SavePent(pEdict);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnPvAllocEntPrivateData)(pEdict, cb);
}

void* pfnPvEntPrivateData(edict_t* pEdict)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnPvEntPrivateData:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnPvEntPrivateData)(pEdict);
}

void pfnFreeEntPrivateData(edict_t* pEdict)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnFreeEntPrivateData:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnFreeEntPrivateData)(pEdict);
}

const char* pfnSzFromIndex(int iString)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSzFromIndex:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnSzFromIndex)(iString);
}

int pfnAllocString(const char* szValue)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnAllocString:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnAllocString)(szValue);
}

entvars_t* pfnGetVarsOfEnt(edict_t* pEdict)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnGetVarsOfEnt:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnGetVarsOfEnt)(pEdict);
}

edict_t* pfnPEntityOfEntOffset(int iEntOffset)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnPEntityOfEntOffset:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnPEntityOfEntOffset)(iEntOffset);
}

int pfnEntOffsetOfPEntity(const edict_t* pEdict)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnEntOffsetOfPEntity: %x\n",pEdict); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnEntOffsetOfPEntity)(pEdict);
}

int pfnIndexOfEdict(const edict_t* pEdict)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnIndexOfEdict: %x\n",pEdict); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnIndexOfEdict)(pEdict);
}

edict_t* pfnPEntityOfEntIndex(int iEntIndex)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnPEntityOfEntIndex:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnPEntityOfEntIndex)(iEntIndex);
}

edict_t* pfnFindEntityByVars(entvars_t* pvars)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnFindEntityByVars:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnFindEntityByVars)(pvars);
}

void* pfnGetModelPtr(edict_t* pEdict)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnGetModelPtr: %x\n",pEdict); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnGetModelPtr)(pEdict);
}

int pfnRegUserMsg(const char* pszName, int iSize)
{
    int msg;

    if(mr_meta)
        msg = META_RESULT_ORIG_RET(int);
    else
        msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);
#ifdef _DEBUG
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnRegUserMsg: pszName=%s msg=%d\n", pszName, msg);
        fclose(fp);
    }
#endif
    //{fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnRegUserMsg: pszName=%s msg=%d\n",pszName,msg); fclose(fp);}
    if(gpGlobals->deathmatch) {
        if(mod_id == VALVE_DLL) {
            if(strcmp(pszName, "WeaponList") == 0)
                message_WeaponList = msg;
            else if(strcmp(pszName, "CurWeapon") == 0)
                message_CurWeapon = msg;
            else if(strcmp(pszName, "AmmoX") == 0)
                message_AmmoX = msg;
            else if(strcmp(pszName, "AmmoPickup") == 0)
                message_AmmoPickup = msg;
            else if(strcmp(pszName, "WeapPickup") == 0)
                message_WeapPickup = msg;
            else if(strcmp(pszName, "ItemPickup") == 0)
                message_ItemPickup = msg;
            else if(strcmp(pszName, "Health") == 0)
                message_Health = msg;
            else if(strcmp(pszName, "Battery") == 0)
                message_Battery = msg;
            else if(strcmp(pszName, "Damage") == 0)
                message_Damage = msg;
            else if(strcmp(pszName, "DeathMsg") == 0)
                message_DeathMsg = msg;
            else if(strcmp(pszName, "ScreenFade") == 0)
                message_ScreenFade = msg;
        } else if(mod_id == TFC_DLL) {
            if(strcmp(pszName, "VGUIMenu") == 0)
                message_VGUI = msg;
            else if(strcmp(pszName, "WeaponList") == 0)
                message_WeaponList = msg;
            else if(strcmp(pszName, "CurWeapon") == 0)
                message_CurWeapon = msg;
            else if(strcmp(pszName, "AmmoX") == 0)
                message_AmmoX = msg;
            else if(strcmp(pszName, "AmmoPickup") == 0)
                message_AmmoPickup = msg;
            else if(strcmp(pszName, "WeapPickup") == 0)
                message_WeapPickup = msg;
            else if(strcmp(pszName, "ItemPickup") == 0)
                message_ItemPickup = msg;
            else if(strcmp(pszName, "Health") == 0)
                message_Health = msg;
            else if(strcmp(pszName, "Battery") == 0)
                message_Battery = msg;
            else if(strcmp(pszName, "Damage") == 0)
                message_Damage = msg;
            else if(strcmp(pszName, "TextMsg") == 0)
                message_TextMsg = msg;
            else if(strcmp(pszName, "DeathMsg") == 0)
                message_DeathMsg = msg;
            else if(strcmp(pszName, "ScreenFade") == 0)
                message_ScreenFade = msg;
            else if(strcmp(pszName, "StatusIcon") == 0)
                message_StatusIcon = msg;
            //
            else if(strcmp(pszName, "TeamScore") == 0)
                message_TeamScores = msg;
            else if(strcmp(pszName, "StatusText") == 0)
                message_StatusText = msg;
            else if(strcmp(pszName, "StatusValue") == 0)
                message_StatusValue = msg;
            else if(strcmp(pszName, "Detpack") == 0)
                message_Detpack = msg;
            else if(strcmp(pszName, "SecAmmoVal") == 0)
                message_SecAmmoVal = msg;
        } /*else if(mod_id == CSTRIKE_DLL) {
            if(strcmp(pszName, "VGUIMenu") == 0)
                message_VGUI = msg;
            else if(strcmp(pszName, "ShowMenu") == 0)
                message_ShowMenu = msg;
            else if(strcmp(pszName, "WeaponList") == 0)
                message_WeaponList = msg;
            else if(strcmp(pszName, "CurWeapon") == 0)
                message_CurWeapon = msg;
            else if(strcmp(pszName, "AmmoX") == 0)
                message_AmmoX = msg;
            else if(strcmp(pszName, "AmmoPickup") == 0)
                message_AmmoPickup = msg;
            else if(strcmp(pszName, "WeapPickup") == 0)
                message_WeapPickup = msg;
            else if(strcmp(pszName, "ItemPickup") == 0)
                message_ItemPickup = msg;
            else if(strcmp(pszName, "Health") == 0)
                message_Health = msg;
            else if(strcmp(pszName, "Battery") == 0)
                message_Battery = msg;
            else if(strcmp(pszName, "Damage") == 0)
                message_Damage = msg;
            //	else if(strcmp(pszName, "Money") == 0)  // counterstrike money
            //		message_Money = msg;
            else if(strcmp(pszName, "DeathMsg") == 0)
                message_DeathMsg = msg;
            else if(strcmp(pszName, "ScreenFade") == 0)
                message_ScreenFade = msg;
        } else if(mod_id == GEARBOX_DLL) {
            if(strcmp(pszName, "VGUIMenu") == 0)
                message_VGUI = msg;
            else if(strcmp(pszName, "WeaponList") == 0)
                message_WeaponList = msg;
            else if(strcmp(pszName, "CurWeapon") == 0)
                message_CurWeapon = msg;
            else if(strcmp(pszName, "AmmoX") == 0)
                message_AmmoX = msg;
            else if(strcmp(pszName, "AmmoPickup") == 0)
                message_AmmoPickup = msg;
            else if(strcmp(pszName, "WeapPickup") == 0)
                message_WeapPickup = msg;
            else if(strcmp(pszName, "ItemPickup") == 0)
                message_ItemPickup = msg;
            else if(strcmp(pszName, "Health") == 0)
                message_Health = msg;
            else if(strcmp(pszName, "Battery") == 0)
                message_Battery = msg;
            else if(strcmp(pszName, "Damage") == 0)
                message_Damage = msg;
            else if(strcmp(pszName, "DeathMsg") == 0)
                message_DeathMsg = msg;
            else if(strcmp(pszName, "ScreenFade") == 0)
                message_ScreenFade = msg;
        } else if(mod_id == FRONTLINE_DLL) {
            if(strcmp(pszName, "VGUIMenu") == 0)
                message_VGUI = msg;
            else if(strcmp(pszName, "WeaponList") == 0)
                message_WeaponList = msg;
            else if(strcmp(pszName, "CurWeapon") == 0)
                message_CurWeapon = msg;
            else if(strcmp(pszName, "AmmoX") == 0)
                message_AmmoX = msg;
            else if(strcmp(pszName, "AmmoPickup") == 0)
                message_AmmoPickup = msg;
            else if(strcmp(pszName, "WeapPickup") == 0)
                message_WeapPickup = msg;
            else if(strcmp(pszName, "ItemPickup") == 0)
                message_ItemPickup = msg;
            else if(strcmp(pszName, "Health") == 0)
                message_Health = msg;
            else if(strcmp(pszName, "Battery") == 0)
                message_Battery = msg;
            else if(strcmp(pszName, "Damage") == 0)
                message_Damage = msg;
            else if(strcmp(pszName, "DeathMsg") == 0)
                message_DeathMsg = msg;
            else if(strcmp(pszName, "TextMsg") == 0)
                message_TextMsg = msg;
            else if(strcmp(pszName, "WarmUp") == 0)
                message_WarmUp = msg;
            else if(strcmp(pszName, "WinMessage") == 0)
                message_WinMessage = msg;
            else if(strcmp(pszName, "ScreenFade") == 0)
                message_ScreenFade = msg;
        }*/
    }

    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return msg;
}

void pfnAnimationAutomove(const edict_t* pEdict, float flTime)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnAnimationAutomove:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnAnimationAutomove)(pEdict, flTime);
}

void pfnGetBonePosition(const edict_t* pEdict, int iBone, float* rgflOrigin, float* rgflAngles)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnGetBonePosition:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnGetBonePosition)(pEdict, iBone, rgflOrigin, rgflAngles);
}

uint32 pfnFunctionFromName(const char* pName)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnFunctionFromName:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnFunctionFromName)(pName);
}

const char* pfnNameForFunction(uint32 function)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnNameForFunction:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnNameForFunction)(function);
}

void pfnClientPrintf(edict_t* pEdict, PRINT_TYPE ptype, const char* szMsg)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnClientPrintf: %p %s\n", (void*)pEdict, szMsg);
        fclose(fp);
    }

    _snprintf(sz_error_check, 250, "CPf: %p %s\n", (void*)pEdict, szMsg);

    // only send message if its not a bot...
    if(pEdict != NULL) {
        bool b = FALSE;
        if(!(pEdict->v.flags & FL_FAKECLIENT)) {
            for(int i = 0; i < 32; i++) {
                // if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
                // bots[i].is_used &&
                /*if(clients[i]!=NULL)
                   _snprintf(sz_error_check,250,"%s %x %d\n",sz_error_check,clients[i],i);*/
                if(clients[i] == pEdict)
                    b = TRUE;
                /*if(bots[i].pEdict==pEdict && (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1 ||
                   (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
                   {
                   b=false;

                   //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"-bot= %x %d\n",pEdict,i); fclose(fp); }
                   }*/
            }
        }
        if(b) {
            char cl_name[128] = " -";

            char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);

            strncat(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120 - strlen(cl_name));
            strncat(cl_name, "-\n", 127 - strlen(cl_name));

            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"cl %d name %s\n",i,cl_name); fclose(fp); }
            if(infobuffer == NULL)
                b = FALSE;
            //	unsigned int u=GETPLAYERWONID(pEdict);
            //	if((u==0 || ENTINDEX(pEdict)==-1))
            //	{
            //		b=false;
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"-wonid=0 %d\n",GETPLAYERWONID(pEdict)); fclose(fp); }
            //	}
            strncat(sz_error_check, cl_name, 250 - strlen(sz_error_check));
        }
        if(b) {
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"b\n"); fclose(fp); }
            //	_snprintf(sz_error_check,250,"%s b = %d %d\n",sz_error_check,GETPLAYERWONID(pEdict),ENTINDEX(pEdict));
            (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
            // else RETURN_META(MRES_HANDLED);
            //(*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
            return;
        } else {
            strncat(sz_error_check, " !b\n", 250 - strlen(sz_error_check));
            return;
            // else
            //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"!!b\n"); fclose(fp); }
        }
        /*else
           {
           if(mr_meta) RETURN_META(MRES_SUPERCEDE);
           }*/
    } else {
        strncat(sz_error_check, " NULL\n", 250 - strlen(sz_error_check));
        //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"fook\n"); fclose(fp); }
        // if(mr_meta) RETURN_META(MRES_SUPERCEDE);
        // if(!mr_meta) (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
        // else RETURN_META(MRES_HANDLED);
        //(*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
    }
    (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
}

void pfnClPrintf(edict_t* pEdict, PRINT_TYPE ptype, const char* szMsg)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnClPrintf: %p %s\n", (void*)pEdict, szMsg);
        fclose(fp);
    }
    _snprintf(sz_error_check, 250, "pfnClPrintf: %p %s\n", (void*)pEdict, szMsg);

    // only send message if its not a bot...
    if(pEdict != NULL) {
        bool b = FALSE;
        if(!((pEdict->v.flags & FL_FAKECLIENT) == FL_FAKECLIENT)) {
            for(int i = 0; i < 32; i++) {
                // if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
                // bots[i].is_used &&
                /*if(bots[i].pEdict==pEdict
                        && (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1
                        || (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
                   b=false;*/
                if(clients[i] == pEdict)
                    b = TRUE;
            }
        }
        if(b) {
            char cl_name[128];
            cl_name[0] = '\0';

            char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);
            strncpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120);
            /*{ fp=UTIL_OpenFoxbotLog();
                    fprintf(fp,"cl %d name %s\n",i,cl_name); fclose(fp);}*/
            if(cl_name[0] == '\0' || infobuffer == NULL)
                b = FALSE;
            //	unsigned int u=GETPLAYERWONID(pEdict);
            //	if((u==0 || ENTINDEX(pEdict)==-1))
            //		b=false;
        }
        if(b) {
            RETURN_META(MRES_HANDLED);
            return;
        } else {
            RETURN_META(MRES_SUPERCEDE);
            return;
        }
    } else {
        RETURN_META(MRES_SUPERCEDE);
        return;
    }
    //	RETURN_META(MRES_HANDLED);
}

void pfnServerPrint(const char* szMsg)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnServerPrint: %s\n", szMsg);
        fclose(fp);
    }

    // _snprintf(sz_error_check,250,"pfnServerPrint: %s\n",szMsg);

    // if were gonna deal with commands for bots (e.i.'follow user')
    // then this is a good place to start

    int i, j, k; // loop counters
    // bool loop = TRUE;
    char sz[1024]; // needs to be defined at max message length..is 1024 ok?
    char msgstart[255];
    char buffa[255];
    char cmd[255];
    i = 0;

    // first compare the message to all bot names, then if bots name is
    // in message pass to bot
    // check that the bot that sent a message isn't getting it back
    strncpy(sz, szMsg, 253);
    // clear up sz, and copy start to buffa
    while(sz[i] != ' ' && i < 250) {
        msgstart[i] = sz[i];
        sz[i] = ' ';
        i++;
    }
    msgstart[i] = '\0'; // finish string off
    i = 0;

    /* { fp=UTIL_OpenFoxbotLog();
            fprintf(fp,"pfnServerPrint: %s %s\n",sz,msgstart); fclose(fp);}*/

    // look through the list of active bots for the intended recipient of
    // the message
    while(i < 32) {
        strncpy(buffa, sz, 253);
        k = 1;
        while(k != 0) {
            // remove start spaces
            j = 0;
            while((buffa[j] == ' ' || buffa[j] == '\n') && j < 250) {
                j++;
            }

            /*{ fp=UTIL_OpenFoxbotLog();
                    fprintf(fp,"pfnServerPrint: %s\n",buffa); fclose(fp); }*/

            k = 0;
            while(buffa[j] != ' ' && buffa[j] != '\0' && buffa[j] != '\n' && j < 250 && k < 250) {
                cmd[k] = buffa[j];
                buffa[j] = ' ';
                j++;
                k++;
            }
            cmd[k] = '\0';

            /*			if(bots[i].is_used)
                                    {
                                            fp = UTIL_OpenFoxbotLog();
                                            fprintf(fp, "pfnServerPrint: cmd|%s bot_name|%s\n", cmd, bots[i].name);
                                            fclose(fp);
                                    }*/

            // check that the message was meant for this bot
            // bots[i].name = name obviously
            if((bots[i].is_used && name_message_check(szMsg, bots[i].name)) ||
                (bots[i].is_used && stricmp(cmd, "bots") == 0)) {
                // DONT ALLOW CHANGECLASS TO ALL BOTS
                if((stricmp(cmd, "bots") == 0) && strstr(szMsg, "changeclass"))
                    continue;
                if((stricmp(cmd, "bots") == 0) && strstr(szMsg, "changeclassnow"))
                    continue;

                strncpy(bots[i].message, szMsg, 253);
                strncpy(bots[i].msgstart, msgstart, 253);
                bots[i].newmsg = TRUE; // tell the bot it has mail
            }
        }

        i++;
    }

    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnServerPrint)(szMsg);
}

// This function returns true if the bots name is in the indicated message.
static bool name_message_check(const char* msg_string, const char* name_string)
{
    const size_t msg_length = strlen(msg_string);
    const size_t name_end = strlen(name_string) - (size_t)1;

    if(msg_length < name_end)
        return FALSE;

    /*	{
                    fp = UTIL_OpenFoxbotLog();
                    fprintf(fp, "bot_name|%s length %d\n", name_string, (int)name_end);
                    fprintf(fp, "msg|%s length %d\n", msg_string, (int)msg_length);
                    fclose(fp);
            }*/

    unsigned int j = 0;

    // start the search
    for(unsigned int i = 0; i < msg_length; i++) {
        // does this letter of the message match a character of the bots name?
        if(msg_string[i] == name_string[j]) {
            // found the last matching character of the bots name?
            if(j >= name_end)
                return TRUE;

            ++j; // go on to the next character of the bots name
        } else
            j = 0; // reset to the start of the bots name
    }

    return FALSE;
}

void pfnGetAttachment(const edict_t* pEdict, int iAttachment, float* rgflOrigin, float* rgflAngles)
{
    /*if(debug_engine)
            { fp=UTIL_OpenFoxbotLog();
                    fprintf(fp,"pfnGetAttachment:\n"); fclose(fp); }*/
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnGetAttachment)(pEdict, iAttachment, rgflOrigin, rgflAngles);
}

void pfnCRC32_Init(CRC32_t* pulCRC)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCRC32_Init:\n");
        fclose(fp);
    }

    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCRC32_Init)(pulCRC);
}

void pfnCRC32_ProcessBuffer(CRC32_t* pulCRC, void* p, int len)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCRC32_ProcessBuffer:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCRC32_ProcessBuffer)(pulCRC, p, len);
}

void pfnCRC32_ProcessByte(CRC32_t* pulCRC, unsigned char ch)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCRC32_ProcessByte:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCRC32_ProcessByte)(pulCRC, ch);
}

CRC32_t pfnCRC32_Final(CRC32_t pulCRC)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCRC32_Final:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnCRC32_Final)(pulCRC);
}

int32 pfnRandomLong(int32 lLow, int32 lHigh)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnRandomLong: lLow=%d lHigh=%d\n",lLow,lHigh);
    //   fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnRandomLong)(lLow, lHigh);
}

float pfnRandomFloat(float flLow, float flHigh)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnRandomFloat:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnRandomFloat)(flLow, flHigh);
}

void pfnSetView(const edict_t* pClient, const edict_t* pViewent)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSetView:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetView)(pClient, pViewent);
}

float pfnTime(void)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnTime:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnTime)();
}

void pfnCrosshairAngle(const edict_t* pClient, float pitch, float yaw)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCrosshairAngle:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCrosshairAngle)(pClient, pitch, yaw);
}

byte* pfnLoadFileForMe(char* filename, int* pLength)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnLoadFileForMe: filename=%s\n", filename);
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnLoadFileForMe)(filename, pLength);
}

void pfnFreeFile(void* buffer)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnFreeFile:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnFreeFile)(buffer);
}

void pfnEndSection(const char* pszSectionName)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnEndSection:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnEndSection)(pszSectionName);
}

int pfnCompareFileTime(char* filename1, char* filename2, int* iCompare)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCompareFileTime:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnCompareFileTime)(filename1, filename2, iCompare);
}

void pfnGetGameDir(char* szGetGameDir)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnGetGameDir:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnGetGameDir)(szGetGameDir);
}

void pfnCvar_RegisterVariable(cvar_t* variable)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCvar_RegisterVariable:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCvar_RegisterVariable)(variable);
}

void pfnFadeClientVolume(const edict_t* pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnFadeClientVolume:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnFadeClientVolume)(pEdict, fadePercent, fadeOutSeconds, holdTime, fadeInSeconds);
}

void pfnSetClientMaxspeed(const edict_t* pEdict, float fNewMaxspeed)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSetClientMaxspeed: edict=%p %f\n", (void*)pEdict, fNewMaxspeed);
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
}

edict_t* pfnCreateFakeClient(const char* netname)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCreateFakeClient:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnCreateFakeClient)(netname);
}

void pfnRunPlayerMove(edict_t* fakeclient,
    const float* viewangles,
    float forwardmove,
    float sidemove,
    float upmove,
    unsigned short buttons,
    byte impulse,
    byte msec)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnRunPlayerMove:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnRunPlayerMove)(fakeclient, viewangles, forwardmove, sidemove, upmove, buttons, impulse, msec);
}

int pfnNumberOfEntities(void)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnNumberOfEntities:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnNumberOfEntities)();
}

char* pfnGetInfoKeyBuffer(edict_t* e)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnGetInfoKeyBuffer:\n");
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnGetInfoKeyBuffer:\n");
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnGetInfoKeyBuffer)(e);
}

char* pfnInfoKeyValue(char* infobuffer, char* key)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnInfoKeyValue: %s %s\n", infobuffer, key);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnInfoKeyValue: %s %s\n",infobuffer,key);
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnInfoKeyValue)(infobuffer, key);
}

void pfnSetKeyValue(char* infobuffer, char* key, char* value)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSetKeyValue: %s %s\n", key, value);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnSetKeyValue: %s %s\n",key,value);
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetKeyValue)(infobuffer, key, value);
}

void pfnSetClientKeyValue(int clientIndex, char* infobuffer, char* key, char* value)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnSetClientKeyValue: %s %s\n", key, value);
        fclose(fp);
    }
    // _snprintf(sz_error_check,250,"pfnSetClientKeyValue: %s %s\n",key,value);
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
}

int pfnIsMapValid(char* filename)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnIsMapValid:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnIsMapValid)(filename);
}

void pfnStaticDecal(const float* origin, int decalIndex, int entityIndex, int modelIndex)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnStaticDecal:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnStaticDecal)(origin, decalIndex, entityIndex, modelIndex);
}

int pfnPrecacheGeneric(char* s)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnPrecacheGeneric: %s\n", s);
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnPrecacheGeneric)(s);
}

int pfnGetPlayerUserId(edict_t* e)
{
    if(gpGlobals->deathmatch) {
        if(debug_engine) {
            fp = UTIL_OpenFoxbotLog();
            fprintf(fp, "pfnGetPlayerUserId: %p\n", (void*)e);
            fclose(fp);
        }
        // _snprintf(sz_error_check,250,"pfnGetPlayerUserId: %x\n",e);

        /*if(mod_id == GEARBOX_DLL) {
            // is this edict a bot?
            if(UTIL_GetBotPointer(e)) {
                if(mr_meta)
                    RETURN_META_VALUE(MRES_SUPERCEDE, 0);
                return 0; // don't return a valid index (so bot won't get kicked)
            }
        }*/
    }

    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnGetPlayerUserId)(e);
}

void pfnBuildSoundMsg(edict_t* entity,
    int channel,
    const char* sample,
    /*int*/ float volume,
    float attenuation,
    int fFlags,
    int pitch,
    int msg_dest,
    int msg_type,
    const float* pOrigin,
    edict_t* ed)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnBuildSoundMsg:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnBuildSoundMsg)(
        entity, channel, sample, volume, attenuation, fFlags, pitch, msg_dest, msg_type, pOrigin, ed);
}

int pfnIsDedicatedServer(void)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnIsDedicatedServer:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnIsDedicatedServer)();
}

cvar_t* pfnCVarGetPointer(const char* szVarName)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnCVarGetPointer: %s\n", szVarName);
            fclose(fp);
        }
    }
    // _snprintf(sz_error_check,250,"pfnCVarGetPointer: %s\n",szVarName);
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnCVarGetPointer)(szVarName);
}

unsigned int pfnGetPlayerWONId(edict_t* e)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnGetPlayerWONId: %p\n", (void*)e);
            fclose(fp);
        }
    }
    // _snprintf(sz_error_check,250,"pfnGetPlayerWONId: %x\n",e);
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnGetPlayerWONId)(e);
}

// new stuff for SDK 2.0

void pfnInfo_RemoveKey(char* s, const char* key)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnInfo_RemoveKey:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnInfo_RemoveKey)(s, key);
}

const char* pfnGetPhysicsKeyValue(const edict_t* pClient, const char* key)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnGetPhysicsKeyValue:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnGetPhysicsKeyValue)(pClient, key);
}

void pfnSetPhysicsKeyValue(const edict_t* pClient, const char* key, const char* value)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnSetPhysicsKeyValue:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetPhysicsKeyValue)(pClient, key, value);
}

const char* pfnGetPhysicsInfoString(const edict_t* pClient)
{
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnGetPhysicsInfoString)(pClient);
}

unsigned short pfnPrecacheEvent(int type, const char* psz)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnPrecacheEvent:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnPrecacheEvent)(type, psz);
}

void pfnPlaybackEvent(int flags,
    const edict_t* pInvoker,
    unsigned short eventindex,
    float delay,
    float* origin,
    float* angles,
    float fparam1,
    float fparam2,
    int iparam1,
    int iparam2,
    int bparam1,
    int bparam2)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnPlaybackEvent: %d %p %d %f (%f %f %f) (%f %f %f) %f %f %d %d %d %d\n", flags,
                (void*)pInvoker, eventindex, delay, (*(Vector*)origin).x, (*(Vector*)origin).y, (*(Vector*)origin).z,
                (*(Vector*)angles).x, (*(Vector*)angles).y, (*(Vector*)angles).z, fparam1, fparam2, iparam1, iparam2,
                bparam1, bparam2);
            fclose(fp);
        }
        // delay=delay+2;
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnPlaybackEvent)(
        flags, pInvoker, eventindex, delay, origin, angles, fparam1, fparam2, iparam1, iparam2, bparam1, bparam2);
}

unsigned char* pfnSetFatPVS(float* org)
{
    // if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnSetFatPVS:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnSetFatPVS)(org);
}

unsigned char* pfnSetFatPAS(float* org)
{
    // if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnSetFatPAS:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, NULL);
    return (*g_engfuncs.pfnSetFatPAS)(org);
}

int pfnCheckVisibility(const edict_t* entity, unsigned char* pset)
{
    //   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnCheckVisibility:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnCheckVisibility)(entity, pset);
}

void pfnDeltaSetField(struct delta_s* pFields, const char* fieldname)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnDeltaSetField:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnDeltaSetField)(pFields, fieldname);
}

void pfnDeltaUnsetField(struct delta_s* pFields, const char* fieldname)
{
    // if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnDeltaUnsetField:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnDeltaUnsetField)(pFields, fieldname);
}

void pfnDeltaAddEncoder(char* name,
    void (*conditionalencode)(struct delta_s* pFields, const unsigned char* from, const unsigned char* to))
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnDeltaAddEncoder:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnDeltaAddEncoder)(name, conditionalencode);
}

int pfnGetCurrentPlayer(void)
{
    // if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnGetCurrentPlayer:\n"); fclose(fp);}
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnGetCurrentPlayer)();
}

int pfnCanSkipPlayer(const edict_t* player)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        fprintf(fp, "pfnCanSkipPlayer:\n");
        fclose(fp);
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnCanSkipPlayer)(player);
}

int pfnDeltaFindField(struct delta_s* pFields, const char* fieldname)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnDeltaFindField:\n");
            fclose(fp);
        }
    }

    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnDeltaFindField)(pFields, fieldname);
}

void pfnDeltaSetFieldByIndex(struct delta_s* pFields, int fieldNumber)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnDeltaSetFieldByIndex:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnDeltaSetFieldByIndex)(pFields, fieldNumber);
}

void pfnDeltaUnsetFieldByIndex(struct delta_s* pFields, int fieldNumber)
{
    // if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnDeltaUnsetFieldByIndex:\n"); fclose(fp); }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnDeltaUnsetFieldByIndex)(pFields, fieldNumber);
}

void pfnSetGroupMask(int mask, int op)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnSetGroupMask:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnSetGroupMask)(mask, op);
}

int pfnCreateInstancedBaseline(int classname, struct entity_state_s* baseline)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnCreateInstancedBaseline:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META_VALUE(MRES_HANDLED, 0);
    return (*g_engfuncs.pfnCreateInstancedBaseline)(classname, baseline);
}

void pfnCvar_DirectSet(struct cvar_s* var, char* value)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnCvar_DirectSet: %s %s\n", var->name, value);
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnCvar_DirectSet)(var, value);
}

void pfnForceUnmodified(FORCE_TYPE type, float* mins, float* maxs, const char* filename)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnForceUnmodified:\n");
            fclose(fp);
        }
    }
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnForceUnmodified)(type, mins, maxs, filename);
}

void pfnGetPlayerStats(const edict_t* pClient, int* ping, int* packet_loss)
{
    if(debug_engine) {
        fp = UTIL_OpenFoxbotLog();
        if(fp != NULL) {
            fprintf(fp, "pfnGetPlayerStats: %p %p %p\n", (void*)pClient, (void*)ping, (void*)packet_loss);
            fclose(fp);
        }
    }
    /*if(pClient!=NULL)
       {
       bool b=true;
       for(int i=0;i<32;i++)
       {
       if(bots[i].pEdict==pClient)
       b=false;
       }
       //if(b)
       //{
       //   if(GETPLAYERWONID(pClient)==0) b=false;
       //}
       if(!b)
       {
     *ping=RANDOM_LONG(100,200);
       if(mr_meta) RETURN_META(MRES_SUPERCEDE);
       }
       }
       else*/
    //{
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnGetPlayerStats)(pClient, ping, packet_loss);
    //}
}

// idea for making meta mod work..add these engine calls

void pfnAlertMessage(ALERT_TYPE atype, char* szFmt, ...)
{
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnAlertMessage)(atype, szFmt);
}

void pfnEngineFprintf(void* pfile, char* szFmt, ...)
{
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnEngineFprintf)(pfile, szFmt);
}

void pfnAddServerCommand(char* cmd_name, void (*function)(void))
{
    if(mr_meta)
        RETURN_META(MRES_HANDLED);
    (*g_engfuncs.pfnAddServerCommand)(cmd_name, function);
}

/*const char* pfnGetPlayerAuthID (edict_t *e)
   {
   if(debug_engine) { fp=UTIL_OpenFoxbotLog(); fprintf(fp,"pfnGetPlayerAuthID: %x\n",e); fclose(fp); }
   if(mr_meta) RETURN_META_VALUE(MRES_HANDLED,0);
   return (*g_engfuncs.pfnGetPlayerAuthID)(e);
   }*/

enginefuncs_t meta_engfuncs = {
    pfnPrecacheModel, // pfnPrecacheModel()
    pfnPrecacheSound, // pfnPrecacheSound()
    pfnSetModel,      // pfnSetModel()
    pfnModelIndex,    // pfnModelIndex()
    pfnModelFrames,   // pfnModelFrames()

    pfnSetSize,        // pfnSetSize()
    pfnChangeLevel,    // pfnChangeLevel()
    pfnGetSpawnParms,  // pfnGetSpawnParms()
    pfnSaveSpawnParms, // pfnSaveSpawnParms()

    pfnVecToYaw,     // pfnVecToYaw()
    pfnVecToAngles,  // pfnVecToAngles()
    pfnMoveToOrigin, // pfnMoveToOrigin()
    pfnChangeYaw,    // pfnChangeYaw()
    pfnChangePitch,  // pfnChangePitch()

    pfnFindEntityByString, // pfnFindEntityByString()
    pfnGetEntityIllum,     // pfnGetEntityIllum()
    pfnFindEntityInSphere, // pfnFindEntityInSphere()
    pfnFindClientInPVS,    // pfnFindClientInPVS()
    pfnEntitiesInPVS,      // pfnEntitiesInPVS()

    pfnMakeVectors,  // pfnMakeVectors()
    pfnAngleVectors, // pfnAngleVectors()

    NULL,            // pfnCreateEntity()
    pfnRemoveEntity, // pfnRemoveEntity()
    NULL,            // pfnCreateNamedEntity()

    pfnMakeStatic,   // pfnMakeStatic()
    pfnEntIsOnFloor, // pfnEntIsOnFloor()
    pfnDropToFloor,  // pfnDropToFloor()

    pfnWalkMove,  // pfnWalkMove()
    pfnSetOrigin, // pfnSetOrigin()

    pfnEmitSound,        // pfnEmitSound()
    pfnEmitAmbientSound, // pfnEmitAmbientSound()

    pfnTraceLine,        // pfnTraceLine()
    pfnTraceToss,        // pfnTraceToss()
    pfnTraceMonsterHull, // pfnTraceMonsterHull()
    pfnTraceHull,        // pfnTraceHull()
    pfnTraceModel,       // pfnTraceModel()
    pfnTraceTexture,     // pfnTraceTexture()
    pfnTraceSphere,      // pfnTraceSphere()
    pfnGetAimVector,     // pfnGetAimVector()

    pfnServerCommand, // pfnServerCommand()
    pfnServerExecute, // pfnServerExecute()
    pfnClCom,         // pfnClientCommand,		// pfnClientCommand()	// d'oh, ClientCommand in dllapi too

    pfnParticleEffect, // pfnParticleEffect()
    pfnLightStyle,     // pfnLightStyle()
    pfnDecalIndex,     // pfnDecalIndex()
    pfnPointContents,  // pfnPointContents()

    MessageBegin, // pfnMessageBegin()
    MessageEnd,   // pfnMessageEnd()

    WriteByte,   // pfnWriteByte()
    WriteChar,   // pfnWriteChar()
    WriteShort,  // pfnWriteShort()
    WriteLong,   // pfnWriteLong()
    WriteAngle,  // pfnWriteAngle()
    WriteCoord,  // pfnWriteCoord()
    WriteString, // pfnWriteString()
    WriteEntity, // pfnWriteEntity()

    NULL,             // pfnCVarRegister,			// pfnCVarRegister()
    pfnCVarGetFloat,  // pfnCVarGetFloat()
    pfnCVarGetString, // pfnCVarGetString()
    pfnCVarSetFloat,  // pfnCVarSetFloat()
    pfnCVarSetString, // pfnCVarSetString()

    pfnAlertMessage,  // pfnAlertMessage()
    pfnEngineFprintf, // pfnEngineFprintf()

    pfnPvAllocEntPrivateData, // pfnPvAllocEntPrivateData()
    pfnPvEntPrivateData,      // pfnPvEntPrivateData()
    pfnFreeEntPrivateData,    // pfnFreeEntPrivateData()

    pfnSzFromIndex, // pfnSzFromIndex()
    pfnAllocString, // pfnAllocString()

    pfnGetVarsOfEnt,       // pfnGetVarsOfEnt()
    pfnPEntityOfEntOffset, // pfnPEntityOfEntOffset()
    pfnEntOffsetOfPEntity, // pfnEntOffsetOfPEntity()
    pfnIndexOfEdict,       // pfnIndexOfEdict()
    pfnPEntityOfEntIndex,  // pfnPEntityOfEntIndex()
    pfnFindEntityByVars,   // pfnFindEntityByVars()
    pfnGetModelPtr,        // pfnGetModelPtr()

    NULL, // pfnRegUserMsg()

    pfnAnimationAutomove, // pfnAnimationAutomove()
    pfnGetBonePosition,   // pfnGetBonePosition()

    pfnFunctionFromName, // pfnFunctionFromName()
    pfnNameForFunction,  // pfnNameForFunction()

    pfnClPrintf,    // pfnClientPrintf()			//! JOHN: engine callbacks so game DLL can print messages
                    // to individual clients
    pfnServerPrint, // pfnServerPrint()

    Cmd_Args, // pfnCmd_Args()				//! these 3 added
    Cmd_Argv, // pfnCmd_Argv()				//! so game DLL can easily
    Cmd_Argc, // pfnCmd_Argc()				//! access client 'cmd' strings

    pfnGetAttachment, // pfnGetAttachment()

    pfnCRC32_Init,          // pfnCRC32_Init()
    pfnCRC32_ProcessBuffer, // pfnCRC32_ProcessBuffer()
    pfnCRC32_ProcessByte,   // pfnCRC32_ProcessByte()
    pfnCRC32_Final,         // pfnCRC32_Final()

    pfnRandomLong,  // pfnRandomLong()
    pfnRandomFloat, // pfnRandomFloat()

    pfnSetView,        // pfnSetView()
    pfnTime,           // pfnTime()
    pfnCrosshairAngle, // pfnCrosshairAngle()

    pfnLoadFileForMe, // pfnLoadFileForMe()
    pfnFreeFile,      // pfnFreeFile()

    pfnEndSection,            // pfnEndSection()				//! trigger_endsection
    pfnCompareFileTime,       // pfnCompareFileTime()
    pfnGetGameDir,            // pfnGetGameDir()
    pfnCvar_RegisterVariable, // pfnCvar_RegisterVariable()
    pfnFadeClientVolume,      // pfnFadeClientVolume()
    pfnSetClientMaxspeed,     // pfnSetClientMaxspeed()
    pfnCreateFakeClient,      // pfnCreateFakeClient()                //! returns NULL if fake client can't be created
    pfnRunPlayerMove,         // pfnRunPlayerMove()
    pfnNumberOfEntities,      // pfnNumberOfEntities()

    pfnGetInfoKeyBuffer,  // pfnGetInfoKeyBuffer()		//! passing in NULL gets the serverinfo
    pfnInfoKeyValue,      // pfnInfoKeyValue()
    pfnSetKeyValue,       // pfnSetKeyValue()
    pfnSetClientKeyValue, // pfnSetClientKeyValue()

    pfnIsMapValid,        // pfnIsMapValid()
    pfnStaticDecal,       // pfnStaticDecal()
    pfnPrecacheGeneric,   // pfnPrecacheGeneric()
    pfnGetPlayerUserId,   // pfnGetPlayerUserId()			//! returns the server assigned userid for
                          // this player.
    pfnBuildSoundMsg,     // pfnBuildSoundMsg()
    pfnIsDedicatedServer, // pfnIsDedicatedServer()		//! is this a dedicated server?
    pfnCVarGetPointer,    // pfnCVarGetPointer()
    pfnGetPlayerWONId,    // pfnGetPlayerWONId()			//! returns the server assigned WONid for
                          // this player.

    //! YWB 8/1/99 TFF Physics additions
    pfnInfo_RemoveKey,       // pfnInfo_RemoveKey()
    pfnGetPhysicsKeyValue,   // pfnGetPhysicsKeyValue()
    pfnSetPhysicsKeyValue,   // pfnSetPhysicsKeyValue()
    pfnGetPhysicsInfoString, // pfnGetPhysicsInfoString()
    pfnPrecacheEvent,        // pfnPrecacheEvent()
    pfnPlaybackEvent,        // pfnPlaybackEvent()

    pfnSetFatPVS, // pfnSetFatPVS()
    pfnSetFatPAS, // pfnSetFatPAS()

    pfnCheckVisibility, // pfnCheckVisibility()

    pfnDeltaSetField,          // pfnDeltaSetField()
    pfnDeltaUnsetField,        // pfnDeltaUnsetField()
    pfnDeltaAddEncoder,        // pfnDeltaAddEncoder()
    pfnGetCurrentPlayer,       // pfnGetCurrentPlayer()
    pfnCanSkipPlayer,          // pfnCanSkipPlayer()
    pfnDeltaFindField,         // pfnDeltaFindField()
    pfnDeltaSetFieldByIndex,   // pfnDeltaSetFieldByIndex()
    pfnDeltaUnsetFieldByIndex, // pfnDeltaUnsetFieldByIndex()

    pfnSetGroupMask, // pfnSetGroupMask()

    pfnCreateInstancedBaseline, // pfnCreateInstancedBaseline()		// d'oh, CreateInstancedBaseline in
                                // dllapi too
    pfnCvar_DirectSet,          // pfnCvar_DirectSet()

    pfnForceUnmodified, // pfnForceUnmodified()

    pfnGetPlayerStats, // pfnGetPlayerStats()

    pfnAddServerCommand, // pfnAddServerCommand()
    NULL, NULL,
    NULL, // pfnGetPlayerAuthID,
};

C_DLLEXPORT int GetEngineFunctions(enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion)
{
    if(!pengfuncsFromEngine) {
        // LOG_ERROR(PLID, "GetEngineFunctions called with null pengfuncsFromEngine");
        return (FALSE);
    } else if(*interfaceVersion != ENGINE_INTERFACE_VERSION) {
        // LOG_ERROR(PLID, "GetEngineFunctions version mismatch; requested=%d ours=%d", *interfaceVersion,
        // ENGINE_INTERFACE_VERSION);
        // Tell metamod what version we had, so it can figure out who is out of date.
        *interfaceVersion = ENGINE_INTERFACE_VERSION;
        return (FALSE);
    }
    memcpy(pengfuncsFromEngine, &meta_engfuncs, sizeof(enginefuncs_t));
    return TRUE;
}

enginefuncs_t meta_engfuncs_post = {
    NULL, // pfnPrecacheModel()
    NULL, // pfnPrecacheSound()
    NULL, // pfnSetModel()
    NULL, // pfnModelIndex()
    NULL, // pfnModelFrames()

    NULL, // pfnSetSize()
    NULL, // pfnChangeLevel()
    NULL, // pfnGetSpawnParms()
    NULL, // pfnSaveSpawnParms()

    NULL, // pfnVecToYaw()
    NULL, // pfnVecToAngles()
    NULL, // pfnMoveToOrigin()
    NULL, // pfnChangeYaw()
    NULL, // pfnChangePitch()

    NULL, // pfnFindEntityByString()
    NULL, // pfnGetEntityIllum()
    NULL, // pfnFindEntityInSphere()
    NULL, // pfnFindClientInPVS()
    NULL, // pfnEntitiesInPVS()

    NULL, // pfnMakeVectors()
    NULL, // pfnAngleVectors()

    pfnCreateEntity_Post,      // pfnCreateEntity()
    NULL,                      // pfnRemoveEntity()
    pfnCreateNamedEntity_Post, // pfnCreateNamedEntity()

    NULL, // pfnMakeStatic()
    NULL, // pfnEntIsOnFloor()
    NULL, // pfnDropToFloor()

    NULL, // pfnWalkMove()
    NULL, // pfnSetOrigin()

    NULL, // pfnEmitSound()
    NULL, // pfnEmitAmbientSound()

    NULL, // pfnTraceLine()
    NULL, // pfnTraceToss()
    NULL, // pfnTraceMonsterHull()
    NULL, // pfnTraceHull()
    NULL, // pfnTraceModel()
    NULL, // pfnTraceTexture()
    NULL, // pfnTraceSphere()
    NULL, // pfnGetAimVector()

    NULL, // pfnServerCommand()
    NULL, // pfnServerExecute()
    NULL, // pfnClientCommand()

    NULL, // pfnParticleEffect()
    NULL, // pfnLightStyle()
    NULL, // pfnDecalIndex()
    NULL, // pfnPointContents()

    NULL, // pfnMessageBegin()
    NULL, // pfnMessageEnd()

    NULL, // pfnWriteByte()
    NULL, // pfnWriteChar()
    NULL, // pfnWriteShort()
    NULL, // pfnWriteLong()
    NULL, // pfnWriteAngle()
    NULL, // pfnWriteCoord()
    NULL, // pfnWriteString()
    NULL, // pfnWriteEntity()

    NULL, // pfnCVarRegister()
    NULL, // pfnCVarGetFloat()
    NULL, // pfnCVarGetString()
    NULL, // pfnCVarSetFloat()
    NULL, // pfnCVarSetString()

    NULL, // pfnAlertMessage()
    NULL, // pfnEngineFprintf()

    NULL, // pfnPvAllocEntPrivateData()
    NULL, // pfnPvEntPrivateData()
    NULL, // pfnFreeEntPrivateData()

    NULL, // pfnSzFromIndex()
    NULL, // pfnAllocString()

    NULL, // pfnGetVarsOfEnt()
    NULL, // pfnPEntityOfEntOffset()
    NULL, // pfnEntOffsetOfPEntity()
    NULL, // pfnIndexOfEdict()
    NULL, // pfnPEntityOfEntIndex()
    NULL, // pfnFindEntityByVars()
    NULL, // pfnGetModelPtr()

    pfnRegUserMsg, // pfnRegUserMsg()

    NULL, // pfnAnimationAutomove()
    NULL, // pfnGetBonePosition()

    NULL, // pfnFunctionFromName()
    NULL, // pfnNameForFunction()

    NULL, // pfnClientPrintf()			//! JOHN: engine callbacks so game DLL can print messages to individual
          // clients
    NULL, // pfnServerPrint()

    NULL, // pfnCmd_Args()	//! these 3 added
    NULL, // pfnCmd_Argv()	//! so game DLL can easily
    NULL, // pfnCmd_Argc()	//! access client 'cmd' strings

    NULL, // pfnGetAttachment()

    NULL, // pfnCRC32_Init()
    NULL, // pfnCRC32_ProcessBuffer()
    NULL, // pfnCRC32_ProcessByte()
    NULL, // pfnCRC32_Final()

    NULL, // pfnRandomLong()
    NULL, // pfnRandomFloat()

    NULL, // pfnSetView()
    NULL, // pfnTime()
    NULL, // pfnCrosshairAngle()

    NULL, // pfnLoadFileForMe()
    NULL, // pfnFreeFile()

    NULL, // pfnEndSection()				//! trigger_endsection
    NULL, // pfnCompareFileTime()
    NULL, // pfnGetGameDir()
    NULL, // pfnCvar_RegisterVariable()
    NULL, // pfnFadeClientVolume()
    NULL, // pfnSetClientMaxspeed()
    NULL, // pfnCreateFakeClient()                //! returns NULL if fake client can't be created
    NULL, // pfnRunPlayerMove()
    NULL, // pfnNumberOfEntities()

    NULL, // pfnGetInfoKeyBuffer()		//! passing in NULL gets the serverinfo
    NULL, // pfnInfoKeyValue()
    NULL, // pfnSetKeyValue()
    NULL, // pfnSetClientKeyValue()

    NULL, // pfnIsMapValid()
    NULL, // pfnStaticDecal()
    NULL, // pfnPrecacheGeneric()
    NULL, // pfnGetPlayerUserId()			//! returns the server assigned userid for this player.
    NULL, // pfnBuildSoundMsg()
    NULL, // pfnIsDedicatedServer()		//! is this a dedicated server?
    NULL, // pfnCVarGetPointer()
    NULL, // pfnGetPlayerWONId()			//! returns the server assigned WONid for this player.

    //! YWB 8/1/99 TFF Physics additions
    NULL, // pfnInfo_RemoveKey()
    NULL, // pfnGetPhysicsKeyValue()
    NULL, // pfnSetPhysicsKeyValue()
    NULL, // pfnGetPhysicsInfoString()
    NULL, // pfnPrecacheEvent()
    NULL, // pfnPlaybackEvent()

    NULL, // pfnSetFatPVS()
    NULL, // pfnSetFatPAS()

    NULL, // pfnCheckVisibility()

    NULL, // pfnDeltaSetField()
    NULL, // pfnDeltaUnsetField()
    NULL, // pfnDeltaAddEncoder()
    NULL, // pfnGetCurrentPlayer()
    NULL, // pfnCanSkipPlayer()
    NULL, // pfnDeltaFindField()
    NULL, // pfnDeltaSetFieldByIndex()
    NULL, // pfnDeltaUnsetFieldByIndex()

    NULL, // pfnSetGroupMask()

    NULL, // pfnCreateInstancedBaseline()		// d'oh, CreateInstancedBaseline in dllapi too
    NULL, // pfnCvar_DirectSet()

    NULL, // pfnForceUnmodified()

    NULL, // pfnGetPlayerStats()

    NULL, // pfnAddServerCommand()

    NULL, NULL, NULL,
};

int GetEngineFunctions_Post(enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion)
{
    if(!pengfuncsFromEngine) {
        UTIL_LogPrintf("%s: GetEngineFunctions called with null pengfuncsFromEngine", Plugin_info.logtag);
        return (FALSE);
    } else if(*interfaceVersion != ENGINE_INTERFACE_VERSION) {
        UTIL_LogPrintf("%s: GetEngineFunctions version mismatch; requested=%d ours=%d", Plugin_info.logtag,
            *interfaceVersion, ENGINE_INTERFACE_VERSION);

        // Tell metamod what version we had, so it can figure out who is
        // out of date.
        *interfaceVersion = ENGINE_INTERFACE_VERSION;
        return (FALSE);
    }

    memcpy(pengfuncsFromEngine, &meta_engfuncs_post, sizeof(enginefuncs_t));
    return TRUE;
}
