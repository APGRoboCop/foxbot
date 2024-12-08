// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
/*
 * File: version.cpp
 * Date: 21.09.2001
 * Project: Foxbot
 *
 * Description: This file provides static strings that hold up-to-date
 *              information about the build. It gets recompiled on every link.
 *
 * Copyright (c) 2001, Tom Simpson
 *
 *
 * $Id: version.cpp,v 1.1 2001/10/08 22:10:02 fb_cvs Exp $
 *
 */

 // Write Strings fixed - [APG]RoboCop[CL]
const char* COMPILE_DTTM = __DATE__ ", " __TIME__;

const char* COMPILE_DATE = __DATE__;