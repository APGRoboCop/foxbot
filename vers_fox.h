// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// vers_meta.h - version info, macros, etc

/*
 * Copyright (c) 2001, Will Day <willday@hpgx.net>
 * This file is covered by the GPL.
 */

#ifndef VERS_META_H
#define VERS_META_H

#ifndef OPT_TYPE
#if defined(_MSC_VER) && defined(_DEBUG)
#define OPT_TYPE "msc debugging"
#elif defined(_MSC_VER) && defined(_NDEBUG)
#define OPT_TYPE "msc optimized"
#else
#define OPT_TYPE "default"
#endif /* _MSC_VER */
#endif /* not OPT_TYPE */

extern const char *COMPILE_DATE;

#define VDATE COMPILE_DATE
constexpr const char* VVERSION = "1.0";
constexpr const char* RC_VERS_DWORD = "1.0"; // Version Windows DLL Resources in res_meta.rc

constexpr const char* VNAME = "FoxBot";
constexpr const char* VAUTHOR = "Tom Simpson <redfox@foxbot.net>";
constexpr const char* VURL = "http://www.foxbot.net/";

 // Various strings for the Windows DLL Resources in res_meta.rc
constexpr const char* RC_COMMENTS = "AI opponent for halflife TFC";
constexpr const char* RC_DESC = "FoxBot Half-Life MOD DLL";
constexpr const char* RC_FILENAME = "foxbot.dll";
constexpr const char* RC_INTERNAL = "FoxBot";
constexpr const char* RC_COPYRIGHT = "Copyright (c) 2002, Tom Simpson";

#endif /* VERS_META_H */
