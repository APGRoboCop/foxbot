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

#include "bot.h"

// meta mod stuff
#include <meta_api.h>

extern bool mr_meta;
// extern engine_t Engine;

#ifndef __linux__
HINSTANCE h_Library = nullptr;
#else
void* h_Library = nullptr;
#endif

enginefuncs_t g_engfuncs;
globalvars_t* gpGlobals;
char g_argv[256] = {
	0,
};

// static FILE *fp;

GETENTITYAPI other_GetEntityAPI = nullptr;
GETNEWDLLFUNCTIONS other_GetNewDLLFunctions = nullptr;
GIVEFNPTRSTODLL other_GiveFnptrsToDll = nullptr;

extern int mod_id;

#ifndef __linux__
// Required DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, const DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
	}
	else if (fdwReason == DLL_PROCESS_DETACH) {
		// Do not call FreeLibrary here
	}
	return true;
}

#endif

#if defined(_WIN32) && !defined(__GNUC__) && defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8,@1")
#pragma comment(linker, "/SECTION:.data,RW")
#endif

C_DLLEXPORT void WINAPI GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals) {
	std::memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;

	if (mr_meta) {
		return;
	}
	char game_dir[256];
	char mod_name[32];

	// find the directory name of the currently running MOD...
	(*g_engfuncs.pfnGetGameDir)(game_dir);

	unsigned int pos = 0;
	if (std::strchr(game_dir, '/') != nullptr) {
		pos = std::strlen(game_dir) - 1;
		// scan backwards till first directory separator...

		while (pos && game_dir[pos] != '/')
			pos--;
		if (pos == 0) {
			// Error getting directory name!
			ALERT(at_error, "FoxBot - Error determining MOD directory name!");
		}
		pos++;
	}
	std::strcpy(mod_name, &game_dir[pos]);

	if (strcasecmp(mod_name, "tfc") == 0) {
		mod_id = TFC_DLL;
#ifndef __linux__
		h_Library = LoadLibraryA("tfc\\dlls\\tfc.dll");
#else
		h_Library = dlopen("tfc/dlls/tfc.so", RTLD_NOW);
#endif
	}

	if (h_Library == nullptr) {
		// Directory error or Unsupported MOD!

		ALERT(at_error, "FoXBot - MOD dll not found (or unsupported MOD)!");
	}
	other_GetEntityAPI = reinterpret_cast<GETENTITYAPI>(GetProcAddress(h_Library, "GetEntityAPI"));

	if (other_GetEntityAPI == nullptr) {
		// Can't find GetEntityAPI!

		ALERT(at_error, "FoXBot - Can't get MOD's GetEntityAPI!");
	}

	other_GetNewDLLFunctions = reinterpret_cast<GETNEWDLLFUNCTIONS>(GetProcAddress(h_Library, "GetNewDLLFunctions"));

	other_GiveFnptrsToDll = reinterpret_cast<GIVEFNPTRSTODLL>(GetProcAddress(h_Library, "GiveFnptrsToDll"));

	if (other_GiveFnptrsToDll == nullptr) {
		// Can't find GiveFnptrsToDll!

		ALERT(at_error, "FoXBot - Can't get MOD's GiveFnptrsToDll!");
	}

	GetEngineFunctions(pengfuncsFromEngine, nullptr);

	// give the engine functions to the other DLL...
	other_GiveFnptrsToDll(pengfuncsFromEngine, pGlobals);
}

/*#if defined(__GNUC__)

void *operator new(size_t size) {
   if (size == 0)
	  return (calloc(1, 1));
   return (calloc(1, size));
}

void *operator new[](size_t size) {
   if (size == 0)
	  return (calloc(1, 1));
   return (calloc(1, size));
}

void operator delete(void *ptr) {
   if (ptr)
	  free(ptr);
}

void operator delete(void *ptr, size_t) {
   if (ptr)
	  free(ptr);
}

void operator delete[](void *ptr) {
   if (ptr)
	  free(ptr);
}

#endif*/