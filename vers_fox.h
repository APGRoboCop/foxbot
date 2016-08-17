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
# if defined(_MSC_VER) && defined(_DEBUG)
#  define OPT_TYPE		"msc debugging"
# elif defined(_MSC_VER) && defined(_NDEBUG)
#  define OPT_TYPE		"msc optimized"
# else
#  define OPT_TYPE		"default"
# endif /* _MSC_VER */
#endif /* not OPT_TYPE */

#define VDATE			"2016/08/01"
#define VVERSION		"0.78"
#define RC_VERS_DWORD	0.78"	// Version Windows DLL Resources in res_meta.rc

#define VNAME		"FoxBot"
#define VAUTHOR		"Tom Simpson <redfox@foxbot.net>"
#define VURL		"http://www.foxbot.net/"

// Various strings for the Windows DLL Resources in res_meta.rc
#define RC_COMMENTS		"AI opponent for halflife TFC"
#define RC_DESC			"FoxBot Half-Life MOD DLL"
#define RC_FILENAME		"foxbot_mm.dll"
#define RC_INTERNAL		"FoxBot"
#define RC_COPYRIGHT	"Copyright© 2002,Tom Simpson"

#endif /* VERS_META_H */
