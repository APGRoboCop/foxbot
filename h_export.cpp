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

#include "enginecallback.h"
#include "extdll.h"

#include "bot.h"
#include "engine.h"

#ifdef WIN32
#define strcmpi _strcmpi
#endif

// meta mod stuff
#include <h_export.h> // me
#include <meta_api.h>

extern bool mr_meta;
// extern engine_t Engine;

#ifndef __linux__
HINSTANCE h_Library = NULL;
HGLOBAL h_global_argv = NULL;
#else
void *h_Library = NULL;
char h_global_argv[1024];
#endif

enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;
char *g_argv;

// static FILE *fp;

GETENTITYAPI other_GetEntityAPI = NULL;
GETNEWDLLFUNCTIONS other_GetNewDLLFunctions = NULL;
GIVEFNPTRSTODLL other_GiveFnptrsToDll = NULL;

extern int mod_id;

#ifndef __linux__
// Required DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, const DWORD fdwReason, LPVOID lpvReserved) {
   if (fdwReason == DLL_PROCESS_ATTACH) {
   } else if (fdwReason == DLL_PROCESS_DETACH) {
      // if (h_Library)
      //	FreeLibrary(h_Library);

      if (h_global_argv) {
         GlobalUnlock(h_global_argv);
         GlobalFree(h_global_argv);
      }
   }

   return TRUE;
}
#endif

#if defined(_WIN32) && !defined(__GNUC__) && defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8,@1")
#pragma comment(linker, "/SECTION:.data,RW")
#endif

void WINAPI GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals) {
   memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
   gpGlobals = pGlobals;

   if (mr_meta) {
      return;
   }
   char game_dir[256];
   char mod_name[32];

   // get the engine functions from the engine...

   memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
   gpGlobals = pGlobals;

   // find the directory name of the currently running MOD...
   (*g_engfuncs.pfnGetGameDir)(game_dir);

   int pos = 0;
   if (strchr(game_dir, '/') != NULL) {
      pos = strlen(game_dir) - 1;
      // scan backwards till first directory separator...

      while (pos && game_dir[pos] != '/')
         pos--;
      if (pos == 0) {
         // Error getting directory name!
         ALERT(at_error, "FoxBot - Error determining MOD directory name!");
      }
      pos++;
   }
   strcpy(mod_name, &game_dir[pos]);

   if (strcmpi(mod_name, "tfc") == 0) {
      mod_id = TFC_DLL;
#ifndef __linux__
      h_Library = LoadLibrary("tfc\\dlls\\tfc.dll");
#else
      h_Library = dlopen("tfc/dlls/tfc.so", RTLD_NOW);
#endif
   }

   if (h_Library == NULL) {
      // Directory error or Unsupported MOD!

      ALERT(at_error, "FoXBot - MOD dll not found (or unsupported MOD)!");
   }
#ifndef __linux__
   h_global_argv = GlobalAlloc(GMEM_SHARE, 1024);
   g_argv = static_cast<char *>(GlobalLock(h_global_argv));
#else
   g_argv = (char *)h_global_argv;
#endif

   other_GetEntityAPI = (GETENTITYAPI)GetProcAddress(h_Library, "GetEntityAPI");

   if (other_GetEntityAPI == NULL) {
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

   if (other_GiveFnptrsToDll == NULL) {
      // Can't find GiveFnptrsToDll!

      ALERT(at_error, "FoXBot - Can't get MOD's GiveFnptrsToDll!");
   }

   GetEngineFunctions(pengfuncsFromEngine, NULL);

   // give the engine functions to the other DLL...
   (*other_GiveFnptrsToDll)(pengfuncsFromEngine, pGlobals);
}