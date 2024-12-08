// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// vi: set ts=4 sw=4 :
// vim: set tw=75 :
//
// sdk_util.cpp - utility routines from HL SDK util.cpp
//
// Selected portions of dlls/util.cpp from SDK 2.1.
// Functions copied from there as needed...
/***
 *
 *	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/
 /*

 ===== util.cpp ========================================================

	Utility code.  Really not optional after all.

 */

#include <extdll.h>
#include <algorithm>
#include "sdk_util.h"

char* UTIL_VarArgs(const char* format, ...) {
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
#ifdef WIN32
	_vsnprintf(string, 1024, format, argptr);
#else
	_vsnprintf(string, 1024, format, argptr);
#endif
	va_end(argptr);

	return string;
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf(char* format, ...) {
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
#ifdef WIN32
	_vsnprintf(string, 1024, format, argptr);
#else
	_vsnprintf(string, 1024, format, argptr);
#endif
	va_end(argptr);

	// Print to server console
	ALERT(at_logged, "%s", string);
}

short FixedSigned16(const float value, const float scale) {
	int output = static_cast<int>(value * scale);

   output = std::min(output, 32767);
   output = std::max(output, -32768);

   return static_cast<short>(output);
}

unsigned short FixedUnsigned16(const float value, const float scale) {
	int output = static_cast<int>(value * scale);

   output = std::max(output, 0);
   output = std::min(output, 0xFFFF);

   return static_cast<unsigned short>(output);
}