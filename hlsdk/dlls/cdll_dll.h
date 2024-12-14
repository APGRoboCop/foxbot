/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
//  cdll_dll.h

// this file is included by both the game-dll and the client-dll,

#ifndef CDLL_DLL_H
#define CDLL_DLL_H

constexpr int MAX_WEAPONS = 32; // ???

constexpr int MAX_WEAPON_SLOTS = 5; // hud item selection slots
constexpr int MAX_ITEM_TYPES = 6;   // hud item selection slots

constexpr int MAX_ITEMS = 5; // hard coded item types

constexpr int HIDEHUD_WEAPONS = (1 << 0);
constexpr int HIDEHUD_FLASHLIGHT = (1 << 1);
constexpr int HIDEHUD_ALL = (1 << 2);
constexpr int HIDEHUD_HEALTH = (1 << 3);

constexpr int MAX_AMMO_TYPES = 32; // ???
constexpr int MAX_AMMO_SLOTS = 32; // not really slots

constexpr int HUD_PRINTNOTIFY = 1;
constexpr int HUD_PRINTCONSOLE = 2;
constexpr int HUD_PRINTTALK = 3;
constexpr int HUD_PRINTCENTER = 4;

constexpr int WEAPON_SUIT = 31;

#endif
