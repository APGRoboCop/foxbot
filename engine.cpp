//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// engine.cpp
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
//#include "util.h"

#include "bot.h"
#include "bot_client.h"
#include "bot_func.h"
#include "engine.h"

#include "meta_api.h" //meta mod"

extern bool mr_meta;

// extern enginefuncs_t g_engfuncs; //No longer required? [APG]RoboCop[CL]
extern bot_t bots[32];
extern int mod_id;

extern int a;

extern char prevmapname[32];

extern edict_t* clients[32];

static bool name_message_check(const char* msg_string, const char* name_string);

// my external stuff for scripted message intercept
/*extern bool blue_av[8];
   extern bool red_av[8];
   extern bool green_av[8];
   extern bool yellow_av[8];

   extern struct msg_com_struct
   {
   char ifs[32];
   int blue_av[8];
   int red_av[8];
   int green_av[8];
   int yellow_av[8];
   struct msg_com_struct *next;
   } msg_com[MSG_MAX];
   extern char msg_msg[64][MSG_MAX]; */

int debug_engine = 0;

bool spawn_check_crash = false;
int spawn_check_crash_count = 0;
edict_t* spawn_check_crash_edict = nullptr;

void (*botMsgFunction)(void*, int) = nullptr;
void (*botMsgEndFunction)(void*, int) = nullptr;
int botMsgIndex;

// g_state from bot_clients
extern int g_state;

// messages created in RegUserMsg which will be "caught"
int message_VGUI = 0;
int message_ShowMenu = 0;
int message_WeaponList = 0;
int message_CurWeapon = 0;
int message_AmmoX = 0;
int message_WeapPickup = 0;
int message_AmmoPickup = 0;
int message_ItemPickup = 0;
int message_Health = 0;
int message_Battery = 0; // Armor
int message_Damage = 0;
// int message_Money = 0;  // for Counter-Strike
int message_DeathMsg = 0;
int message_TextMsg = 0;
// int message_WarmUp = 0;     // for Front Line Force
// int message_WinMessage = 0; // for Front Line Force
int message_ScreenFade = 0;
int message_StatusIcon = 0; // flags in tfc

// mine
int message_TeamScores = 0;
int message_StatusText = 0;
int message_StatusValue = 0;
int message_Detpack = 0;
int message_SecAmmoVal = 0;

bool MM_func = false;
static std::FILE* fp;

bool dont_send_packet = false;

char sz_error_check[255];

edict_t* pfnFindEntityInSphere(edict_t* pEdictStartSearchAfter, const float* org, const float rad) {
	if (debug_engine) {
		const Vector orgVector(org[0], org[1], org[2]); // Create a new Vector from the float array
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "pfnFindEntityInSphere:%p (%f %f %f) %f %d\n", static_cast<void*>(pEdictStartSearchAfter), orgVector.x, orgVector.y, orgVector.z,
			static_cast<double>(rad), spawn_check_crash_count);
	   
		if (pEdictStartSearchAfter != nullptr)
			if (pEdictStartSearchAfter->v.classname != 0)
				std::fprintf(fp, "classname %s\n", STRING(pEdictStartSearchAfter->v.classname));
		std::fclose(fp);
	}
	if (spawn_check_crash && static_cast<int>(rad) == 96) {
		spawn_check_crash_count++;
		if (spawn_check_crash_count > 512) {
			// pfnSetSize: 958fd0 (-16.000000 -16.000000 -36.000000) (16.000000 16.000000 36.000000)
			SET_ORIGIN(spawn_check_crash_edict, org);
			{
				fp = UTIL_OpenFoxbotLog();
				std::fprintf(fp, "spawn crash fix!: \n");
				std::fclose(fp);
			}
		}
	}
	if (mr_meta)
		RETURN_META_VALUE(MRES_HANDLED, NULL);
	return (*g_engfuncs.pfnFindEntityInSphere)(pEdictStartSearchAfter, org, rad);
}

void pfnRemoveEntity(edict_t* e) {
	// tell each bot to forget about the removed entity
	for (auto &bot : bots) {
		if (bot.is_used) {
			if (bot.lastEnemySentryGun == e)
            bot.lastEnemySentryGun = nullptr;
			if (bot.enemy.ptr == e)
            bot.enemy.ptr = nullptr;

			if (bot.pEdict->v.playerclass == TFC_CLASS_ENGINEER) {
				if (bot.sentry_edict == e) {
               bot.has_sentry = false;
               bot.sentry_edict = nullptr;
               bot.SGRotated = false;
				}

				if (bot.dispenser_edict == e) {
               bot.has_dispenser = false;
               bot.dispenser_edict = nullptr;
				}

				if (bot.tpEntrance == e) {
               bot.tpEntrance = nullptr;
               bot.tpEntranceWP = -1;
				}

				if (bot.tpExit == e) {
               bot.tpExit = nullptr;
               bot.tpExitWP = -1;
				}
			}
		}
	}
	// snprintf(sz_error_check,250,"pfnRemoveEntity: %x %d\n",e,e->v.spawnflags);

	if (mr_meta)
		RETURN_META(MRES_HANDLED);
	(*g_engfuncs.pfnRemoveEntity)(e);
}

void pfnSetOrigin(edict_t* e, const float* rgflOrigin) {
	if (std::strcmp(STRING(e->v.classname), "player") == 0) {
		// teleport at new round start
		// clear up current wpt
		for (auto &bot : bots) {
			// only consider existing bots that haven't died very recently
			if (bot.pEdict == e && bot.is_used && bot.f_killed_time + 3.0f < gpGlobals->time) {
				// see if a teleporter pad moved the bot
				const edict_t* teleExit = BotEntityAtPoint("building_teleporter", bot.pEdict->v.origin, 90.0f);

				if (teleExit == nullptr) {
					//	UTIL_BotLogPrintf("%s Non-teleport translocation, time %f\n",
					//		bots[bot_index].name, gpGlobals->time);

               bot.current_wp = -1;
               bot.f_snipe_time = 0.0f;
               bot.f_primary_charging = 0.0f;
				}
				/*	else
								{
												UTIL_BotLogPrintf("%s Teleported somewhere, time %f\n",
																				bots[bot_index].name, gpGlobals->time);
								}*/

				break; // must have found the right bot
			}
		}
	}
	else if (std::strcmp(STRING(e->v.classname), "building_sentrygun") == 0) {
		// ok, we have the 'base' entity pointer
		// we want the pointer to the sentry itself

		for (auto &bot : bots) {
			if (bot.sentry_edict != nullptr && bot.has_sentry) {
				edict_t* pent = e;
				const Vector rgflOriginVector(rgflOrigin[0], rgflOrigin[1], rgflOrigin[2]); // Create a new Vector from the float array
				int l = static_cast<int>(bot.sentry_edict->v.origin.z - rgflOriginVector.z);
				if (l < 0)
					l = -l;
				const int xa = static_cast<int>(rgflOriginVector.x);
				const int ya = static_cast<int>(rgflOriginVector.y);
				const int xb = static_cast<int>(bot.sentry_edict->v.origin.x);
				const int yb = static_cast<int>(bot.sentry_edict->v.origin.y);
				// FILE *fp;
				//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"l %d xa %d xb %d ya %d yb %d\n",l,xa,xb,ya,yb); std::fclose(fp); }
				if (l >= 8 &&
					l <= 60
					//&& (xa<xb+2 && xa+2>xb)
					//&& (ya<yb+2 && ya+2>yb))
					&& xa == xb && ya == yb) {
               bot.sentry_edict = pent;
				}
			}
		}
	}
	else if (std::strncmp(STRING(e->v.classname), "func_button", 11) == 0 || std::strncmp(STRING(e->v.classname), "func_rot_button", 15) == 0) {
		if (e->v.target != 0) {
			char msg[255];
			// TYPEDESCRIPTION		*pField;
			// pField = &gEntvarsDescription[36];
			//(*(float *)((char *)pev + pField->fieldOffset))
			std::sprintf(msg, "target %s, toggle %.0f", STRING(e->v.target), static_cast<double>(e->v.frame));
			script(msg);
		}
	}
	/*else if(std::strncmp(STRING(e->v.classname),"func_",5)==0)
	   {
	   if(e->v.globalname!=NULL)
	   {
	   char msg[255];
	   //TYPEDESCRIPTION		*pField;
	   //pField = &gEntvarsDescription[36];
	   //(*(float *)((char *)pev + pField->fieldOffset))
	   std::sprintf(msg,"name %s, toggle %.0f",STRING(e->v.globalname),e->v.frame);
	   script(msg);
	   }
	   }*/

	if (debug_engine) {
		const Vector rgflOriginVector(rgflOrigin[0], rgflOrigin[1], rgflOrigin[2]); // Create a new Vector from the float array
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "pfnSetOrigin: %p (%f %f %f)\n", static_cast<void*>(e), static_cast<double>(rgflOriginVector.x), static_cast<double>(rgflOriginVector.y), static_cast<double>(rgflOriginVector.z));

		if (e->v.classname != 0)
			std::fprintf(fp, " name=%s\n", STRING(e->v.classname));
		if (e->v.target != 0)
			std::fprintf(fp, " target=%s\n", STRING(e->v.target));
		if (e->v.ltime < e->v.nextthink)
			std::fprintf(fp, " 1\n");
		else
			std::fprintf(fp, " 0\n");

		std::fprintf(fp, " t %f %f\n", static_cast<double>(e->v.ltime), static_cast<double>(e->v.nextthink));
		std::fprintf(fp, " button=%d\n", e->v.button);
		std::fclose(fp);
	}
	if (mr_meta)
		RETURN_META(MRES_HANDLED);
	(*g_engfuncs.pfnSetOrigin)(e, rgflOrigin);
}

void pfnEmitSound(edict_t* entity, const int channel, const char* sample, const float volume, const float attenuation, const int fFlags, const int pitch) {
	if (debug_engine) {
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "pfnEmitSound: %s\n", sample);
		std::fclose(fp);
	}

	BotSoundSense(entity, sample, volume);

	if (mr_meta)
		RETURN_META(MRES_HANDLED);
	(*g_engfuncs.pfnEmitSound)(entity, channel, sample, volume, attenuation, fFlags, pitch);
}

void pfnEmitAmbientSound(edict_t* entity, float* pos, const char* samp, const float vol, const float attenuation, const int fFlags, const int pitch) {
	if (debug_engine) {
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "pfnEmitAmbientSound: %s\n", samp);
		std::fclose(fp);
	}

	script(samp);
	if (mr_meta)
		RETURN_META(MRES_HANDLED);
	(*g_engfuncs.pfnEmitAmbientSound)(entity, pos, samp, vol, attenuation, fFlags, pitch);
}

void pfnClientCommand(edict_t* pEdict, char* szFmt, ...) {
	if (debug_engine) {
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "-pfnClientCommand=%s %p\n", szFmt, static_cast<void*>(pEdict));
		std::fclose(fp);
	}
	snprintf(sz_error_check, 250, "-pfnClientCommand=%s %p\n", szFmt, static_cast<void*>(pEdict));

   /*if(pEdict!=NULL)
	{
	   if((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT)
	   {
	   //admin mod fix here! ...maybee clientprintf aswell..dunno
	   FakeClientCommand(pEdict,szFmt,NULL,NULL);
	   //if(mr_meta) RETURN_META(MRES_SUPERCEDE);
	   }
	}*/

   char tempFmt[1024];
	va_list argp;

	va_start(argp, szFmt);
	std::vsnprintf(tempFmt, sizeof(tempFmt), szFmt, argp);
	va_end(argp);

	if (pEdict != nullptr) {
		// if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
		bool b = false;
		if ((pEdict->v.flags & FL_FAKECLIENT) != FL_FAKECLIENT) {
			for (auto &client : clients) {
				// if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
				// bots[i].is_used &&
				if (client == pEdict)
					b = true;
				/*if(bots[i].pEdict==pEdict && (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1 ||
				   (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
				   {
				   b=false;
				   //snprintf(sz_error_check,250,"%s %d",sz_error_check,i);
				   }*/
			}
		}
		if (b) {
			char cl_name[128];
			cl_name[0] = '\0';

         const char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);
			std::strncpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120);

			//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"cl %d name %s\n",i,cl_name); std::fclose(fp); }
			if (cl_name[0] == '\0' || infobuffer == nullptr)
				b = false;
			//	unsigned int u=GETPLAYERWONID(pEdict);
			//	if((u==0 || ENTINDEX(pEdict)==-1))
			//		b=false;
			// snprintf(sz_error_check,250,"%s %d",sz_error_check,GETPLAYERWONID(pEdict));
		}
		if (b) {
			//	snprintf(sz_error_check,250,"%s b = %d %d\n",sz_error_check,GETPLAYERWONID(pEdict),ENTINDEX(pEdict));
			//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"b\n"); std::fclose(fp); }
			// snprintf(sz_error_check,250,"%s -executing",sz_error_check);
			(*g_engfuncs.pfnClientCommand)(pEdict, tempFmt);
			va_end(argp);
			return;
		}
		std::strncat(sz_error_check, " !b\n", 250 - std::strlen(sz_error_check) - 1);
		return;
		//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"!b\n"); std::fclose(fp); }
	}
	(*g_engfuncs.pfnClientCommand)(pEdict, tempFmt);
	va_end(argp);
	// if(mr_meta) RETURN_META(MRES_HANDLED);
	//return;
}

void pfnClCom(edict_t* pEdict, char* szFmt, ...) {
	if (debug_engine) {
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "-pfnClientCom=%s %p\n", szFmt, static_cast<void*>(pEdict));
		std::fclose(fp);
	}
	snprintf(sz_error_check, 250, "-pfnClientCom=%s %p\n", szFmt, static_cast<void*>(pEdict));
	if (pEdict != nullptr) {
		bool b = false;

		if ((pEdict->v.flags & FL_FAKECLIENT) != FL_FAKECLIENT) {
			for (auto &client : clients) {
				// if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
				// bots[i].is_used &&
				if (client == pEdict)
					b = true;
				/*if(bots[i].pEdict==pEdict && (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1 ||
				   (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
				   b=false;*/
			}
		}
		if (b) {
			char cl_name[128];
			cl_name[0] = '\0';

         const char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);
			std::strncpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120);
			//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"cl %d name %s\n",i,cl_name); std::fclose(fp); }
			if (cl_name[0] == '\0' || infobuffer == nullptr)
				b = false;
			// unsigned int u=GETPLAYERWONID(pEdict);
			// if((u==0 || ENTINDEX(pEdict)==-1))
			//	b=false;
		}
		// if its a bot (b=false) we need to override
		if (!b) {
			std::strncat(sz_error_check, " !b\n", 250 - std::strlen(sz_error_check));
			// admin mod fix here! ...maybee clientprintf aswell..dunno
			// FakeClientCommand(pEdict,szFmt,NULL,NULL);
			//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"!b\n"); std::fclose(fp); }
			if (mr_meta)
				RETURN_META(MRES_SUPERCEDE);
			return;
		}
		//	snprintf(sz_error_check,250,"%s b = %d %d\n",sz_error_check,GETPLAYERWONID(pEdict),ENTINDEX(pEdict));
		return;
	}
	if (mr_meta)
		RETURN_META(MRES_SUPERCEDE);
	return;
	//	if(mr_meta) RETURN_META(MRES_HANDLED);  // unreachable code
	 //	return;
}

void MessageBegin(const int msg_dest, const int msg_type, const float* pOrigin, edict_t* ed) {
	MM_func = true;
	pfnMessageBegin(msg_dest, msg_type, pOrigin, ed);
	MM_func = false;
}

void pfnMessageBegin(const int msg_dest, const int msg_type, const float* pOrigin, edict_t* ed) {
	/*if(ed!=NULL)
	   if(ed->v.classname==NULL || ed->v.netname==NULL)
	   dont_send_packet=true;*/
	if (gpGlobals->deathmatch) {
		if (debug_engine /* || dont_send_packet*/) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnMessageBegin: edict=%p dest=%d type=%d\n", static_cast<void*>(ed), msg_dest, msg_type);
			std::fclose(fp);
		}

		/*snprintf(sz_error_check,250,
						"pfnMessageBegin: edict=%x dest=%d type=%d id=%d %d\n",
						ed,msg_dest,msg_type,GETPLAYERWONID(ed),ENTINDEX(ed));*/

		if (ed) {
			const int index = UTIL_GetBotIndex(ed);

			// is this message for a bot?
			if (index != -1) {
				g_state = 0;                 // reset global message state..where we at!
				botMsgFunction = nullptr;    // no msg function until known otherwise
				botMsgEndFunction = nullptr; // no msg end function until known otherwise
				botMsgIndex = index;         // index of bot receiving message

				if (mod_id == TFC_DLL) {
					if (msg_type == message_VGUI)
						botMsgFunction = BotClient_TFC_VGUI;
					else if (msg_type == message_WeaponList)
						botMsgFunction = BotClient_TFC_WeaponList;
					else if (msg_type == message_CurWeapon)
						botMsgFunction = BotClient_TFC_CurrentWeapon;
					else if (msg_type == message_AmmoX)
						botMsgFunction = BotClient_TFC_AmmoX;
					else if (msg_type == message_AmmoPickup)
						botMsgFunction = BotClient_TFC_AmmoPickup;
					else if (msg_type == message_WeapPickup)
						botMsgFunction = BotClient_TFC_WeaponPickup;
					else if (msg_type == message_ItemPickup)
						botMsgFunction = BotClient_TFC_ItemPickup;
					else if (msg_type == message_Health)
						botMsgFunction = BotClient_TFC_Health;
					else if (msg_type == message_Battery)
						botMsgFunction = BotClient_TFC_Battery;
					else if (msg_type == message_Damage)
						botMsgFunction = BotClient_TFC_Damage;
					else if (msg_type == message_ScreenFade)
						botMsgFunction = BotClient_TFC_ScreenFade;
					else if (msg_type == message_StatusIcon)
						botMsgFunction = BotClient_TFC_StatusIcon;
					// not all these messages are used for..but all I am checking for
					else if (msg_type == message_TextMsg || msg_type == message_StatusText)
						botMsgFunction = BotClient_Engineer_BuildStatus;
					else if (msg_type == message_StatusValue)
						botMsgFunction = BotClient_TFC_SentryAmmo;
					else if (msg_type == message_Detpack)
						botMsgFunction = BotClient_TFC_DetPack;
					// menu! prolly from admin mod
					else if (msg_type == message_ShowMenu)
						botMsgFunction = BotClient_Menu;
					else if (msg_type == message_SecAmmoVal)
						botMsgFunction = BotClient_TFC_Grens;
				}
			}
			else {
				// (index == -1)
				g_state = 0;                 // reset global message state..where we at!
				botMsgFunction = nullptr;    // no msg function until known otherwise
				botMsgEndFunction = nullptr; // no msg end function until known otherwise
				botMsgIndex = index;         // index of bot receiving message
				if (mod_id == TFC_DLL) {
					if (msg_type == message_TextMsg || msg_type == message_StatusText)
						botMsgFunction = BotClient_Engineer_BuildStatus;
				}
			}
		}
		else if (msg_dest == MSG_ALL) {
			botMsgFunction = nullptr; // no msg function until known otherwise
			botMsgIndex = -1;         // index of bot receiving message (none)

			if (mod_id == TFC_DLL) {
				if (msg_type == message_DeathMsg)
					botMsgFunction = BotClient_TFC_DeathMsg;

				//	else if(msg_type == message_TeamScores)
				//		botMsgFunction = BotClient_TFC_Scores;

				// put the new message here
			}
		}
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}

void MessageEnd() {
	MM_func = true;
	pfnMessageEnd();
	MM_func = false;
}

void pfnMessageEnd() {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnMessageEnd:\n"); std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnMessageEnd:\n");
			std::fclose(fp);
		}

		if (botMsgEndFunction)
			(*botMsgEndFunction)(nullptr, botMsgIndex); // NULL indicated msg end

		// clear out the bot message function pointers...
		botMsgFunction = nullptr;
		botMsgEndFunction = nullptr;
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet) {
			dont_send_packet = false;
			RETURN_META(MRES_SUPERCEDE);
		}
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet) {
		dont_send_packet = false;
		return;
	}
	(*g_engfuncs.pfnMessageEnd)();
}

void WriteByte(const int iValue) {
	MM_func = true;
	pfnWriteByte(iValue);
	MM_func = false;
}

void pfnWriteByte(int iValue) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteByte: %d\n",iValue);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteByte: %d\n", iValue);
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteByte)(iValue);
}

void WriteChar(const int iValue) {
	MM_func = true;
	pfnWriteChar(iValue);
	MM_func = false;
}

void pfnWriteChar(int iValue) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteChar: %d\n",iValue);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteChar: %d\n", iValue);
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteChar)(iValue);
}

void WriteShort(const int iValue) {
	MM_func = true;
	pfnWriteShort(iValue);
	MM_func = false;
}

void pfnWriteShort(int iValue) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteShort: %d\n",iValue);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteShort: %d\n", iValue);
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteShort)(iValue);
}

void WriteLong(const int iValue) {
	MM_func = true;
	pfnWriteLong(iValue);
	MM_func = false;
}

void pfnWriteLong(int iValue) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteLong: %d\n",iValue);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteLong: %d\n", iValue);
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteLong)(iValue);
}

void WriteAngle(const float flValue) {
	MM_func = true;
	pfnWriteAngle(flValue);
	MM_func = false;
}

void pfnWriteAngle(float flValue) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteAngle: %f\n",flValue);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteAngle: %f\n", static_cast<double>(flValue));
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void*>(&flValue), botMsgIndex);
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteAngle)(flValue);
}

void WriteCoord(const float flValue) {
	MM_func = true;
	pfnWriteCoord(flValue);
	MM_func = false;
}

void pfnWriteCoord(float flValue) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteCoord: %f\n",flValue);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteCoord: %f\n", static_cast<double>(flValue));
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void*>(&flValue), botMsgIndex);
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteCoord)(flValue);
}

void WriteString(const char* sz) {
	MM_func = true;
	pfnWriteString(sz);
	MM_func = false;
}

void pfnWriteString(const char* sz) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteString: %s\n",sz);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteString: %s\n", sz);
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)((void*)sz, botMsgIndex);
	}
	script(sz);

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteString)(sz);
}

void WriteEntity(const int iValue) {
	MM_func = true;
	pfnWriteEntity(iValue);
	MM_func = false;
}

void pfnWriteEntity(int iValue) {
	if (gpGlobals->deathmatch) {
		// if(debug_engine || dont_send_packet) { fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"pfnWriteEntity: %d\n",iValue);
		// std::fclose(fp); }
		if (debug_engine) {
			fp = UTIL_OpenFoxbotLog();
			std::fprintf(fp, "pfnWriteEntity: %d\n", iValue);
			std::fclose(fp);
		}

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void*>(&iValue), botMsgIndex);
	}

	if (mr_meta && MM_func) {
		if (dont_send_packet)
			RETURN_META(MRES_SUPERCEDE);
		RETURN_META(MRES_HANDLED);
	}
	if (dont_send_packet)
		return;
	(*g_engfuncs.pfnWriteEntity)(iValue);
}

void pfnRegUserMsg_common(const char* pszName, int msg) {
	if (std::strcmp(pszName, "VGUIMenu") == 0)
		message_VGUI = msg;
	else if (std::strcmp(pszName, "WeaponList") == 0)
		message_WeaponList = msg;
	else if (std::strcmp(pszName, "CurWeapon") == 0)
		message_CurWeapon = msg;
	else if (std::strcmp(pszName, "AmmoX") == 0)
		message_AmmoX = msg;
	else if (std::strcmp(pszName, "AmmoPickup") == 0)
		message_AmmoPickup = msg;
	else if (std::strcmp(pszName, "WeapPickup") == 0)
		message_WeapPickup = msg;
	else if (std::strcmp(pszName, "ItemPickup") == 0)
		message_ItemPickup = msg;
	else if (std::strcmp(pszName, "Health") == 0)
		message_Health = msg;
	else if (std::strcmp(pszName, "Battery") == 0)
		message_Battery = msg;
	else if (std::strcmp(pszName, "Damage") == 0)
		message_Damage = msg;
	else if (std::strcmp(pszName, "TextMsg") == 0)
		message_TextMsg = msg;
	else if (std::strcmp(pszName, "DeathMsg") == 0)
		message_DeathMsg = msg;
	else if (std::strcmp(pszName, "ScreenFade") == 0)
		message_ScreenFade = msg;
	else if (std::strcmp(pszName, "StatusIcon") == 0)
		message_StatusIcon = msg;
	//
	else if (std::strcmp(pszName, "TeamScore") == 0)
		message_TeamScores = msg;
	else if (std::strcmp(pszName, "StatusText") == 0)
		message_StatusText = msg;
	else if (std::strcmp(pszName, "StatusValue") == 0)
		message_StatusValue = msg;
	else if (std::strcmp(pszName, "Detpack") == 0)
		message_Detpack = msg;
	else if (std::strcmp(pszName, "SecAmmoVal") == 0)
		message_SecAmmoVal = msg;
}

int pfnRegUserMsg_post(const char* pszName, const int iSize) {
	const int msg = META_RESULT_ORIG_RET(int);

	pfnRegUserMsg_common(pszName, msg);

	RETURN_META_VALUE(MRES_IGNORED, 0); // Fix by Jeefo
}

int pfnRegUserMsg_pre(const char* pszName, const int iSize) {
	if (mr_meta)
		RETURN_META_VALUE(MRES_HANDLED, 0);

	const int msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

	pfnRegUserMsg_common(pszName, msg);

	return msg;
}

void pfnClientPrintf(edict_t* pEdict, const PRINT_TYPE ptype, const char* szMsg) {
	if (debug_engine) {
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "pfnClientPrintf: %p %s\n", static_cast<void*>(pEdict), szMsg);
		std::fclose(fp);
	}

	snprintf(sz_error_check, 250, "CPf: %p %s\n", static_cast<void*>(pEdict), szMsg);

	// only send message if its not a bot...
	if (pEdict != nullptr) {
		bool b = false;
		if (!(pEdict->v.flags & FL_FAKECLIENT)) {
			for (auto &client : clients) {
				// if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
				// bots[i].is_used &&
				/*if(clients[i]!=NULL)
				   snprintf(sz_error_check,250,"%s %x %d\n",sz_error_check,clients[i],i);*/
				if (client == pEdict)
					b = true;
				/*if(bots[i].pEdict==pEdict && (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1 ||
				   (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
				   {
				   b=false;

				   //{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"-bot= %x %d\n",pEdict,i); std::fclose(fp); }
				   }*/
			}
		}
		if (b) {
			char cl_name[128] = " -";

         const char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);

			std::strncat(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120 - std::strlen(cl_name));
			std::strncat(cl_name, "-\n", 127 - std::strlen(cl_name));

			//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"cl %d name %s\n",i,cl_name); std::fclose(fp); }
			if (infobuffer == nullptr)
				b = false;
			//	unsigned int u=GETPLAYERWONID(pEdict);
			//	if((u==0 || ENTINDEX(pEdict)==-1))
			//	{
			//		b=false;
			//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"-wonid=0 %d\n",GETPLAYERWONID(pEdict)); std::fclose(fp); }
			//	}
			std::strncat(sz_error_check, cl_name, 250 - std::strlen(sz_error_check));
		}
		if (b) {
			//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"b\n"); std::fclose(fp); }
			//	snprintf(sz_error_check,250,"%s b = %d %d\n",sz_error_check,GETPLAYERWONID(pEdict),ENTINDEX(pEdict));
			(*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
			// else RETURN_META(MRES_HANDLED);
			//(*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
			return;
		}
		std::strncat(sz_error_check, " !b\n", 250 - std::strlen(sz_error_check));
		return;
		// else
		//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"!!b\n"); std::fclose(fp); }
		/*else
			 {
			 if(mr_meta) RETURN_META(MRES_SUPERCEDE);
			 }*/
	}
	std::strncat(sz_error_check, " NULL\n", 250 - std::strlen(sz_error_check));
	//{ fp=UTIL_OpenFoxbotLog(); std::fprintf(fp,"fook\n"); std::fclose(fp); }
	// if(mr_meta) RETURN_META(MRES_SUPERCEDE);
	// if(!mr_meta) (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
	// else RETURN_META(MRES_HANDLED);
	//(*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
	(*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
}

void pfnClPrintf(edict_t* pEdict, PRINT_TYPE ptype, const char* szMsg) {
	if (debug_engine) {
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "pfnClPrintf: %p %s\n", static_cast<void*>(pEdict), szMsg);
		std::fclose(fp);
	}
	snprintf(sz_error_check, 250, "pfnClPrintf: %p %s\n", static_cast<void*>(pEdict), szMsg);

	// only send message if its not a bot...
	if (pEdict != nullptr) {
		bool b = false;
		if ((pEdict->v.flags & FL_FAKECLIENT) != FL_FAKECLIENT) {
			for (auto &client : clients) {
				// if(!((pEdict->v.flags & FL_FAKECLIENT)==FL_FAKECLIENT))
				// bots[i].is_used &&
				/*if(bots[i].pEdict==pEdict
								&& (GETPLAYERWONID(pEdict)==0 || ENTINDEX(pEdict)==-1
								|| (GETPLAYERWONID(pEdict)==-1 && IS_DEDICATED_SERVER())))
				   b=false;*/
				if (client == pEdict)
					b = true;
			}
		}
		if (b) {
			char cl_name[128];
			cl_name[0] = '\0';

         const char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEdict);
			std::strncpy(cl_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "name"), 120);
			/*{ fp=UTIL_OpenFoxbotLog();
							std::fprintf(fp,"cl %d name %s\n",i,cl_name); std::fclose(fp);}*/
			if (cl_name[0] == '\0' || infobuffer == nullptr)
				b = false;
			//	unsigned int u=GETPLAYERWONID(pEdict);
			//	if((u==0 || ENTINDEX(pEdict)==-1))
			//		b=false;
		}
		if (b) {
			RETURN_META(MRES_HANDLED);
		}
		RETURN_META(MRES_SUPERCEDE);
	}
	RETURN_META(MRES_SUPERCEDE);
	//	RETURN_META(MRES_HANDLED);
}

void pfnServerPrint(const char* szMsg) {
	if (debug_engine) {
		fp = UTIL_OpenFoxbotLog();
		std::fprintf(fp, "pfnServerPrint: %s\n", szMsg);
		std::fclose(fp);
	}

	// snprintf(sz_error_check,250,"pfnServerPrint: %s\n",szMsg);

	// if were gonna deal with commands for bots (e.i.'follow user')
	// then this is a good place to start

	// bool loop = true;
	char sz[1024]; // needs to be defined at max message length..is 1024 ok?
	char msgstart[255];
	char buffa[255];
	char cmd[255];
	int i = 0;

	// first compare the message to all bot names, then if bots name is
	// in message pass to bot
	// check that the bot that sent a message isn't getting it back
	std::strncpy(sz, szMsg, 253);
	// clear up sz, and copy start to buffa
	while (i < 250 && sz[i] != ' ') {
		msgstart[i] = sz[i];
		sz[i] = ' ';
		i++;
	}

	msgstart[i] = '\0'; // finish string off
	i = 0;

	/* { fp=UTIL_OpenFoxbotLog();
					std::fprintf(fp,"pfnServerPrint: %s %s\n",sz,msgstart); std::fclose(fp);}*/

					// look through the list of active bots for the intended recipient of
					// the message
	while (i < 32) {
		std::strncpy(buffa, sz, 253);
		int k = 1;
		while (k != 0) {
			// remove start spaces
			int j = 0;
			while (j < 250 && (buffa[j] == ' ' || buffa[j] == '\n')) {
				j++;
			}

			/*{ fp=UTIL_OpenFoxbotLog();
							std::fprintf(fp,"pfnServerPrint: %s\n",buffa); std::fclose(fp); }*/

			k = 0;
			while (j < 250 && k < 250 && (buffa[j] != ' ' && buffa[j] != '\0' && buffa[j] != '\n')) {
				cmd[k] = buffa[j];
				buffa[j] = ' ';
				j++;
				k++;
			}
			cmd[k] = '\0';

			/*			if(bots[i].is_used)
															{
																			fp = UTIL_OpenFoxbotLog();
																			std::fprintf(fp, "pfnServerPrint: cmd|%s bot_name|%s\n", cmd, bots[i].name);
																			std::fclose(fp);
															}*/

															// check that the message was meant for this bot
															// bots[i].name = name obviously
			if ((bots[i].is_used && name_message_check(szMsg, bots[i].name)) || (bots[i].is_used && strcasecmp(cmd, "bots") == 0)) {
				// DONT ALLOW CHANGECLASS TO ALL BOTS
				if (strcasecmp(cmd, "bots") == 0 && std::strstr(szMsg, "changeclass"))
					continue;
				if (strcasecmp(cmd, "bots") == 0 && std::strstr(szMsg, "changeclassnow"))
					continue;

				std::strncpy(bots[i].message, szMsg, 253);
				std::strncpy(bots[i].msgstart, msgstart, 253);
				bots[i].newmsg = true; // tell the bot it has mail
			}
		}

		i++;
	}

	if (mr_meta)
		RETURN_META(MRES_HANDLED);
	(*g_engfuncs.pfnServerPrint)(szMsg);
}

// This function returns true if the bots name is in the indicated message.
static bool name_message_check(const char* msg_string, const char* name_string) {
	const size_t msg_length = std::strlen(msg_string);
	const size_t name_end = std::strlen(name_string) - static_cast<size_t>(1);

	if (msg_length < name_end)
		return false;

	/*	{
									fp = UTIL_OpenFoxbotLog();
									std::fprintf(fp, "bot_name|%s length %d\n", name_string, (int)name_end);
									std::fprintf(fp, "msg|%s length %d\n", msg_string, (int)msg_length);
									std::fclose(fp);
					}*/

	unsigned int j = 0;

	// start the search
	for (unsigned int i = 0; i < msg_length; i++) {
		// does this letter of the message match a character of the bots name?
		if (msg_string[i] == name_string[j]) {
			// found the last matching character of the bots name?
			if (j >= name_end)
				return true;

			++j; // go on to the next character of the bots name
		}
		else
			j = 0; // reset to the start of the bots name
	}

	return false;
}

C_DLLEXPORT int GetEngineFunctions(enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion) {
	if (mr_meta)
		std::memset(pengfuncsFromEngine, 0, sizeof(enginefuncs_t));

	pengfuncsFromEngine->pfnCmd_Args = Cmd_Args;
	pengfuncsFromEngine->pfnCmd_Argv = Cmd_Argv;
	pengfuncsFromEngine->pfnCmd_Argc = Cmd_Argc;
	pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;
	pengfuncsFromEngine->pfnClientPrintf = pfnClientPrintf;
	pengfuncsFromEngine->pfnMessageBegin = MessageBegin;
	pengfuncsFromEngine->pfnMessageEnd = MessageEnd;
	pengfuncsFromEngine->pfnWriteByte = WriteByte;
	pengfuncsFromEngine->pfnWriteChar = WriteChar;
	pengfuncsFromEngine->pfnWriteShort = WriteShort;
	pengfuncsFromEngine->pfnWriteLong = WriteLong;
	pengfuncsFromEngine->pfnWriteAngle = WriteAngle;
	pengfuncsFromEngine->pfnWriteCoord = WriteCoord;
	pengfuncsFromEngine->pfnWriteString = WriteString;
	pengfuncsFromEngine->pfnWriteEntity = WriteEntity;
	pengfuncsFromEngine->pfnServerPrint = pfnServerPrint;
	pengfuncsFromEngine->pfnSetOrigin = pfnSetOrigin;
	pengfuncsFromEngine->pfnRemoveEntity = pfnRemoveEntity;
	pengfuncsFromEngine->pfnRegUserMsg = pfnRegUserMsg_pre;
	pengfuncsFromEngine->pfnFindEntityInSphere = pfnFindEntityInSphere;
	pengfuncsFromEngine->pfnEmitSound = pfnEmitSound;
	pengfuncsFromEngine->pfnEmitAmbientSound = pfnEmitAmbientSound;
	pengfuncsFromEngine->pfnClientCommand = pfnClCom;
	pengfuncsFromEngine->pfnClientPrintf = pfnClPrintf;

	return true;
}

int GetEngineFunctions_Post(enginefuncs_t* pengfuncsFromEngine, int* interfaceVersion) {
	pengfuncsFromEngine->pfnRegUserMsg = pfnRegUserMsg_post;
	return true;
}