//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// botclient.cpp
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
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_job_think.h"
#include "bot_client.h"
#include "bot_weapons.h"
#include "waypoint.h"
#include "bot_navigate.h"

#include <tf_defs.h>

// types of damage to ignore...
#define IGNORE_DAMAGE                                                                                              \
    (DMG_CRUSH | DMG_FREEZE | DMG_SHOCK | DMG_DROWN | DMG_NERVEGAS | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | \
        DMG_SLOWBURN | DMG_SLOWFREEZE)

extern int mod_id;
extern bot_t bots[32];
extern edict_t* clients[32];

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions

int g_state;

// int MatchScores[4];  // doesn't update reliably on all maps

// This message is sent when the TFC VGUI menu is displayed.
void BotClient_TFC_VGUI(void* p, int bot_index)
{
    /*if((*(int *)p) == 2)  // is it a team select menu?

            bots[bot_index].start_action = MSG_TFC_TEAM_SELECT;

            else if((*(int *)p) == 3)  // is is a class selection menu?

            bots[bot_index].start_action = MSG_TFC_CLASS_SELECT;*/
}

// This message is sent when the Counter-Strike VGUI menu is displayed. // Not required for TFC - [APG]RoboCop[CL]
/*void BotClient_CS_VGUI(void* p, int bot_index)
{
    if((*(int*)p) == 2) // is it a team select menu?

        bots[bot_index].start_action = MSG_CS_TEAM_SELECT;

    else if((*(int*)p) == 26) // is is a terrorist model select menu?

        bots[bot_index].start_action = MSG_CS_T_SELECT;

    else if((*(int*)p) == 27) // is is a counter-terrorist model select menu?

        bots[bot_index].start_action = MSG_CS_CT_SELECT;
}

// This message is sent when a menu is being displayed in Counter-Strike.
void BotClient_CS_ShowMenu(void* p, int bot_index)
{
    static int state = 0; // current state machine state

    if(state < 3) {
        state++; // ignore first 3 fields of message
        return;
    }

    if(strcmp((char*)p, "#Team_Select") == 0) // team select menu?
    {
        bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
    } else if(strcmp((char*)p, "#Terrorist_Select") == 0) // T model select?
    {
        bots[bot_index].start_action = MSG_CS_T_SELECT;
    } else if(strcmp((char*)p, "#CT_Select") == 0) // CT model select menu?
    {
        bots[bot_index].start_action = MSG_CS_CT_SELECT;
    }

    state = 0; // reset state machine
}

// This message is sent when the Op4 VGUI menu is displayed.
void BotClient_Gearbox_VGUI(void* p, int bot_index)
{
    if((*(int*)p) == 2) // is it a team select menu?

        bots[bot_index].start_action = MSG_OPFOR_TEAM_SELECT;

    else if((*(int*)p) == 3) // is is a class selection menu?

        bots[bot_index].start_action = MSG_OPFOR_CLASS_SELECT;
}

// This message is sent when the FrontLineForce VGUI menu is displayed.
void BotClient_FLF_VGUI(void* p, int bot_index)
{
    if((*(int*)p) == 2) // is it a team select menu?
        bots[bot_index].start_action = MSG_FLF_TEAM_SELECT;
    else if((*(int*)p) == 3) // is it a class selection menu?
        bots[bot_index].start_action = MSG_FLF_CLASS_SELECT;
    else if((*(int*)p) == 70) // is it a weapon selection menu?
        bots[bot_index].start_action = MSG_FLF_WEAPON_SELECT;
    else if((*(int*)p) == 72) // is it a submachine gun selection menu?
        bots[bot_index].start_action = MSG_FLF_SUBMACHINE_SELECT;
    else if((*(int*)p) == 73) // is it a shotgun selection menu?
        bots[bot_index].start_action = MSG_FLF_SHOTGUN_SELECT;
    else if((*(int*)p) == 75) // is it a rifle selection menu?
        bots[bot_index].start_action = MSG_FLF_RIFLE_SELECT;
    else if((*(int*)p) == 76) // is it a pistol selection menu?
        bots[bot_index].start_action = MSG_FLF_PISTOL_SELECT;
    else if((*(int*)p) == 78) // is it a heavyweapons selection menu?
        bots[bot_index].start_action = MSG_FLF_HEAVYWEAPONS_SELECT;
}
*/
// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
void BotClient_Valve_WeaponList(void* p, int bot_index)
{
    static int state = 0; // current state machine state
    static bot_weapon_t bot_weapon;

    if(state == 0) {
        state++;
        strcpy(bot_weapon.szClassname, (char*)p);
    } else if(state == 1) {
        state++;
        bot_weapon.iAmmo1 = *(int*)p; // ammo index 1
    } else if(state == 2) {
        state++;
        bot_weapon.iAmmo1Max = *(int*)p; // max ammo1
    } else if(state == 3) {
        state++;
        bot_weapon.iAmmo2 = *(int*)p; // ammo index 2
    } else if(state == 4) {
        state++;
        bot_weapon.iAmmo2Max = *(int*)p; // max ammo2
    } else if(state == 5) {
        state++;
        bot_weapon.iSlot = *(int*)p; // slot for this weapon
    } else if(state == 6) {
        state++;
        bot_weapon.iPosition = *(int*)p; // position in slot
    } else if(state == 7) {
        state++;
        bot_weapon.iId = *(int*)p; // weapon ID
    } else if(state == 8) {
        state = 0;

        bot_weapon.iFlags = *(int*)p; // flags for weapon (WTF???)

        // store away this weapon with it's ammo information...
        weapon_defs[bot_weapon.iId] = bot_weapon;
    }
}

void BotClient_TFC_WeaponList(void* p, int bot_index)
{
    // this is just like the Valve Weapon List message
    BotClient_Valve_WeaponList(p, bot_index);
}
// Not required for TFC - [APG]RoboCop[CL]
/*void BotClient_CS_WeaponList(void* p, int bot_index)
{
    // this is just like the Valve Weapon List message
    BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_Gearbox_WeaponList(void* p, int bot_index)
{
    // this is just like the Valve Weapon List message
    BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_FLF_WeaponList(void* p, int bot_index)
{
    // this is just like the Valve Weapon List message
    BotClient_Valve_WeaponList(p, bot_index);
}
*/
// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
void BotClient_Valve_CurrentWeapon(void* p, int bot_index)
{
    // static int state = 0;	// current state machine state
    static int iState;
    static int iId;
    static int iClip;

    if(g_state == 0) {
        g_state++;
        iState = *(int*)p; // state of the current weapon
    } else if(g_state == 1) {
        g_state++;
        iId = *(int*)p; // weapon ID of current weapon
    } else if(g_state == 2) {
        // state = 0;

        iClip = *(int*)p; // ammo currently in the clip for this weapon

        if(iId <= 31) {
            bots[bot_index].bot_weapons |= (1 << iId); // set this weapon bit

            if(iState == 1) {
                bots[bot_index].current_weapon.iId = iId;
                bots[bot_index].current_weapon.iClip = iClip;

                // update the ammo counts for this weapon...
                bots[bot_index].current_weapon.iAmmo1 = bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo1];
                bots[bot_index].current_weapon.iAmmo2 = bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo2];
            }
        }
    }
}

void BotClient_TFC_CurrentWeapon(void* p, int bot_index)
{
    // this is just like the Valve Current Weapon message
    BotClient_Valve_CurrentWeapon(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_CurrentWeapon(void* p, int bot_index)
{
    // this is just like the Valve Current Weapon message
    BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_Gearbox_CurrentWeapon(void* p, int bot_index)
{
    // this is just like the Valve Current Weapon message
    BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_FLF_CurrentWeapon(void* p, int bot_index)
{
    // this is just like the Valve Current Weapon message
    BotClient_Valve_CurrentWeapon(p, bot_index);
}
*/
// This message is sent whenever ammo ammounts are adjusted (up or down).
void BotClient_Valve_AmmoX(void* p, int bot_index)
{
    // static int state = 0;	// current state machine state
    static int index;
    static int ammount;

    if(g_state == 0) {
        g_state++;
        index = *(int*)p; // ammo index (for type of ammo)
    } else if(g_state == 1) {
        // state = 0;

        ammount = *(int*)p; // the amount of ammo currently available

        bots[bot_index].m_rgAmmo[index] = ammount; // store it away

        int ammo_index = bots[bot_index].current_weapon.iId;

        // update the ammo counts for this weapon...
        bots[bot_index].current_weapon.iAmmo1 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
        bots[bot_index].current_weapon.iAmmo2 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
    }
}

void BotClient_TFC_AmmoX(void* p, int bot_index)
{
    // this is just like the Valve AmmoX message
    BotClient_Valve_AmmoX(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_AmmoX(void* p, int bot_index)
{
    // this is just like the Valve AmmoX message
    BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_Gearbox_AmmoX(void* p, int bot_index)
{
    // this is just like the Valve AmmoX message
    BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_FLF_AmmoX(void* p, int bot_index)
{
    // this is just like the Valve AmmoX message
    BotClient_Valve_AmmoX(p, bot_index);
}
*/
// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_Valve_AmmoPickup(void* p, int bot_index)
{
    static int state = 0; // current state machine state
    static int index;
    static int ammount;

    if(state == 0) {
        state++;
        index = *(int*)p;
    } else if(state == 1) {
        state = 0;

        ammount = *(int*)p;

        bots[bot_index].m_rgAmmo[index] = ammount;

        const int ammo_index = bots[bot_index].current_weapon.iId;

        // update the ammo counts for this weapon...
        bots[bot_index].current_weapon.iAmmo1 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
        bots[bot_index].current_weapon.iAmmo2 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
    }
}

void BotClient_TFC_AmmoPickup(void* p, int bot_index)
{
    // this is just like the Valve Ammo Pickup message
    BotClient_Valve_AmmoPickup(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_AmmoPickup(void* p, int bot_index)
{
    // this is just like the Valve Ammo Pickup message
    BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_Gearbox_AmmoPickup(void* p, int bot_index)
{
    // this is just like the Valve Ammo Pickup message
    BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_FLF_AmmoPickup(void* p, int bot_index)
{
    // this is just like the Valve Ammo Pickup message
    BotClient_Valve_AmmoPickup(p, bot_index);
}
*/
// This message gets sent when the bot picks up a weapon.
void BotClient_Valve_WeaponPickup(void* p, int bot_index)
{
    int index;

    index = *(int*)p;

    // set this weapon bit to indicate that we are carrying this weapon
    bots[bot_index].bot_weapons |= (1 << index);
}

void BotClient_TFC_WeaponPickup(void* p, int bot_index)
{
    // this is just like the Valve Weapon Pickup message
    BotClient_Valve_WeaponPickup(p, bot_index);
}
/*// Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_WeaponPickup(void* p, int bot_index)
{
    // this is just like the Valve Weapon Pickup message
    BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_Gearbox_WeaponPickup(void* p, int bot_index)
{
    // this is just like the Valve Weapon Pickup message
    BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_FLF_WeaponPickup(void* p, int bot_index)
{
    // this is just like the Valve Weapon Pickup message
    BotClient_Valve_WeaponPickup(p, bot_index);
}
*/
// This message gets sent when the bot picks up an item (like a battery
// or a healthkit)
void BotClient_Valve_ItemPickup(void* p, int bot_index)
{
}

void BotClient_TFC_ItemPickup(void* p, int bot_index)
{
    // this is just like the Valve Item Pickup message
    BotClient_Valve_ItemPickup(p, bot_index);
    int index;

    index = *(int*)p;
    char msg[255];
    sprintf(msg, "%d", index);
    // UTIL_HostSay(0, 0, msg);
    // FILE *fp;
    //{ fp=fopen("route.txt","a"); fprintf(fp,"a %s\n",msg); fclose(fp); }
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_ItemPickup(void* p, int bot_index)
{
    // this is just like the Valve Item Pickup message
    BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_Gearbox_ItemPickup(void* p, int bot_index)
{
    // this is just like the Valve Item Pickup message
    BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_FLF_ItemPickup(void* p, int bot_index)
{
    // this is just like the Valve Item Pickup message
    BotClient_Valve_ItemPickup(p, bot_index);
}
*/
// This message gets sent when the bots health changes.
void BotClient_Valve_Health(void* p, int bot_index)
{
    bots[bot_index].bot_real_health = *(int*)p; // health ammount
}

void BotClient_TFC_Health(void* p, int bot_index)
{
    // this is just like the Valve Health message
    BotClient_Valve_Health(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_Health(void* p, int bot_index)
{
    // this is just like the Valve Health message
    BotClient_Valve_Health(p, bot_index);
}

void BotClient_Gearbox_Health(void* p, int bot_index)
{
    // this is just like the Valve Health message
    BotClient_Valve_Health(p, bot_index);
}

void BotClient_FLF_Health(void* p, int bot_index)
{
    // this is just like the Valve Health message
    BotClient_Valve_Health(p, bot_index);
}
*/
// This message gets sent when the bots armor changes.
void BotClient_Valve_Battery(void* p, int bot_index)
{
    bots[bot_index].bot_armor = *(int*)p; // armor ammount
}

void BotClient_TFC_Battery(void* p, int bot_index)
{
    // this is just like the Valve Battery message
    BotClient_Valve_Battery(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_Battery(void* p, int bot_index)
{
    // this is just like the Valve Battery message
    BotClient_Valve_Battery(p, bot_index);
}

void BotClient_Gearbox_Battery(void* p, int bot_index)
{
    // this is just like the Valve Battery message
    BotClient_Valve_Battery(p, bot_index);
}

void BotClient_FLF_Battery(void* p, int bot_index)
{
    // this is just like the Valve Battery message
    BotClient_Valve_Battery(p, bot_index);
}
*/
// This message gets sent when the bots are getting damaged.
// Note: This message might also be sent after a bot died and has
// respawned reporting the damage that killed it.
// i.e. after BotSpawnInit() is called.
void BotClient_Valve_Damage(void* p, int bot_index)
{
    static int state = 0; // current state machine state
    static int damage_armor;
    static int damage_taken;
    static int damage_bits; // type of damage being done
    static Vector damage_origin;

    if(state == 0) {
        state++;
        damage_armor = *(int*)p;
    } else if(state == 1) {
        state++;
        damage_taken = *(int*)p;
    } else if(state == 2) {
        state++;
        damage_bits = *(int*)p;
    } else if(state == 3) {
        state++;
        damage_origin.x = *(float*)p;
    } else if(state == 4) {
        state++;
        damage_origin.y = *(float*)p;
    } else if(state == 5) {
        state = 0;

        damage_origin.z = *(float*)p;

        if(mod_id == TFC_DLL) {
            if((damage_bits & DMG_SONIC) == DMG_SONIC) {
                bots[bot_index].disturbedViewAmount = 45;
                bots[bot_index].f_disturbedViewTime = bots[bot_index].f_think_time + 5.0f;
            } else if((damage_bits & DMG_BURN) == DMG_BURN) {
                // added by Zybby, burning bots aim worse too
                bots[bot_index].disturbedViewAmount = 15;
                bots[bot_index].f_disturbedViewTime = bots[bot_index].f_think_time + 2.0f;
            } else if((damage_bits & DMG_BLAST) == DMG_BLAST) {
                // blown up bots aim worse too
                bots[bot_index].disturbedViewAmount = 10;
                bots[bot_index].f_disturbedViewTime = bots[bot_index].f_think_time + 2.0f;
            }
        }

        // let the bot know it's getting hurt
        if((damage_armor > 0) || (damage_taken > 0)) {
            // drowning detection - set up a job to avoid death by drowning
            if(damage_bits & DMG_DROWN
                //	&& bots[bot_index].pEdict->v.waterlevel == WL_HEAD_IN_WATER
                && bots[bot_index].bot_skill < 3) {
                job_struct* newJob = InitialiseNewJob(&bots[bot_index], JOB_DROWN_RECOVER);
                if(newJob != NULL) {
                    newJob->waypoint = BotDrowningWaypointSearch(&bots[bot_index]);
                    if(newJob->waypoint != -1)
                        SubmitNewJob(&bots[bot_index], JOB_DROWN_RECOVER, newJob);
                }
            }

            // ignore certain types of damage(e.g. environmental)
            if(damage_bits & IGNORE_DAMAGE)
                return;

            // if the bot didn't spawn very recently
            if(gpGlobals->time > (bots[bot_index].last_spawn_time + 1.0f)) {
                bots[bot_index].f_injured_time = gpGlobals->time;

                // stop using health or HEV stations...
                bots[bot_index].b_use_health_station = FALSE;
                bots[bot_index].b_use_HEV_station = FALSE;
            }
        }
    }
}

void BotClient_TFC_Damage(void* p, int bot_index)
{
    BotClient_Valve_Damage(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_Damage(void* p, int bot_index)
{
    // this is just like the Valve Battery message
    BotClient_Valve_Damage(p, bot_index);
}

void BotClient_Gearbox_Damage(void* p, int bot_index)
{
    // this is just like the Valve Battery message
    BotClient_Valve_Damage(p, bot_index);
}

void BotClient_FLF_Damage(void* p, int bot_index)
{
    // this is just like the Valve Battery message
    BotClient_Valve_Damage(p, bot_index);
}
*/
// This message gets sent when the bots money ammount changes
// For Counterstrike.
/*void BotClient_CS_Money(void *p, int bot_index)
{
        static int state = 0; // current state machine state

        if(state == 0)
        {
                state++;

                bots[bot_index].bot_money = *(int *)p; // amount of money
        }
        else
        {
                state = 0; // ingore this field
        }
}*/

// This message gets sent when the bots get killed
void BotClient_Valve_DeathMsg(void* p, int bot_index)
{
    static int state = 0; // current state machine state
    static int killer_index;
    static int victim_index;
    static edict_t* victim_edict;
    static edict_t* killer_edict;
    static int index;
    static int indexb;

    if(state == 0) {
        state++;
        killer_index = *(int*)p; // ENTINDEX() of killer
    } else if(state == 1) {
        state++;
        victim_index = *(int*)p; // ENTINDEX() of victim
    } else if(state == 2) {
        state = 0;

        victim_edict = INDEXENT(victim_index);

        index = UTIL_GetBotIndex(victim_edict);

        // do message return for human client, so bot can do
        // haha killed ya message
        // FILE *fp;
        //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"kill messages\n"); fclose(fp); }
        if(index == -1) {
            if((killer_index == 0) || (killer_index == victim_index)) {
                // bot killed by world (worldspawn) or bot killed self...
                // bots[index].killer_edict = NULL;
            } else {
                killer_edict = INDEXENT(killer_index);
                indexb = UTIL_GetBotIndex(killer_edict);
                if(indexb != -1 && victim_edict != NULL) {
                    if(UTIL_GetTeam(killer_edict) != UTIL_GetTeam(victim_edict)) {
                        bots[indexb].killed_edict = victim_edict;
                    }
                }
            }
        }

        // is this message about a bot being killed?
        if(index != -1) {
            if((killer_index == 0) || (killer_index == victim_index)) {
                // bot killed by world (worldspawn) or bot killed self...
                bots[index].killer_edict = NULL;
            } else {
                // store edict of player that killed this bot...
                bots[index].killer_edict = INDEXENT(killer_index);
                killer_edict = INDEXENT(killer_index);
                indexb = UTIL_GetBotIndex(killer_edict);
                if(indexb != -1 && victim_edict != NULL) {
                    if(UTIL_GetTeam(killer_edict) != UTIL_GetTeam(victim_edict)) {
                        bots[indexb].killed_edict = victim_edict;
                    }
                }
            }
        }
    }
}

void BotClient_TFC_DeathMsg(void* p, int bot_index)
{
    // this is just like the Valve DeathMsg message
    BotClient_Valve_DeathMsg(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_DeathMsg(void* p, int bot_index)
{
    // this is just like the Valve DeathMsg message
    BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_Gearbox_DeathMsg(void* p, int bot_index)
{
    // this is just like the Valve DeathMsg message
    BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_FLF_DeathMsg(void* p, int bot_index)
{
    // this is just like the Valve DeathMsg message
    BotClient_Valve_DeathMsg(p, bot_index);
}
*/
// This message gets sent when a text message is displayed
/*void BotClient_FLF_TextMsg(void *p, int bot_index)
{
        static int state = 0; // current state machine state
        static int msg_dest = 0;

        if(state == 0)
        {
                state++;
                msg_dest = *(int *)p; // HUD_PRINTCENTER, etc.
        }
        else if(state == 1)
        {
                state = 0;

                if(strcmp((char *)p, "You are Attacking\n") == 0) // attacker msg
                {
                        bots[bot_index].defender = 0; // attacker
                }
                else if(strcmp((char *)p, "You are Defending\n") == 0) // defender msg
                {
                        bots[bot_index].defender = 1; // defender
                }
        }
}

// This message gets sent when the WarmUpTime is enabled/disabled
void BotClient_FLF_WarmUp(void *p, int bot_index)
{
//	bots[bot_index].warmup = *(int *)p;
}

// This message gets sent to ALL when the WarmUpTime is enabled/disabled
void BotClient_FLF_WarmUpAll(void *p, int bot_index)
{
        for(int i = 0; i < 32; i++)
        {
                if(bots[i].is_used) // count the number of bots in use
                        bots[i].warmup = *(int *)p;
        }
}

// This message gets sent when the round is over
void BotClient_FLF_WinMessage(void *p, int bot_index)
{
        for(int i = 0; i < 32; i++)
        {
                if(bots[i].is_used) // count the number of bots in use
                        bots[i].round_end = 1;
        }
}

// This message gets sent when a temp entity is created
void BotClient_FLF_TempEntity(void *p, int bot_index)
{
        static int state = 0; // current state machine state
        static int te_type; // TE_ message type

        if(p == NULL) // end of message?
        {
                state = 0;
                return;
        }

        if(state == 0)
        {
                state++;
                te_type = *(int *)p;

                return;
        }

        if(te_type == TE_TEXTMESSAGE)
        {
                if(state == 16)
                {
                        if(strncmp((char *)p, "Capturing ", 10) == 0)
                        {
                                // if bot is currently capturing, keep timer alive...
                                if(bots[bot_index].b_use_capture)
                                        bots[bot_index].f_use_capture_time = gpGlobals->time + 2.0;
                        }
                }

                state++;
        }
}*/

void BotClient_Valve_ScreenFade(void* p, int bot_index)
{
    static int state = 0; // current state machine state
    static int duration;
    static int hold_time;
    static int fade_flags;

    if(state == 0) {
        state++;
        duration = *(int*)p;
    } else if(state == 1) {
        state++;
        hold_time = *(int*)p;
    } else if(state == 2) {
        state++;
        fade_flags = *(int*)p;
    } else if(state == 6) {
        state = 0;

        int length = (duration + hold_time) / 4096;
        bots[bot_index].f_blinded_time = gpGlobals->time + length - 2.0;
    } else {
        state++;
    }
}

void BotClient_TFC_ScreenFade(void* p, int bot_index)
{
    // this is just like the Valve ScreenFade message
    BotClient_Valve_ScreenFade(p, bot_index);
}
/* // Not required for TFC - [APG]RoboCop[CL]
void BotClient_CS_ScreenFade(void* p, int bot_index)
{
    // this is just like the Valve ScreenFade message
    BotClient_Valve_ScreenFade(p, bot_index);
}

void BotClient_Gearbox_ScreenFade(void* p, int bot_index)
{
    // this is just like the Valve ScreenFade message
    BotClient_Valve_ScreenFade(p, bot_index);
}

void BotClient_FLF_ScreenFade(void* p, int bot_index)
{
    // this is just like the Valve ScreenFade message
    BotClient_Valve_ScreenFade(p, bot_index);
}
*/
// This function can be used to monitor which icons are displayed on the bot's HUD.
// NOTE: This can be used to see if a bot is tranquilised or hallucinating but not
// if it's infected.
void BotClient_TFC_StatusIcon(void* p, int bot_index)
{
    /*	static int icon_status = 0;

            if(g_state == 0)
            {
                    icon_status = *(int *)p;
                    ++g_state;
            }
            else if(g_state == 1)
            {
                    // we could detect caltrop damage here
                    if(strncmp("dmg_", (char *)p, 4) == 0)
                            UTIL_BotLogPrintf("HUD: %s, status %d\n",
                                    (char *)p, icon_status);
            }*/
}

// This function can be used to track messages to TFC engineer bots telling
// them about the status of what they are building.
// From what I've seen a bot can be told by this function that it has
// built a sentry gun before the sentry gun even exists!
void BotClient_Engineer_BuildStatus(void* p, int bot_index)
{
    // here are some interesting messages(so far unused):
    // #Sentry_shellslow
    // #Sentry_sbar n%%	  (where n is a number from 0 to 100 I think)
    // #Sentry_finish		 (#Sentry_built seems to come after this)
    // #Sentry_repair
    // #Sentry_upgrade
    // #Sentry_destroyed
    // #Build_onesentry		(just tried to make two?)
    // #Build_stop			  (e.g. stopped building a sentry)
    // #Observer				 (not engineer related, but received by this function)
    // #Spy_disguised Red #Sniper
    // #Teleporter_entrance_idle

    // which tp component building
    static int teleportType = 0;

    if(g_state == 0) {
        g_state++;
    } else if(g_state == 1) {
        //	UTIL_BotLogPrintf("%s, message %s\n", bots[bot_index].name, (char *)p);

        if(strcmp((char*)p, "#Dispenser_used") == 0) // enemy is using bots dispenser
        {
            if(bot_index != -1) {
                // set up the time when the bot will detonate it's dispenser
                if(bots[bot_index].bot_skill < 3)
                    bots[bot_index].f_dispenserDetTime =
                        gpGlobals->time + random_float(0.5, static_cast<float>(bots[bot_index].bot_skill) + 0.5);
            }
            g_state++;
        } else if(strcmp((char*)p, "#Teleporter_Entrance_Built") == 0) {
            teleportType = W_FL_TFC_TELEPORTER_ENTRANCE;
            g_state++;
        } else if(strcmp((char*)p, "#Teleporter_Exit_Built") == 0) {
            teleportType = W_FL_TFC_TELEPORTER_EXIT;
            g_state++;
        } else {
            // do script handling... as hunted info gets passed in here
            script((char*)p);
        }
    } else if(g_state == 2) {
        // here we can track objects built by human players
        if(bot_index == -1) {
            // The builders name
            char builder[128];
            strncpy(builder, (char*)p, 128);
            builder[127] = '\0';

            // Loop through the humans to look for the builder
            for(int i = 0; i < 32; i++) {
                // is this client the builder?
                if(clients[i] && strcmp(STRING(clients[i]->v.netname), builder) == 0) {
                    // Get the teleporter ent
                    edict_t* pent = NULL;
                    while(
                        (pent = FIND_ENTITY_IN_SPHERE(pent, clients[i]->v.origin, 200)) != NULL && (!FNullEnt(pent))) {
                        float l = (clients[i]->v.origin - pent->v.origin).Length2D();

                        if((strcmp("building_teleporter", STRING(pent->v.classname)) == 0) && l >= 16.0 && l <= 96.0) {
                            // Set the owner on the teleport
                            pent->v.euser1 = clients[i];
                            pent->v.iuser1 = teleportType;
                            teleportType = 0;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

void BotClient_TFC_SentryAmmo(void* p, int bot_index)
{
    // static int state = 0;	// current state machine state
    static int val;

    if(g_state == 0) {
        g_state++;
        val = *(int*)p;
    } else if(g_state == 1) {
        if(val == 4) {
            bots[bot_index].sentry_ammo = *(int*)p;
        }
    }
}

void BotClient_TFC_DetPack(void* p, int bot_index)
{
    // static int state = 0;	// current state machine state
    // static int val;

    /*if(g_state == 0)
            {
            //g_state++;
            val = *(int *)p;
            }*/

    bots[bot_index].detpack = *(int*)p;

    /*	FILE *fp = UTIL_OpenFoxbotLog();
            if(fp != NULL)
            {
                    fprintf(fp,"%s detpack set to %d\n",
                            bots[bot_index].name, bots[bot_index].detpack);
                    fclose(fp);
            }*/
}

void BotClient_Menu(void* p, int bot_index)
{
    static int val, s;
    if(g_state == 0) {
        // g_state++;
        val = *(int*)p;
        s = 0;
        int i = 10;
        while(!((1 << i & val) == 1 << i) && i >= 0)
            i--;
        s = i + 1;

        // FILE *fp;
        //{ fp=UTIL_OpenFoxbotLog(); fprintf(fp,"%d %d\n",*(int *)p,selection); fclose(fp); }
    }
    /*if(g_state==3)
            {
                     if(s>0)
                     {
                             int c=RANDOM_LONG(1,s);
                             char msg[20];
                             itoa(c,msg,10);
                             FakeClientCommand(bots[bot_index].pEdict,"menuselect",msg,NULL);
                     }
            }*/
    g_state++;
}

// update the amount of grenades a bot carries
void BotClient_TFC_Grens(void* p, int bot_index)
{
    static int gren;

    if(g_state == 0) {
        g_state++;
        gren = *(int*)p;
    } else if(g_state == 1) {
        if(gren == 0)
            bots[bot_index].grenades[0] = *(int*)p;
        else if(gren == 1)
            bots[bot_index].grenades[1] = *(int*)p;
    }
}

#if 0 // don't compile this function yet, it's not reliable enough
// This function can update the team scores data.
// This function is unreliable because the names of the teams can change.
// For example Red are "Red" on most maps, but on hunted they are called
// "Bodyguards", and on dustbowl they are called "Defenders".
void BotClient_TFC_Scores(void *p, int bot_index)
{
	// current state machine state
	// (used instead of g_state for maximum reliability)
	static int state = 0;

/*	FILE *fp;
	{
		fp = UTIL_OpenFoxbotLog();
		if(fp != NULL)
		{
			fprintf(fp,"state %d  p %d\n", state, *(int *)p);
			fclose(fp);
		}
	}*/

	static int team = -1;

	if(state == 0)
	{
		if(strncmp((char *)p, "Blue", 4) == 0)
			team = 0;
		else if(strncmp((char *)p, "Red", 3) == 0)
			team = 1;
		else if(strncmp((char *)p, "Yellow", 6) == 0)
			team = 2;
		else if(strncmp((char *)p, "Green", 5) == 0)
			team = 3;
		else team = -1;  // just in case

		state = 1;
	}
	else if(state == 1)
	{
		if(team != -1)
			MatchScores[team] = *(int *)p;

		state = 2;
	}
	else if(state == 2)
		state = 0;
}
#endif
