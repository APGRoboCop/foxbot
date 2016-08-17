//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// h_export.cpp
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
#include "enginecallback.h"
//#include "util.h"
//#include "cbase.h"

#include "bot.h"
#include "engine.h"

// meta mod stuff
#include <h_export.h> // me
#include <meta_api.h>

extern bool mr_meta;
// extern engine_t Engine;

#ifndef __linux__
HINSTANCE h_Library = NULL;
HGLOBAL h_global_argv = NULL;
#else
void* h_Library = NULL;
char h_global_argv[1024];
#endif

enginefuncs_t g_engfuncs;
globalvars_t* gpGlobals;
char* g_argv;

// static FILE *fp;

GETENTITYAPI other_GetEntityAPI = NULL;
GETNEWDLLFUNCTIONS other_GetNewDLLFunctions = NULL;
GIVEFNPTRSTODLL other_GiveFnptrsToDll = NULL;

extern int mod_id;

#ifndef __linux__
// Required DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH) {
    } else if(fdwReason == DLL_PROCESS_DETACH) {
        if(h_Library)
            FreeLibrary(h_Library);

        if(h_global_argv) {
            GlobalUnlock(h_global_argv);
            GlobalFree(h_global_argv);
        }
    }

    return TRUE;
}
#endif

/*
#ifndef __linux__
#ifdef __BORLANDC__
extern "C" DLLEXPORT void EXPORT GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals)
#else
void DLLEXPORT GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
#endif
#else
//origornally wif no void EXPORT
extern "C" DLLEXPORT void EXPORT GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
#endif
*/

void WINAPI GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals)
{
    int pos;
    char game_dir[256];
    char mod_name[32];

    // get the engine functions from the engine...

    memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
    gpGlobals = pGlobals;

    // find the directory name of the currently running MOD...
    (*g_engfuncs.pfnGetGameDir)(game_dir);

    pos = 0;
    if(strstr(game_dir, "/") != NULL) {
        pos = strlen(game_dir) - 1;
        // scan backwards till first directory separator...

        while((pos) && (game_dir[pos] != '/'))
            pos--;
        if(pos == 0) {
            // Error getting directory name!
            ALERT(at_error, "FoxBot - Error determining MOD directory name!");
        }
        pos++;
    }
    strcpy(mod_name, &game_dir[pos]);

    if(strcmpi(mod_name, "valve") == 0) {
        mod_id = VALVE_DLL;
#ifndef __linux__
        h_Library = LoadLibrary("valve\\dlls\\hl.dll");
#else
        h_Library = dlopen("valve/dlls/hl.so", RTLD_NOW);
#endif
    } else if(strcmpi(mod_name, "tfc") == 0) {
        mod_id = TFC_DLL;
#ifndef __linux__
        h_Library = LoadLibrary("tfc\\dlls\\tfc.dll");
#else
        h_Library = dlopen("tfc/dlls/tfc.so", RTLD_NOW);
#endif
    }
    // Not required for TFC - [APG]RoboCop[CL]
    /* else if(strcmpi(mod_name, "cstrike") == 0) {
            mod_id = CSTRIKE_DLL;
    #ifndef __linux__
            h_Library = LoadLibrary("cstrike\\dlls\\mp.dll");
    #else
            h_Library = dlopen("cstrike/dlls/cs.so", RTLD_NOW);
    #endif
        } else if(strcmpi(mod_name, "gearbox") == 0) {
            mod_id = GEARBOX_DLL;
    #ifndef __linux__
            h_Library = LoadLibrary("gearbox\\dlls\\opfor.dll");
    #else
            h_Library = dlopen("gearbox/dlls/opfor.so", RTLD_NOW);
    #endif
        } else if(strcmpi(mod_name, "frontline") == 0) {
            mod_id = FRONTLINE_DLL;
    #ifndef __linux__
            h_Library = LoadLibrary("frontline\\dlls\\frontline.dll");
    #else
            h_Library = dlopen("frontline/dlls/front.so", RTLD_NOW);
    #endif
        }
    */
    if(h_Library == NULL) {
        // Directory error or Unsupported MOD!

        ALERT(at_error, "FoXBot - MOD dll not found (or unsupported MOD)!");
    }
#ifndef __linux__
    h_global_argv = GlobalAlloc(GMEM_SHARE, 1024);
    g_argv = (char*)GlobalLock(h_global_argv);
#else
    g_argv = (char*)h_global_argv;
#endif

    if(!mr_meta) {
        other_GetEntityAPI = (GETENTITYAPI)GetProcAddress(h_Library, "GetEntityAPI");

        if(other_GetEntityAPI == NULL) {
            // Can't find GetEntityAPI!

            ALERT(at_error, "FoXBot - Can't get MOD's GetEntityAPI!");
        }

        other_GetNewDLLFunctions = (GETNEWDLLFUNCTIONS)GetProcAddress(h_Library, "GetNewDLLFunctions");

        //	if (other_GetNewDLLFunctions == NULL)
        //	{
        //		// Can't find GetNewDLLFunctions!
        //
        //		ALERT( at_error, "FoXBot - Can't get MOD's GetNewDLLFunctions!" );
        //	}

        other_GiveFnptrsToDll = (GIVEFNPTRSTODLL)GetProcAddress(h_Library, "GiveFnptrsToDll");

        if(other_GiveFnptrsToDll == NULL) {
            // Can't find GiveFnptrsToDll!

            ALERT(at_error, "FoXBot - Can't get MOD's GiveFnptrsToDll!");
        }

        // admin does this again here..so..
        // memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));

        // gpGlobals = pGlobals;

        // if(!mr_meta)
        //{
        pengfuncsFromEngine->pfnCmd_Args = Cmd_Args;
        pengfuncsFromEngine->pfnCmd_Argv = Cmd_Argv;
        pengfuncsFromEngine->pfnCmd_Argc = Cmd_Argc;

        pengfuncsFromEngine->pfnPrecacheModel = pfnPrecacheModel;
        pengfuncsFromEngine->pfnPrecacheSound = pfnPrecacheSound;
        pengfuncsFromEngine->pfnSetModel = pfnSetModel;
        pengfuncsFromEngine->pfnModelIndex = pfnModelIndex;
        pengfuncsFromEngine->pfnModelFrames = pfnModelFrames;
        pengfuncsFromEngine->pfnSetSize = pfnSetSize;
        pengfuncsFromEngine->pfnChangeLevel = pfnChangeLevel;
        pengfuncsFromEngine->pfnGetSpawnParms = pfnGetSpawnParms;
        pengfuncsFromEngine->pfnSaveSpawnParms = pfnSaveSpawnParms;
        pengfuncsFromEngine->pfnVecToYaw = pfnVecToYaw;
        pengfuncsFromEngine->pfnVecToAngles = pfnVecToAngles;
        pengfuncsFromEngine->pfnMoveToOrigin = pfnMoveToOrigin;
        pengfuncsFromEngine->pfnChangeYaw = pfnChangeYaw;
        pengfuncsFromEngine->pfnChangePitch = pfnChangePitch;
        pengfuncsFromEngine->pfnFindEntityByString = pfnFindEntityByString;
        pengfuncsFromEngine->pfnGetEntityIllum = pfnGetEntityIllum;
        pengfuncsFromEngine->pfnFindEntityInSphere = pfnFindEntityInSphere;
        pengfuncsFromEngine->pfnFindClientInPVS = pfnFindClientInPVS;
        pengfuncsFromEngine->pfnEntitiesInPVS = pfnEntitiesInPVS;
        pengfuncsFromEngine->pfnMakeVectors = pfnMakeVectors;
        pengfuncsFromEngine->pfnAngleVectors = pfnAngleVectors;
        pengfuncsFromEngine->pfnCreateEntity = pfnCreateEntity;
        pengfuncsFromEngine->pfnRemoveEntity = pfnRemoveEntity;
        pengfuncsFromEngine->pfnCreateNamedEntity = pfnCreateNamedEntity;
        pengfuncsFromEngine->pfnMakeStatic = pfnMakeStatic;
        pengfuncsFromEngine->pfnEntIsOnFloor = pfnEntIsOnFloor;
        pengfuncsFromEngine->pfnDropToFloor = pfnDropToFloor;
        pengfuncsFromEngine->pfnWalkMove = pfnWalkMove;
        pengfuncsFromEngine->pfnSetOrigin = pfnSetOrigin;
        pengfuncsFromEngine->pfnEmitSound = pfnEmitSound;
        pengfuncsFromEngine->pfnEmitAmbientSound = pfnEmitAmbientSound;
        pengfuncsFromEngine->pfnTraceLine = pfnTraceLine;
        pengfuncsFromEngine->pfnTraceToss = pfnTraceToss;
        pengfuncsFromEngine->pfnTraceMonsterHull = pfnTraceMonsterHull;
        pengfuncsFromEngine->pfnTraceHull = pfnTraceHull;
        pengfuncsFromEngine->pfnTraceModel = pfnTraceModel;
        pengfuncsFromEngine->pfnTraceTexture = pfnTraceTexture;
        pengfuncsFromEngine->pfnTraceSphere = pfnTraceSphere;
        pengfuncsFromEngine->pfnGetAimVector = pfnGetAimVector;
        pengfuncsFromEngine->pfnServerCommand = pfnServerCommand;
        pengfuncsFromEngine->pfnServerExecute = pfnServerExecute;

        pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;

        pengfuncsFromEngine->pfnParticleEffect = pfnParticleEffect;
        pengfuncsFromEngine->pfnLightStyle = pfnLightStyle;
        pengfuncsFromEngine->pfnDecalIndex = pfnDecalIndex;
        pengfuncsFromEngine->pfnPointContents = pfnPointContents;
        pengfuncsFromEngine->pfnMessageBegin = pfnMessageBegin;
        pengfuncsFromEngine->pfnMessageEnd = pfnMessageEnd;
        pengfuncsFromEngine->pfnWriteByte = pfnWriteByte;
        pengfuncsFromEngine->pfnWriteChar = pfnWriteChar;
        pengfuncsFromEngine->pfnWriteShort = pfnWriteShort;
        pengfuncsFromEngine->pfnWriteLong = pfnWriteLong;
        pengfuncsFromEngine->pfnWriteAngle = pfnWriteAngle;
        pengfuncsFromEngine->pfnWriteCoord = pfnWriteCoord;
        pengfuncsFromEngine->pfnWriteString = pfnWriteString;
        pengfuncsFromEngine->pfnWriteEntity = pfnWriteEntity;
        pengfuncsFromEngine->pfnCVarRegister = pfnCVarRegister;

        pengfuncsFromEngine->pfnCVarGetFloat = pfnCVarGetFloat;
        pengfuncsFromEngine->pfnCVarGetString = pfnCVarGetString;
        pengfuncsFromEngine->pfnCVarSetFloat = pfnCVarSetFloat;
        pengfuncsFromEngine->pfnCVarSetString = pfnCVarSetString;
        pengfuncsFromEngine->pfnPvAllocEntPrivateData = pfnPvAllocEntPrivateData;
        pengfuncsFromEngine->pfnPvEntPrivateData = pfnPvEntPrivateData;
        pengfuncsFromEngine->pfnFreeEntPrivateData = pfnFreeEntPrivateData;
        pengfuncsFromEngine->pfnSzFromIndex = pfnSzFromIndex;
        pengfuncsFromEngine->pfnAllocString = pfnAllocString;
        pengfuncsFromEngine->pfnGetVarsOfEnt = pfnGetVarsOfEnt;
        pengfuncsFromEngine->pfnPEntityOfEntOffset = pfnPEntityOfEntOffset;
        pengfuncsFromEngine->pfnEntOffsetOfPEntity = pfnEntOffsetOfPEntity;
        pengfuncsFromEngine->pfnIndexOfEdict = pfnIndexOfEdict;
        pengfuncsFromEngine->pfnPEntityOfEntIndex = pfnPEntityOfEntIndex;
        pengfuncsFromEngine->pfnFindEntityByVars = pfnFindEntityByVars;
        pengfuncsFromEngine->pfnGetModelPtr = pfnGetModelPtr;
        pengfuncsFromEngine->pfnRegUserMsg = pfnRegUserMsg;
        pengfuncsFromEngine->pfnAnimationAutomove = pfnAnimationAutomove;
        pengfuncsFromEngine->pfnGetBonePosition = pfnGetBonePosition;
        pengfuncsFromEngine->pfnFunctionFromName = pfnFunctionFromName;
        pengfuncsFromEngine->pfnNameForFunction = pfnNameForFunction;
        pengfuncsFromEngine->pfnClientPrintf = pfnClientPrintf;
        pengfuncsFromEngine->pfnServerPrint = pfnServerPrint;
        pengfuncsFromEngine->pfnGetAttachment = pfnGetAttachment;
        pengfuncsFromEngine->pfnCRC32_Init = pfnCRC32_Init;
        pengfuncsFromEngine->pfnCRC32_ProcessBuffer = pfnCRC32_ProcessBuffer;
        pengfuncsFromEngine->pfnCRC32_ProcessByte = pfnCRC32_ProcessByte;
        pengfuncsFromEngine->pfnCRC32_Final = pfnCRC32_Final;
        pengfuncsFromEngine->pfnRandomLong = pfnRandomLong;
        pengfuncsFromEngine->pfnRandomFloat = pfnRandomFloat;
        pengfuncsFromEngine->pfnSetView = pfnSetView;
        pengfuncsFromEngine->pfnTime = pfnTime;
        pengfuncsFromEngine->pfnCrosshairAngle = pfnCrosshairAngle;
        pengfuncsFromEngine->pfnLoadFileForMe = pfnLoadFileForMe;
        pengfuncsFromEngine->pfnFreeFile = pfnFreeFile;
        pengfuncsFromEngine->pfnEndSection = pfnEndSection;
        pengfuncsFromEngine->pfnCompareFileTime = pfnCompareFileTime;
        pengfuncsFromEngine->pfnGetGameDir = pfnGetGameDir;
        pengfuncsFromEngine->pfnCvar_RegisterVariable = pfnCvar_RegisterVariable;
        pengfuncsFromEngine->pfnFadeClientVolume = pfnFadeClientVolume;
        pengfuncsFromEngine->pfnSetClientMaxspeed = pfnSetClientMaxspeed;
        pengfuncsFromEngine->pfnCreateFakeClient = pfnCreateFakeClient;
        pengfuncsFromEngine->pfnRunPlayerMove = pfnRunPlayerMove;
        pengfuncsFromEngine->pfnNumberOfEntities = pfnNumberOfEntities;
        pengfuncsFromEngine->pfnGetInfoKeyBuffer = pfnGetInfoKeyBuffer;
        pengfuncsFromEngine->pfnInfoKeyValue = pfnInfoKeyValue;
        pengfuncsFromEngine->pfnSetKeyValue = pfnSetKeyValue;
        pengfuncsFromEngine->pfnSetClientKeyValue = pfnSetClientKeyValue;
        pengfuncsFromEngine->pfnIsMapValid = pfnIsMapValid;
        pengfuncsFromEngine->pfnStaticDecal = pfnStaticDecal;
        pengfuncsFromEngine->pfnPrecacheGeneric = pfnPrecacheGeneric;
        pengfuncsFromEngine->pfnGetPlayerUserId = pfnGetPlayerUserId;
        pengfuncsFromEngine->pfnBuildSoundMsg = pfnBuildSoundMsg;
        pengfuncsFromEngine->pfnIsDedicatedServer = pfnIsDedicatedServer;
        pengfuncsFromEngine->pfnCVarGetPointer = pfnCVarGetPointer;
        pengfuncsFromEngine->pfnGetPlayerWONId = pfnGetPlayerWONId;

        pengfuncsFromEngine->pfnPlaybackEvent = pfnPlaybackEvent;

        // pengfuncsFromEngine->pfnGetPlayerAuthID = pfnGetPlayerAuthID;
        // give the engine functions to the other DLL...
        if(!mr_meta)
            (*other_GiveFnptrsToDll)(pengfuncsFromEngine, pGlobals);

        //}
    } else {
        // meta mod can't intercept engine calls, so we must do it our selves

        pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;
        pengfuncsFromEngine->pfnClientPrintf = pfnClientPrintf;
        // Engine.pl_funcs->pfnClientCommand = pfnClientCommand;
        // Engine.pl_funcs->pfnClientPrintf = pfnClientPrintf;
        // other stuff we want to know :)
        pengfuncsFromEngine->pfnMessageBegin = pfnMessageBegin;
        pengfuncsFromEngine->pfnMessageEnd = pfnMessageEnd;
        pengfuncsFromEngine->pfnWriteByte = pfnWriteByte;
        pengfuncsFromEngine->pfnWriteChar = pfnWriteChar;
        pengfuncsFromEngine->pfnWriteShort = pfnWriteShort;
        pengfuncsFromEngine->pfnWriteLong = pfnWriteLong;
        pengfuncsFromEngine->pfnWriteAngle = pfnWriteAngle;
        pengfuncsFromEngine->pfnWriteCoord = pfnWriteCoord;
        pengfuncsFromEngine->pfnWriteString = pfnWriteString;
        pengfuncsFromEngine->pfnWriteEntity = pfnWriteEntity;
    }
}
