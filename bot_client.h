//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_client.h
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

void BotClient_TFC_VGUI(void* p, int bot_index);
// void BotClient_CS_VGUI(void* p, int bot_index); // Not required for TFC - [APG]RoboCop[CL]
// void BotClient_CS_ShowMenu(void* p, int bot_index);
// void BotClient_Gearbox_VGUI(void* p, int bot_index);
// void BotClient_FLF_VGUI(void* p, int bot_index);

void BotClient_Valve_WeaponList(void* p, int bot_index);
void BotClient_TFC_WeaponList(void* p, int bot_index);
// void BotClient_CS_WeaponList(void* p, int bot_index);
// void BotClient_Gearbox_WeaponList(void* p, int bot_index);
// void BotClient_FLF_WeaponList(void* p, int bot_index);

void BotClient_Valve_CurrentWeapon(void* p, int bot_index);
void BotClient_TFC_CurrentWeapon(void* p, int bot_index);
// void BotClient_CS_CurrentWeapon(void* p, int bot_index);
// void BotClient_Gearbox_CurrentWeapon(void* p, int bot_index);
// void BotClient_FLF_CurrentWeapon(void* p, int bot_index);

void BotClient_Valve_AmmoX(void* p, int bot_index);
void BotClient_TFC_AmmoX(void* p, int bot_index);
// void BotClient_CS_AmmoX(void* p, int bot_index);
// void BotClient_Gearbox_AmmoX(void* p, int bot_index);
// void BotClient_FLF_AmmoX(void* p, int bot_index);

void BotClient_Valve_AmmoPickup(void* p, int bot_index);
void BotClient_TFC_AmmoPickup(void* p, int bot_index);
// void BotClient_CS_AmmoPickup(void* p, int bot_index);
// void BotClient_Gearbox_AmmoPickup(void* p, int bot_index);
// void BotClient_FLF_AmmoPickup(void* p, int bot_index);

void BotClient_Valve_WeaponPickup(void* p, int bot_index);
void BotClient_TFC_WeaponPickup(void* p, int bot_index);
// void BotClient_CS_WeaponPickup(void* p, int bot_index);
// void BotClient_Gearbox_WeaponPickup(void* p, int bot_index);
// void BotClient_FLF_WeaponPickup(void* p, int bot_index);

void BotClient_Valve_ItemPickup(void* p, int bot_index);
void BotClient_TFC_ItemPickup(void* p, int bot_index);
// void BotClient_CS_ItemPickup(void* p, int bot_index);
// void BotClient_Gearbox_ItemPickup(void* p, int bot_index);
// void BotClient_FLF_ItemPickup(void* p, int bot_index);

void BotClient_Valve_Health(void* p, int bot_index);
void BotClient_TFC_Health(void* p, int bot_index);
// void BotClient_CS_Health(void* p, int bot_index);
// void BotClient_Gearbox_Health(void* p, int bot_index);
// void BotClient_FLF_Health(void* p, int bot_index);

void BotClient_Valve_Battery(void* p, int bot_index);
void BotClient_TFC_Battery(void* p, int bot_index);
// void BotClient_CS_Battery(void* p, int bot_index);
// void BotClient_Gearbox_Battery(void* p, int bot_index);
// void BotClient_FLF_Battery(void* p, int bot_index);

void BotClient_Valve_Damage(void* p, int bot_index);
void BotClient_TFC_Damage(void* p, int bot_index);
// void BotClient_CS_Damage(void* p, int bot_index);
// void BotClient_Gearbox_Damage(void* p, int bot_index);
// void BotClient_FLF_Damage(void* p, int bot_index);

// void BotClient_CS_Money(void* p, int bot_index);

void BotClient_Valve_DeathMsg(void* p, int bot_index);
void BotClient_TFC_DeathMsg(void* p, int bot_index);
// void BotClient_CS_DeathMsg(void* p, int bot_index);
// void BotClient_Gearbox_DeathMsg(void* p, int bot_index);
// void BotClient_FLF_DeathMsg(void* p, int bot_index);

void BotClient_Valve_ScreenFade(void* p, int bot_index);
void BotClient_TFC_ScreenFade(void* p, int bot_index);
// void BotClient_CS_ScreenFade(void* p, int bot_index);
// void BotClient_Gearbox_ScreenFade(void* p, int bot_index);
// void BotClient_FLF_ScreenFade(void* p, int bot_index);

// tfc specific
void BotClient_TFC_StatusIcon(void* p, int bot_index);
void BotClient_Engineer_BuildStatus(void* p, int bot_index);
void BotClient_TFC_SentryAmmo(void* p, int bot_index);
void BotClient_TFC_DetPack(void* p, int bot_index);
void BotClient_Menu(void* p, int bot_index);
void BotClient_TFC_Grens(void* p, int bot_index);
