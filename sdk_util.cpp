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
#include "sdk_util.h"

#include <cstdarg>
#include <cstdio>
#include <vector>
#include <thread>
#include <mutex>

#ifdef _WIN32
#define vsnprintf _vsnprintf
#endif

// Thread-local storage for the buffer
thread_local std::vector<char> thread_local_buffer(1024);
static std::mutex buffer_mutex;

// Utility function to format a string with variable arguments
char *UTIL_VarArgs(const char *format, ...) {
   std::scoped_lock lock(buffer_mutex);

   va_list argptr;
   va_start(argptr, format);
   int ret = vsnprintf(thread_local_buffer.data(), thread_local_buffer.size(), format, argptr);
   va_end(argptr);

   // Handle buffer overflow
   while (ret < 0 || static_cast<size_t>(ret) >= thread_local_buffer.size()) {
      // Resize buffer to the required size plus one for the null terminator
      thread_local_buffer.resize(thread_local_buffer.size() * 2);
      va_start(argptr, format);
      ret = vsnprintf(thread_local_buffer.data(), thread_local_buffer.size(), format, argptr);
      va_end(argptr);
   }

   // Check if vsnprintf failed again
   if (ret < 0) {
      // Log error and return an empty string
      fprintf(stderr, "UTIL_VarArgs: vsnprintf failed\n");
      return nullptr;
   }

   return thread_local_buffer.data();
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