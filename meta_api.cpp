// vi: set ts=4	sw=4 :
// vim:	set	tw=75 :

// meta_api.cpp	- minimal implementation of	metamod's plugin interface

// This	is intended	to illustrate the (more	or less) bare minimum code
// required	for	a valid	metamod	plugin,	and	is targeted	at those who want
// to port existing	HL/SDK DLL code	to run as a	metamod	plugin.

/*
* Copyright	(c)	2001, Will Day <willday@hpgx.net>
*/

#include <extdll.h> // always

#include <meta_api.h> // of course

//#include "bot.h"

extern bool mr_meta;

//#include "sdk_util.h"		// UTIL_LogPrintf, etc

// Must	provide	at least one of	these..
static META_FUNCTIONS gMetaFunctionTable = {
    GetEntityAPI,            // pfnGetEntityAPI				HL SDK;	called before game DLL
    GetEntityAPI_Post,       // pfnGetEntityAPI_Post			META; called after game	DLL
    NULL,                    // pfnGetEntityAPI2				HL SDK2; called	before game	DLL
    NULL,                    // pfnGetEntityAPI2_Post		META; called after game	DLL
    NULL,                    // pfnGetNewDLLFunctions		HL SDK2; called	before game	DLL
    NULL,                    // pfnGetNewDLLFunctions_Post	META; called after game	DLL
    GetEngineFunctions,      // pfnGetEngineFunctions		META; called before	HL engine
    GetEngineFunctions_Post, // pfnGetEngineFunctions_Post	META; called after HL engine
};

// Description of plugin
plugin_info_t Plugin_info = {
    META_INTERFACE_VERSION,                       // ifvers
    "FoxBot",                                     // name
    "0.78-b3",                                    // version
    "14/8/2016",                                  // date
    "Tom Simpson, RoboCop <robocop@lycos.co.uk>", // author
    "http://www.apg-clan.org/",                   // url
    //"http://www.omni-bot.com/",	// url
    //"http://www.foxbot.net/",	// url
    "FOXBOT",   // logtag
    PT_STARTUP, // (when) loadable
    PT_STARTUP, // (when) unloadable
};

// Global vars from	metamod:
meta_globals_t* gpMetaGlobals;   // metamod globals
gamedll_funcs_t* gpGamedllFuncs; // gameDLL function	tables
mutil_funcs_t* gpMetaUtilFuncs;  // metamod utility functions

void Meta_Init(void)
{
    mr_meta = TRUE;
}

// Metamod requesting info about this plugin:
//	ifvers			(given)	interface_version metamod is using
//	pPlugInfo		(requested)	struct with	info about plugin
//	pMetaUtilFuncs	(given)	table of utility functions provided	by metamod
C_DLLEXPORT int Meta_Query(char* ifvers, plugin_info_t** pPlugInfo, mutil_funcs_t* pMetaUtilFuncs)
{
    // Give metamod our plugin_info struct
    *pPlugInfo = &Plugin_info;
    // Get metamod utility function table.
    gpMetaUtilFuncs = pMetaUtilFuncs;

    // check for interface version compatibility
    // this bit of code was adapted from code by Pierre-Marie Baty
    if(strcmp(ifvers, Plugin_info.ifvers) != 0) {
        LOG_CONSOLE(PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers,
            Plugin_info.name, Plugin_info.ifvers);

        LOG_MESSAGE(PLID, "%s: meta-interface version mismatch (metamod: %s, %s: %s)", Plugin_info.name, ifvers,
            Plugin_info.name, Plugin_info.ifvers);
    }

    return (TRUE);
}

// Metamod attaching plugin	to the server.
//	now				(given)	current	phase, ie during map, during changelevel, or at	startup
//	pFunctionTable	(requested)	table of function tables this plugin catches
//	pMGlobals		(given)	global vars	from metamod
//	pGamedllFuncs	(given)	copy of	function tables	from game dll
C_DLLEXPORT int Meta_Attach(PLUG_LOADTIME now,
    META_FUNCTIONS* pFunctionTable,
    meta_globals_t* pMGlobals,
    gamedll_funcs_t* pGamedllFuncs)
{
    if(now)
        ; // to satisfy gcc -Wunused
    if(!pMGlobals) {
        LOG_ERROR(PLID, "Meta_Attach called	with null pMGlobals");
        return (FALSE);
    }
    gpMetaGlobals = pMGlobals;
    if(!pFunctionTable) {
        LOG_ERROR(PLID, "Meta_Attach called	with null pFunctionTable");
        return (FALSE);
    }
    memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
    gpGamedllFuncs = pGamedllFuncs;
    return (TRUE);
}

// Metamod detaching plugin	from the server.
// now		(given)	current	phase, ie during map, etc
// reason	(given)	why	detaching (refresh,	console	unload,	forced unload, etc)
C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason)
{
    if(now && reason)
        ; // to satisfy gcc -Wunused
    return (TRUE);
}
