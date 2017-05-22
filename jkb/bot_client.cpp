//
// JK_Botti - be more human!
//
// bot_client.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_client.h"
#include "bot_weapons.h"
#include "player.h"

// types of damage to ignore...
#define IGNORE_DAMAGE (DMG_CRUSH | DMG_BURN | DMG_FREEZE | DMG_FALL | \
                       DMG_SHOCK | DMG_DROWN | DMG_NERVEGAS | DMG_RADIATION | \
                       DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN | \
                       DMG_SLOWFREEZE | 0xFF000000)

extern bot_t bots[32];
extern int num_logos;

extern int bot_taunt_count;
extern int recent_bot_taunt[];
extern bot_chat_t bot_taunt[MAX_BOT_CHAT];

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions


// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
void BotClient_Valve_WeaponList(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static bot_weapon_t bot_weapon;

   if (state == 0)
   {
      state++;
      safe_strcopy(bot_weapon.szClassname, sizeof(bot_weapon.szClassname), (char *)p);
   }
   else if (state == 1)
   {
      state++;
      bot_weapon.iAmmo1 = *(int *)p;  // ammo index 1
   }
   else if (state == 2)
   {
      state++;
      bot_weapon.iAmmo1Max = *(int *)p;  // max ammo1
   }
   else if (state == 3)
   {
      state++;
      bot_weapon.iAmmo2 = *(int *)p;  // ammo index 2
   }
   else if (state == 4)
   {
      state++;
      bot_weapon.iAmmo2Max = *(int *)p;  // max ammo2
   }
   else if (state == 5)
   {
      state++;
      bot_weapon.iSlot = *(int *)p;  // slot for this weapon
   }
   else if (state == 6)
   {
      state++;
      bot_weapon.iPosition = *(int *)p;  // position in slot
   }
   else if (state == 7)
   {
      state++;
      bot_weapon.iId = *(int *)p;  // weapon ID
   }
   else if (state == 8)
   {
      state = 0;

      bot_weapon.iFlags = *(int *)p;  // flags for weapon (WTF???)

      // store away this weapon with it's ammo information...
      weapon_defs[bot_weapon.iId] = bot_weapon;
   }
}


// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
void BotClient_Valve_CurrentWeapon(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int iState;
   static int iId;
   static int iClip;

   if (state == 0)
   {
      state++;
      iState = *(int *)p;  // state of the current weapon
   }
   else if (state == 1)
   {
      state++;
      iId = *(int *)p;  // weapon ID of current weapon
   }
   else if (state == 2)
   {
      state = 0;

      iClip = *(int *)p;  // ammo currently in the clip for this weapon

      if (iId <= 31)
      {
         if (iState == 1)
         {
            bot_weapon_select_t *pSelect = NULL;
            int found = 0;
            
            bots[bot_index].current_weapon.iId = iId;
            bots[bot_index].current_weapon.iClip = iClip;

            // update the ammo counts for this weapon...
            bots[bot_index].current_weapon.iAmmo1 = bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo1];
            bots[bot_index].current_weapon.iAmmo2 = bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo2];

            pSelect = &weapon_select[0];
            
            for(int i = 0; pSelect && pSelect[i].iId; i++) 
            {
               if(iId == pSelect[i].iId) 
               	{
                  bots[bot_index].current_opt_distance = pSelect[i].opt_distance;
                  bots[bot_index].current_weapon_index = i;
                  found = 1;
               	  break;
               }
            }
            
            if(!found)
               bots[bot_index].current_opt_distance = 99999.0;
         }
      }
   }
}

// This message is sent when a weapon is selected.
void PlayerClient_Valve_CurrentWeapon(void *p, int player_index)
{
   static int state = 0;   // current state machine state
   static int iState;
   static int iId;
   //static int iClip;

   if (state == 0)
   {
      state++;
      iState = *(int *)p;  // state of the current weapon
   }
   else if (state == 1)
   {
      state++;
      iId = *(int *)p;  // weapon ID of current weapon
   }
   else if (state == 2)
   {
      state = 0;

      //iClip = *(int *)p;  // ammo currently in the clip for this weapon

      if (iId <= 31)
      {
         if (iState == 1)
         {
            players[player_index].current_weapon_id = iId;
         }
      }
   }
}


// This message is sent whenever ammo ammounts are adjusted (up or down).
void BotClient_Valve_AmmoX(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int index;
   static int ammount;
   int ammo_index;

   if (state == 0)
   {
      state++;
      index = *(int *)p;  // ammo index (for type of ammo)
   }
   else if (state == 1)
   {
      state = 0;

      ammount = *(int *)p;  // the ammount of ammo currently available

      bots[bot_index].m_rgAmmo[index] = ammount;  // store it away

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
   }
}


// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_Valve_AmmoPickup(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int index;
   static int ammount;
   int ammo_index;

   if (state == 0)
   {
      state++;
      index = *(int *)p;
   }
   else if (state == 1)
   {
      state = 0;

      ammount = *(int *)p;

      bots[bot_index].m_rgAmmo[index] = ammount;

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
   }
}


// This message gets sent when the bot picks up a weapon.
void BotClient_Valve_WeaponPickup(void *p, int bot_index)
{
}


// This message gets sent when the bot picks up an item (like a battery
// or a healthkit)
void BotClient_Valve_ItemPickup(void *p, int bot_index)
{
   char itemname[64];
   
   safe_strcopy(itemname, sizeof(itemname), (char *)p);

   if (strcmp(itemname, "item_longjump") == 0)
   {
      bots[bot_index].b_longjump = TRUE;
      bots[bot_index].f_combat_longjump = gpGlobals->time + 0.2;
      bots[bot_index].b_combat_longjump = FALSE;
   }
}


// This message gets sent when the bots health changes.
void BotClient_Valve_Health(void *p, int bot_index)
{
}


// This message gets sent when the bots armor changes.
void BotClient_Valve_Battery(void *p, int bot_index)
{
}


// This message gets sent when the bots are getting damaged.
void BotClient_Valve_Damage(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int damage_armor;
   static int damage_taken;
   static int damage_bits;  // type of damage being done
   static Vector damage_origin;

   if (state == 0)
   {
      state++;
      damage_armor = *(int *)p;
   }
   else if (state == 1)
   {
      state++;
      damage_taken = *(int *)p;
   }
   else if (state == 2)
   {
      state++;
      damage_bits = *(int *)p;
   }
   else if (state == 3)
   {
      state++;
      damage_origin.x = *(float *)p;
   }
   else if (state == 4)
   {
      state++;
      damage_origin.y = *(float *)p;
   }
   else if (state == 5)
   {
      state = 0;

      damage_origin.z = *(float *)p;

      if ((damage_armor > 0) || (damage_taken > 0))
      {
         // ignore certain types of damage...
         if (damage_bits & IGNORE_DAMAGE)
            return;

         bots[bot_index].f_last_time_attacked = gpGlobals->time;

         // if the bot doesn't have an enemy and someone is shooting at it then
         // turn in the attacker's direction...
         if (bots[bot_index].pBotEnemy == NULL || !FPredictedVisible(bots[bot_index]))
         {
            // face the attacker...
            Vector v_enemy = damage_origin - bots[bot_index].pEdict->v.origin;
            Vector bot_angles = UTIL_VecToAngles( v_enemy );

            bots[bot_index].pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(bots[bot_index].pEdict);
         
            // stop using health or HEV stations...
            bots[bot_index].b_use_health_station = FALSE;
            bots[bot_index].b_use_HEV_station = FALSE;
         }
      }
   }
}


// This message gets sent when the bots get killed
void BotClient_Valve_DeathMsg(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int killer_index;
   static int victim_index;
   static edict_t *killer_edict;
   static edict_t *victim_edict;
   static int index;

   if (state == 0)
   {
      state++;
      killer_index = *(int *)p;  // ENTINDEX() of killer
   }
   else if (state == 1)
   {
      state++;
      victim_index = *(int *)p;  // ENTINDEX() of victim
   }
   else if (state == 2)
   {
      state = 0;

      killer_edict = INDEXENT(killer_index);
      victim_edict = INDEXENT(victim_index);

      // get the bot index of the killer...
      index = UTIL_GetBotIndex(killer_edict);

      // is this message about a bot killing someone?
      if (index != -1)
      {
         if (killer_index != victim_index)  // didn't kill self...
         {
            if ((RANDOM_LONG2(1, 100) <= bots[index].logo_percent) && (num_logos))
            {
               bots[index].b_spray_logo = TRUE;  // this bot should spray logo now
               bots[index].f_spray_logo_time = gpGlobals->time;
            }
         }

         if (victim_edict != NULL)
         {
            BotChatTaunt(bots[index], victim_edict);
         }
      }

      // get the bot index of the victim...
      index = UTIL_GetBotIndex(victim_edict);

      // is this message about a bot being killed?
      if (index != -1)
      {
         if ((killer_index == 0) || (killer_index == victim_index))
         {
            // bot killed by world (worldspawn) or bot killed self...
            bots[index].killer_edict = NULL;
         }
         else
         {
            // store edict of player that killed this bot...
            bots[index].killer_edict = INDEXENT(killer_index);
         }
      }
   }
}


void BotClient_Valve_ScreenFade(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int duration;
   static int hold_time;
   //static int fade_flags;
   int length;

   if (state == 0)
   {
      state++;
      duration = *(int *)p;
   }
   else if (state == 1)
   {
      state++;
      hold_time = *(int *)p;
   }
   else if (state == 2)
   {
      state++;
      //fade_flags = *(int *)p;
   }
   else if (state == 6)
   {
      state = 0;

      length = (duration + hold_time) / 4096;
      bots[bot_index].blinded_time = gpGlobals->time + length - 2.0;
   }
   else
   {
      state++;
   }
}

