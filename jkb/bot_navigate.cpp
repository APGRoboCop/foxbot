//
// JK_Botti - be more human!
//
// bot_navigate.cpp
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
#include "bot_weapons.h"
#include "waypoint.h"
#include "bot_skill.h"
#include "bot_weapons.h"
#include "bot_sound.h"


extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;  // number of waypoints currently in use
extern qboolean is_team_play;
extern qboolean checked_teamplay;
extern bot_weapon_t weapon_defs[MAX_WEAPONS];

// SET THIS UP BASED ON MOD!!!
const int max_drop_height = 800;


// returns the number of degrees left to turn toward ideal pitch...
float BotChangePitch( bot_t &pBot, float speed )
{
   edict_t *pEdict = pBot.pEdict;
   float ideal;
   float current;
   float current_180;  // current +/- 180 degrees
   float diff;

   // turn from the current v_angle pitch to the idealpitch by selecting
   // the quickest way to turn to face that direction
   
   current = pEdict->v.v_angle.x;

   ideal = pEdict->v.idealpitch;

   // find the difference in the current and ideal angle
   diff = fabs(current - ideal);

   speed = speed * pBot.f_frame_time;  // angles per second

   // check if difference is less than the max degrees per turn
   if (diff < speed)
      speed = diff;  // just need to turn a little bit (less than max)

   // here we have four cases, both angle positive, one positive and
   // the other negative, one negative and the other positive, or
   // both negative.  handle each case separately...

   if ((current >= 0) && (ideal >= 0))  // both positive
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }
   else if ((current >= 0) && (ideal < 0))
   {
      current_180 = current - 180;

      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else if ((current < 0) && (ideal >= 0))
   {
      current_180 = current + 180;
      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else  // (current < 0) && (ideal < 0)  both negative
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }

   // check for wrap around of angle...
   if (current > 180)
      current -= 360;
   if (current < -180)
      current += 360;

   pEdict->v.v_angle.x = current;

   return diff;  // return number of degrees left to turn
}


// returns the number of degrees left to turn toward ideal yaw...
float BotChangeYaw( bot_t &pBot, float speed )
{
   edict_t *pEdict = pBot.pEdict;
   float ideal;
   float current;
   float current_180;  // current +/- 180 degrees
   float diff;

   // turn from the current v_angle yaw to the ideal_yaw by selecting
   // the quickest way to turn to face that direction

   current = pEdict->v.v_angle.y;

   ideal = pEdict->v.ideal_yaw;

   // find the difference in the current and ideal angle
   diff = fabs(current - ideal);

   // speed that we can turn during this frame...
   speed = speed * pBot.f_frame_time;

   // check if difference is less than the max degrees per turn
   if (diff < speed)
      speed = diff;  // just need to turn a little bit (less than max)

   // here we have four cases, both angle positive, one positive and
   // the other negative, one negative and the other positive, or
   // both negative.  handle each case separately...

   if ((current >= 0) && (ideal >= 0))  // both positive
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }
   else if ((current >= 0) && (ideal < 0))
   {
      current_180 = current - 180;

      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else if ((current < 0) && (ideal >= 0))
   {
      current_180 = current + 180;
      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else  // (current < 0) && (ideal < 0)  both negative
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }

   // check for wrap around of angle...
   if (current > 180)
      current -= 360;
   if (current < -180)
      current += 360;

   pEdict->v.v_angle.y = current;

   return diff;  // return number of degrees left to turn
}


static qboolean BotFindWaypoint( bot_t &pBot )
{
   int index, select_index;
   int path_index = -1;
   float distance, min_distance[3];
   int min_index[3];

   edict_t *pEdict = pBot.pEdict;
   
   for (index=0; index < 3; index++)
   {
      min_distance[index] = 99999.0;
      min_index[index] = -1;
   }

   index = WaypointFindPath(path_index, pBot.curr_waypoint_index);

   while (index != -1)
   {
      // if index is not a current or recent previous waypoint...
      if ((index != pBot.curr_waypoint_index) &&
          (index != pBot.prev_waypoint_index[0]) &&
          (index != pBot.prev_waypoint_index[1]) &&
          (index != pBot.prev_waypoint_index[2]) &&
          (index != pBot.prev_waypoint_index[3]) &&
          (index != pBot.prev_waypoint_index[4]))
      {
         // find the distance from the bot to this waypoint
         distance = (pEdict->v.origin - waypoints[index].origin).Length();

         if (distance < min_distance[0])
         {
            min_distance[2] = min_distance[1];
            min_index[2] = min_index[1];

            min_distance[1] = min_distance[0];
            min_index[1] = min_index[0];

            min_distance[0] = distance;
            min_index[0] = index;
         }
         else if (distance < min_distance [1])
         {
            min_distance[2] = min_distance[1];
            min_index[2] = min_index[1];

            min_distance[1] = distance;
            min_index[1] = index;
         }
         else if (distance < min_distance[2])
         {
            min_distance[2] = distance;
            min_index[2] = index;
         }
      }

      // find the next path to a waypoint
      index = WaypointFindPath(path_index, pBot.curr_waypoint_index);
   }

   select_index = -1;

   // about 20% of the time choose a waypoint at random
   // (don't do this any more often than every 10 seconds)

   if ((RANDOM_LONG2(1, 100) <= 20) &&
       (pBot.f_random_waypoint_time <= gpGlobals->time))
   {
      pBot.f_random_waypoint_time = gpGlobals->time + 10.0;

      if (min_index[2] != -1)
         index = RANDOM_LONG2(0, 2);
      else if (min_index[1] != -1)
         index = RANDOM_LONG2(0, 1);
      else if (min_index[0] != -1)
         index = 0;
      else
         return FALSE;  // no waypoints found!

      select_index = min_index[index];
   }
   else
   {
      // use the closest waypoint that has been recently used
      select_index = min_index[0];
   }

   if (select_index != -1)  // was a waypoint found?
   {
      pBot.prev_waypoint_index[4] = pBot.prev_waypoint_index[3];
      pBot.prev_waypoint_index[3] = pBot.prev_waypoint_index[2];
      pBot.prev_waypoint_index[2] = pBot.prev_waypoint_index[1];
      pBot.prev_waypoint_index[1] = pBot.prev_waypoint_index[0];
      pBot.prev_waypoint_index[0] = pBot.curr_waypoint_index;

      pBot.curr_waypoint_index = select_index;
      pBot.waypoint_origin = waypoints[select_index].origin;

      pBot.f_waypoint_time = gpGlobals->time;

      return TRUE;
   }

   return FALSE;  // couldn't find a waypoint
}


static void BotEvaluateGoal( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;

   // we're dying!  Forget about our goal
   if (pBot.waypoint_goal != -1 && pEdict->v.health <= 25 && pBot.wpt_goal_type != WPT_GOAL_HEALTH)
   {
      //if(pBot.waypoint_goal != -1)
      //   UTIL_ConsolePrintf("[%s] Dying, forget goal: %d -> %d", pBot.name, pBot.waypoint_goal, -1);
      
      pBot.f_waypoint_goal_time = 0;
      pBot.waypoint_goal = -1;
   }
}


//
static int BotGetSoundWaypoint( bot_t &pBot, edict_t *pTrackSoundEdict, edict_t ** pNewTrackSoundEdict )
{
   edict_t *pEdict = pBot.pEdict;
   
   int index = -1;
   int iSound;
   CSound *pCurrentSound;
   float mindistance;
   edict_t *pMinDistanceEdict = NULL;
   
   mindistance = WAYPOINT_UNREACHABLE;
      
   // walk through active sound linked list
   for(iSound = CSoundEnt::ActiveList(); iSound != SOUNDLIST_EMPTY; iSound = pCurrentSound->m_iNext)
   {
      pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );
      
      if(!pCurrentSound)
         continue;
      
      // ignore sounds created by bot itself
      if(pCurrentSound->m_iBotOwner == (&pBot - &bots[0]))
         continue;
      
      // we want specific waypoint?
      if(!FNullEnt(pTrackSoundEdict) && pCurrentSound->m_pEdict != pTrackSoundEdict)
         continue;
      
      // is sound too far away? (bot cannot hear)
      float s_distance = (pCurrentSound->m_vecOrigin - pEdict->v.origin).Length();
      if(s_distance > pCurrentSound->m_iVolume * skill_settings[pBot.bot_skill].hearing_sensitivity)
         continue;
      
      int temp_index = WaypointFindNearest(pCurrentSound->m_vecOrigin, pEdict, REACHABLE_RANGE, TRUE);
         
      // get distance
      if (temp_index > -1 && temp_index < num_waypoints)
      {
         float distance;
         
         // use alternative way in case bot doesn't have current waypoint
         if(pBot.curr_waypoint_index == -1)
            distance = (pEdict->v.origin - waypoints[temp_index].origin).Length();
         else
            distance = WaypointDistanceFromTo(pBot.curr_waypoint_index, temp_index);
            
         if (distance < mindistance)
         {
            mindistance = distance;
            index = temp_index;
            pMinDistanceEdict = FNullEnt(pCurrentSound->m_pEdict)?NULL:pCurrentSound->m_pEdict;
         }
      }
   }
   
   if(pNewTrackSoundEdict)
      *pNewTrackSoundEdict = pMinDistanceEdict;
   
   return index;
}


//
qboolean BotUpdateTrackSoundGoal( bot_t &pBot )
{
   if(pBot.wpt_goal_type != WPT_GOAL_TRACK_SOUND)
      return FALSE;
   
   // check track-sound-time if we should stop tracking
   // check if target has been erased
   // check our state -> do we want to keep tracking
   if((pBot.f_track_sound_time <= 0.0f && pBot.f_track_sound_time < gpGlobals->time) || 
        !pBot.b_has_enough_ammo_for_good_weapon || pBot.b_low_health)
   {
      if(pBot.waypoint_goal != -1)
      {
         //if(!pBot.b_has_enough_ammo_for_good_weapon || pBot.b_low_health)
         //   UTIL_ConsolePrintf("[%s] Dropped sound tracking goal because(%d -> %d): out of ammo/health", pBot.name, pBot.waypoint_goal, -1);
         //if(pBot.f_track_sound_time <= 0.0f && pBot.f_track_sound_time < gpGlobals->time)
         //   UTIL_ConsolePrintf("[%s] Dropped sound tracking goal because(%d -> %d): end of tracking time", pBot.name, pBot.waypoint_goal, -1);
      }
      
      pBot.waypoint_goal = -1;
      pBot.wpt_goal_type = WPT_GOAL_NONE;
      
      pBot.pTrackSoundEdict = NULL;
      pBot.f_track_sound_time = -1;
      
      return FALSE;
   }
   
   // get waypoint close to sound of track-sound-edict
   edict_t* pNew = NULL;
   int waypoint = BotGetSoundWaypoint( pBot, pBot.pTrackSoundEdict, &pNew );
   
   if(FNullEnt(pBot.pTrackSoundEdict) && !FNullEnt(pNew))
      pBot.pTrackSoundEdict = pNew;
   
   // update waypoint_goal
   //if(pBot.waypoint_goal != waypoint)
   //   UTIL_ConsolePrintf("[%s] Updated sound tracking goal waypoint: %d -> %d", pBot.name, pBot.waypoint_goal, waypoint);
   
   pBot.waypoint_goal = waypoint;
   
   return TRUE;
}


static void BotFindWaypointGoal( bot_t &pBot )
{
   int index = -1;
   
   edict_t *pEdict = pBot.pEdict;
   bot_weapon_select_t *pSelect = &weapon_select[0];

   if (pEdict->v.health * RANDOM_FLOAT2(0.9f, 1.0f/0.9f) < VALVE_MAX_NORMAL_HEALTH * 0.25f)
   {
      // look for health if we're pretty dead
      index = WaypointFindNearestGoal(pEdict, pBot.curr_waypoint_index, W_FL_HEALTH);
      
      if(index == -1)
         index = WaypointFindRandomGoal(pEdict, W_FL_HEALTH);

      if (index != -1)
      {
         pBot.wpt_goal_type = WPT_GOAL_HEALTH;
         pBot.waypoint_goal = index;
      }
      
      goto exit;
   }
   
   if (pBot.pBotEnemy == NULL)
   {
      int goal_type = 0;
      int ammoflags;
      int weaponflags;
      float mindistance = WAYPOINT_UNREACHABLE;
      edict_t* pTrackSoundEdict = NULL;
      
      // only if not engaging
      
      // find these if have good weapon and no low health
      if(!pBot.b_low_health && pBot.b_has_enough_ammo_for_good_weapon)
      {
         // find nearest interesting sound that isn't visible
         int temp_index = BotGetSoundWaypoint(pBot, NULL, &pTrackSoundEdict);
         
         if(temp_index != -1)
         {
            if(RANDOM_FLOAT2(0.0, 1.0) < 0.5f)
            {
               float track_time = RANDOM_FLOAT2(skill_settings[pBot.bot_skill].track_sound_time_min, skill_settings[pBot.bot_skill].track_sound_time_max);
               
               // don't try get other objects
               pBot.wpt_goal_type = WPT_GOAL_TRACK_SOUND;
               pBot.waypoint_goal = temp_index;
               pBot.pTrackSoundEdict = pTrackSoundEdict;
               pBot.f_track_sound_time = gpGlobals->time + track_time;
               index = temp_index;
               
               goto exit;
            }
            else
            {
               float distance = WaypointDistanceFromTo(pBot.curr_waypoint_index, temp_index);
               
               if (distance < mindistance)
               {
                  mindistance = distance;
                  index = temp_index;
                  goal_type = WPT_GOAL_TRACK_SOUND;
               }
            }
         }
         
         // want health?
         if (pEdict->v.health < VALVE_MAX_NORMAL_HEALTH * 0.95)
         {  
            // find nearest health
            int temp_index = WaypointFindNearestGoal(pEdict, pBot.curr_waypoint_index, W_FL_HEALTH, pBot.exclude_points);

            if(temp_index == -1)
               temp_index = WaypointFindRandomGoal(pEdict, W_FL_HEALTH, pBot.exclude_points);
         
            // get distance
            if (temp_index > -1 && temp_index < num_waypoints)
            {
               float distance = WaypointDistanceFromTo(pBot.curr_waypoint_index, temp_index);
               
               if (distance < mindistance)
               {
                  mindistance = distance;
                  index = temp_index;
                  goal_type = WPT_GOAL_HEALTH;
               }
            }
         }
         
         // want health?
         if (pEdict->v.armorvalue < VALVE_MAX_NORMAL_BATTERY * 0.95)
         {  
            // find nearest armor
            int temp_index = WaypointFindNearestGoal(pEdict, pBot.curr_waypoint_index, W_FL_ARMOR, pBot.exclude_points);

            if(temp_index == -1)
               temp_index = WaypointFindRandomGoal(pEdict, W_FL_ARMOR, pBot.exclude_points);
         
            // get distance
            if (temp_index > -1 && temp_index < num_waypoints)
            {
               float distance = WaypointDistanceFromTo(pBot.curr_waypoint_index, temp_index);
               
               if (distance < mindistance)
               {
                  mindistance = distance;
                  index = temp_index;
                  goal_type = WPT_GOAL_ARMOR;
               }
            }
         }
      }

      if (!pBot.b_longjump && skill_settings[pBot.bot_skill].can_longjump)
      {
         // find a longjump module
         int temp_index = WaypointFindNearestGoal(pEdict, pBot.curr_waypoint_index, W_FL_LONGJUMP, pBot.exclude_points);

         if(temp_index == -1)
            temp_index = WaypointFindRandomGoal(pEdict, W_FL_LONGJUMP, pBot.exclude_points);
         
         // get distance
         if (temp_index > -1 && temp_index < num_waypoints)
         {
            float distance = WaypointDistanceFromTo(pBot.curr_waypoint_index, temp_index);
            
            if (distance < mindistance)
            {
               mindistance = distance;
               index = temp_index;
               goal_type = WPT_GOAL_ITEM;
            }
         }
      }

      // find new weapon if only have shitty weapons or running out of ammo on all weapons 
      if (BotGetGoodWeaponCount(pBot, 1)==0 || !pBot.b_has_enough_ammo_for_good_weapon)
      {
         // find weapons that bot can use
         int select_index = -1;
         int weaponflags = 0;
         
         // collect item flags of desired weapons 
         while(pSelect[++select_index].iId) 
         {
            if(pSelect[select_index].avoid_this_gun)
               continue;
            
            if(!BotCanUseWeapon(pBot, pSelect[select_index]))
               continue;
               
            weaponflags |= pSelect[select_index].waypoint_flag;
         }
                  
         if(weaponflags)
         {
            int temp_index = WaypointFindNearestGoal(pEdict, pBot.curr_waypoint_index, W_FL_WEAPON, weaponflags, pBot.exclude_points);

            if(temp_index == -1)
               temp_index = WaypointFindRandomGoal(pEdict, W_FL_WEAPON, weaponflags, pBot.exclude_points);

            if (temp_index != -1)
            {
               float distance = WaypointDistanceFromTo(pBot.curr_waypoint_index, temp_index); 
                              
               if (distance < mindistance)
               {
                  index = temp_index;
                  mindistance = distance;
                  goal_type = WPT_GOAL_WEAPON;
               }
            }
         }
      }
      
      // get flags for ammo that are low, primarily ammo that can we can use with current weapons
      else if (((ammoflags = BotGetLowAmmoFlags(pBot, &(weaponflags=0), TRUE)) != 0 && weaponflags != 0) || 
      	       ((ammoflags = BotGetLowAmmoFlags(pBot, &(weaponflags=0), FALSE)) != 0 && weaponflags != 0))
      {
         int flags = (ammoflags ? W_FL_AMMO : 0) | (weaponflags ? W_FL_WEAPON : 0);
         
         // find ammo for that we don't have enough yet
         int temp_index = WaypointFindNearestGoal(pEdict, pBot.curr_waypoint_index, flags, ammoflags|weaponflags, pBot.exclude_points);
         
         if(temp_index == -1)
            temp_index = WaypointFindRandomGoal(pEdict, flags, ammoflags|weaponflags, pBot.exclude_points);
                  
         if (temp_index != -1)
         {
            float distance = WaypointDistanceFromTo(pBot.curr_waypoint_index, temp_index); 
                        
            if (distance < mindistance)
            {
               index = temp_index;
               mindistance = distance;
               goal_type = WPT_GOAL_AMMO;
            }
         }
      }
      
      if(index != -1)
      {
         pBot.wpt_goal_type = goal_type;
         pBot.waypoint_goal = index;
         
         if(goal_type != WPT_GOAL_TRACK_SOUND)
         {
            pBot.pTrackSoundEdict = NULL;
            pBot.f_track_sound_time = -1;
         }
         else
         {
            float track_time = RANDOM_FLOAT2(skill_settings[pBot.bot_skill].track_sound_time_min, skill_settings[pBot.bot_skill].track_sound_time_max);
            
            pBot.pTrackSoundEdict = pTrackSoundEdict;
            pBot.f_track_sound_time = gpGlobals->time + track_time;
         }
         
         goto exit;
      }
   }
   else if(!FNullEnt(pBot.pBotEnemy))
   {  
      // find a waypoint near our enemy
      index = WaypointFindNearest(pBot.pBotEnemy, 1024);

      if (index != -1)
      {
         pBot.wpt_goal_type = WPT_GOAL_ENEMY;
         pBot.waypoint_goal = index;
         pBot.pTrackSoundEdict = NULL;
         pBot.f_track_sound_time = -1;
         
         goto exit;
      }
   }

   // we couldn't find ANYTHING, go somewhere random
   if (index == -1)
   {
      index = WaypointFindRandomGoal(pEdict, 0);

      if (index != -1)
      {
         pBot.wpt_goal_type = WPT_GOAL_LOCATION;
         pBot.waypoint_goal = index;
         pBot.pTrackSoundEdict = NULL;
         pBot.f_track_sound_time = -1;
         
         goto exit;
      }
   }

exit:

   if (index != -1)
   {
#if 0
      switch (pBot.wpt_goal_type)
      {
         case WPT_GOAL_HEALTH:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I am going for some health!\n");
            break;
         case WPT_GOAL_ARMOR:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I am going for some armor!\n");
            break;
         case WPT_GOAL_WEAPON:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I am going for a weapon!\n");
            break;
         case WPT_GOAL_AMMO:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I am going for some ammo!\n");
            break;
         case WPT_GOAL_ITEM:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I am going for an item!\n");
            break;
         case WPT_GOAL_TRACK_SOUND:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I am tracking a sound!\n");
            break;
         case WPT_GOAL_ENEMY:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I am tracking/engaging an enemy!\n");
            break;
         case WPT_GOAL_LOCATION:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I have a random location goal!\n");
            break;
         default:
            UTIL_ConsolePrintf("[%s] %s", pBot.name, "I have an unknown goal!\n");
            break;
      }
#endif
   }
}


qboolean BotHeadTowardWaypoint( bot_t &pBot )
{
   int i;
   Vector v_src, v_dest;
   TraceResult tr;
   int index;
   qboolean status;
   float waypoint_distance, min_distance;
   qboolean touching;

   edict_t *pEdict = pBot.pEdict;


   // check if the bot has been trying to get to this waypoint for a while...
   if ((pBot.f_waypoint_time + 5.0) < gpGlobals->time)
   {
      pBot.curr_waypoint_index = -1;  // forget about this waypoint
      pBot.waypoint_goal = -1;  // also forget about a goal
   }

   // no goal, no goal time
   if ((pBot.waypoint_goal == -1) && (pBot.f_waypoint_goal_time > gpGlobals->time + 2) &&
       (pBot.f_waypoint_goal_time != 0.0))
      pBot.f_waypoint_goal_time = 0.0;

   // find new waypoint for sound we are tracking
   if(pBot.wpt_goal_type == WPT_GOAL_TRACK_SOUND)
   {
      BotUpdateTrackSoundGoal(pBot);
   }

   // check if we need to find a waypoint...
   if (pBot.curr_waypoint_index == -1)
   {
      pBot.waypoint_top_of_ladder = FALSE;

      // did we just come off of a ladder or are we underwater?
      if (((pBot.f_end_use_ladder_time + 2.0) > gpGlobals->time) || pBot.b_in_water)
      {
         // find the nearest visible waypoint
         i = WaypointFindNearest(pEdict, REACHABLE_RANGE);
      }
      else
      {
         // find the nearest reachable waypoint
         i = WaypointFindReachable(pEdict, REACHABLE_RANGE);
      }

      if (i == -1)
      {
         pBot.curr_waypoint_index = -1;
         return FALSE;
      }

      pBot.curr_waypoint_index = i;
      pBot.waypoint_origin = waypoints[i].origin;

      pBot.f_waypoint_time = gpGlobals->time;
   }
   else
   {
      if(pBot.b_longjump && skill_settings[pBot.bot_skill].can_longjump)
      {         
         Vector vecToWpt = waypoints[pBot.curr_waypoint_index].origin - pEdict->v.origin;
         float max_lj_distance = LONGJUMP_DISTANCE * (800 / CVAR_GET_FLOAT("sv_gravity"));
         
         // longjump toward the waypoint
         // we have to be out of water, on the ground, able to longjump, far enough away to longjump, and
         // facing pretty close to the waypoint
         if(!pBot.b_in_water && pBot.b_on_ground && !pBot.b_ducking && !pBot.b_on_ladder &&
            vecToWpt.Length() >= max_lj_distance * 0.6 && 
            pEdict->v.velocity.Length() > 50 && 
            DotProduct(UTIL_AnglesToForward(pEdict->v.v_angle), vecToWpt.Normalize()) > cos(deg2rad(10)))
         {
            // trace a hull toward the current waypoint the distance of a longjump (depending on gravity)
            UTIL_TraceHull(
               UTIL_GetOrigin(pEdict), 
               UTIL_GetOrigin(pEdict) + vecToWpt.Normalize() * max_lj_distance,
               dont_ignore_monsters, head_hull, pEdict, &tr
            );
            
            // make sure it's clear
            if (tr.flFraction > 0.999999f)
            {
               // trace another hull straight down so we can get ground level here
               UTIL_TraceHull(tr.vecEndPos, tr.vecEndPos - Vector(0,0,8192), dont_ignore_monsters, head_hull, pEdict, &tr);
               
               // make sure the point we found is about level with 
               if (waypoints[pBot.curr_waypoint_index].origin.z - tr.vecEndPos.z < 52)
               {
                  // actually do the longjump
                  pEdict->v.button |= IN_DUCK;
                  pBot.b_longjump_do_jump = TRUE;
                  
                  // recognize we'll be in the air for a second most likely
                  pBot.f_longjump_time = gpGlobals->time + 1.0;
                  
                  //UTIL_ConsolePrintf("%s doing longjump! -- wp\n", STRING(pEdict->v.netname));
               }
            }
         }
      }
      
      // skip this part if bot is trying to get out of water...
      if (pBot.f_exit_water_time < gpGlobals->time)
      {
         // check if we can still see our target waypoint...

         v_src = pEdict->v.origin + pEdict->v.view_ofs;
         v_dest = waypoints[pBot.curr_waypoint_index].origin;

         // trace a line from bot's eyes to destination...
         UTIL_TraceMove( v_src, v_dest, ignore_monsters, 
                         pEdict->v.pContainingEntity, &tr );

         // check if line of sight to object is blocked (i.e. not visible)
         if (tr.flFraction < 1.0f)
         {
            // did we just come off of a ladder or are we under water?
            if (((pBot.f_end_use_ladder_time + 2.0) > gpGlobals->time) || pBot.b_in_water)
            {
               // find the nearest visible waypoint
               i = WaypointFindNearest(pEdict, REACHABLE_RANGE);
            }
            else
            {
               // find the nearest reachable waypoint
               i = WaypointFindReachable(pEdict, REACHABLE_RANGE);
            }

            if (i == -1)
            {
               pBot.curr_waypoint_index = -1;
               return FALSE;
            }

            pBot.curr_waypoint_index = i;
            pBot.waypoint_origin = waypoints[i].origin;

            pBot.f_waypoint_time = gpGlobals->time;
         }
      }
   }

   // find the distance to the target waypoint
   waypoint_distance = (pEdict->v.origin - pBot.waypoint_origin).Length();

   // set the minimum distance from waypoint to be considered "touching" it
   min_distance = 50.0;

   // if this is a crouch waypoint, bot must be fairly close...
   if (waypoints[pBot.curr_waypoint_index].flags & W_FL_JUMP)
      min_distance = 25.0;
   
   if (waypoints[pBot.curr_waypoint_index].flags & W_FL_CROUCH)
      min_distance = 20.0;

   // if this is a ladder waypoint, bot must be fairly close to get on ladder
   if (waypoints[pBot.curr_waypoint_index].flags & W_FL_LADDER)
      min_distance = 20.0;

   // if this is a lift-start waypoint
   if (waypoints[pBot.curr_waypoint_index].flags & W_FL_LIFT_START)
      min_distance = 32.0;
   
   // if this is a lift-end waypoint, bot must be very close
   if (waypoints[pBot.curr_waypoint_index].flags & W_FL_LIFT_END)
      min_distance = 20.0;

   // if item waypoint, go close
   if (waypoints[pBot.curr_waypoint_index].flags & (W_FL_LONGJUMP | W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
      min_distance = 20.0;

   // if trying to get out of water, need to get very close to waypoint...
   if (pBot.f_exit_water_time >= gpGlobals->time)
      min_distance = 20.0;

   touching = FALSE;

   // did the bot run past the waypoint? (prevent the loop-the-loop problem)
   if ((pBot.prev_waypoint_distance > 1.0f) &&
       (waypoint_distance > pBot.prev_waypoint_distance))
      touching = TRUE;

   // are we close enough to a target waypoint...
   if (waypoint_distance <= min_distance)
      touching = TRUE;

   // save current distance as previous
   pBot.prev_waypoint_distance = waypoint_distance;

   if (touching)
   {
      qboolean waypoint_found = FALSE;

      pBot.prev_waypoint_distance = 0.0;

      // reeval our goal
      BotEvaluateGoal( pBot );

      // check if the waypoint is a door waypoint
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_DOOR)
      {
         pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
      }

      // check if the next waypoint is a jump waypoint...
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_JUMP)
      {
         pEdict->v.button |= IN_JUMP;  // jump here
      }

      // check if the bot has reached the goal waypoint...
      if (pBot.curr_waypoint_index == pBot.waypoint_goal)
      {
         // if this waypoint has an item, make sure we get it
         if (waypoints[pBot.waypoint_goal].flags & (W_FL_LONGJUMP| W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON))
         {
            pBot.pBotPickupItem = WaypointFindItem(pBot.waypoint_goal);
            pBot.f_find_item = gpGlobals->time + 0.2;
            pBot.f_last_item_found = gpGlobals->time;
         }

         if (pBot.pBotEnemy != NULL)
            pBot.f_waypoint_goal_time = gpGlobals->time + 0.25;
         else   // a little delay time, since we'll touch the waypoint before we actually get what it has
            pBot.f_waypoint_goal_time = gpGlobals->time + 0.25;

         // don't pick same object too often
         if(pBot.wpt_goal_type == WPT_GOAL_HEALTH || 
            pBot.wpt_goal_type == WPT_GOAL_ARMOR || 
            pBot.wpt_goal_type == WPT_GOAL_WEAPON || 
            pBot.wpt_goal_type == WPT_GOAL_AMMO || 
            pBot.wpt_goal_type == WPT_GOAL_ITEM)
         {
            for(int i = EXCLUDE_POINTS_COUNT - 1; i > 0; i--)
               pBot.exclude_points[i] = pBot.exclude_points[i-1];
            pBot.exclude_points[0] = pBot.waypoint_goal;
         }

         //UTIL_ConsolePrintf("[%s] Reach goal: %d -> %d", pBot.name, pBot.waypoint_goal, -1);
         
         pBot.waypoint_goal = -1;  // forget this goal waypoint
         pBot.wpt_goal_type = WPT_GOAL_NONE;
      }

      // test special case of bot underwater and close to surface...
      if (pBot.b_in_water)
      {
         Vector v_src, v_dest;
         TraceResult tr;
         int contents;

         // trace a line straight up 100 units...
         v_src = pEdict->v.origin;
         v_dest = v_src;
         v_dest.z = v_dest.z + 100.0;

         // trace a line to destination...
         UTIL_TraceMove( v_src, v_dest, ignore_monsters, 
                         pEdict->v.pContainingEntity, &tr );

         if (tr.flFraction > 0.999999f)
         {
            // find out what the contents is of the end of the trace...
            contents = POINT_CONTENTS( tr.vecEndPos );
   
            // check if the trace endpoint is in open space...
            if (contents == CONTENTS_EMPTY)
            {
               // find the nearest visible waypoint
               i = WaypointFindNearest(tr.vecEndPos, pEdict, 100);

               if (i != -1)
               {
                  waypoint_found = TRUE;
                  pBot.curr_waypoint_index = i;
                  pBot.waypoint_origin = waypoints[i].origin;

                  pBot.f_waypoint_time = gpGlobals->time;

                  // keep trying to exit water for next 3 seconds
                  pBot.f_exit_water_time = gpGlobals->time + 3.0;
               }
            }
         }
      }
      
      // if the bot doesn't have a goal waypoint then pick one...
      if ((pBot.waypoint_goal == -1 || pBot.pBotEnemy != NULL) &&
          (pBot.f_waypoint_goal_time < gpGlobals->time))
      {
         // tracking something, pick goal much more often
         if (pBot.pBotEnemy != NULL)
            pBot.f_waypoint_goal_time = gpGlobals->time + 0.5;
         else // don't pick a goal more often than every 120 seconds...
            pBot.f_waypoint_goal_time = gpGlobals->time + 120.0;
      
         BotFindWaypointGoal(pBot);
      }
      
      // check if the bot has a goal waypoint...
      if (pBot.waypoint_goal != -1)
      {
         // get the next waypoint to reach goal...
         i = WaypointRouteFromTo(pBot.curr_waypoint_index, pBot.waypoint_goal);

         if (i != WAYPOINT_UNREACHABLE)  // can we get to the goal from here?
         {
            waypoint_found = TRUE;
            pBot.curr_waypoint_index = i;
            pBot.waypoint_origin = waypoints[i].origin;

            pBot.f_waypoint_time = gpGlobals->time;
         }
      }

      if (waypoint_found == FALSE)
      {
         index = 4;

         // try to find the next waypoint
         while (((status = BotFindWaypoint( pBot )) == FALSE) &&
                (index > 0))
         {
            // if waypoint not found, clear oldest prevous index and try again

            pBot.prev_waypoint_index[index] = -1;
            index--;
         }

         if (status == FALSE)
         {
            pBot.curr_waypoint_index = -1;  // indicate no waypoint found

            // clear all previous waypoints...
            for (index=0; index < 5; index++)
               pBot.prev_waypoint_index[index] = -1;

            return FALSE;
         }
      }
   }

   // check if the next waypoint is on a ladder AND
   // the bot it not currently on a ladder...
   if ((waypoints[pBot.curr_waypoint_index].flags & W_FL_LADDER) && !pBot.b_on_ladder)
   {
      // if it's origin is lower than the bot...
      if (waypoints[pBot.curr_waypoint_index].origin.z < pEdict->v.origin.z)
      {
         pBot.waypoint_top_of_ladder = TRUE;
      }
   }
   else
   {
      pBot.waypoint_top_of_ladder = FALSE;
   }

   // keep turning towards the waypoint...

   Vector v_direction = pBot.waypoint_origin - pEdict->v.origin;

   Vector v_angles = UTIL_VecToAngles(v_direction);

   // if the bot is NOT on a ladder, change the yaw...
   if (!pBot.b_on_ladder)
   {
      pEdict->v.idealpitch = -v_angles.x;
      BotFixIdealPitch(pEdict);

      pEdict->v.ideal_yaw = v_angles.y;
      BotFixIdealYaw(pEdict);
   }

   return TRUE;
}


void BotOnLadder( bot_t &pBot, float moved_distance )
{
   static float search_angles[] = { 0.0f, 0.0f, 90.0f, -90.0f, 45.0f, -45.0f, 60.0f, -30.0f, 30.0f, -60.0f, 10.0f, -10.0f, 20.0f, -20.0f, 40.0f, -40.0f, 50.0f, -50.0f, 70.0f, -70.0f, 80.0f, -80.0f, 9999.9f };
   Vector v_src, v_dest, view_angles;
   TraceResult tr;
   float angle, search_dir;
   qboolean done = FALSE;
   int i;

   edict_t *pEdict = pBot.pEdict;

   // check if the bot has JUST touched this ladder...
   if (pBot.ladder_dir == LADDER_UNKNOWN)
   {
      i = 0;
      search_dir = RANDOM_LONG2(0, 1) ? 1.0f : -1.0f;      
      search_angles[0] = 0.0f;
      
      // add special case to angles, check towards next waypoint
      if(pBot.curr_waypoint_index != -1)
      {
         Vector v_dir = waypoints[pBot.curr_waypoint_index].origin - pEdict->v.origin;
         Vector v_to_wp = UTIL_VecToAngles(v_dir);
         
         search_angles[0] = v_to_wp.y * search_dir;
      }
      
      if(search_angles[0] == 0.0f)
         i++;
      
      // try to square up the bot on the ladder...
      while ((!done) && (search_angles[i] <= 90.0f))
      {
         angle = search_angles[i] * search_dir;
         
         // try looking in one direction (forward + angle)
         view_angles.x = pEdict->v.v_angle.x;
         view_angles.y = UTIL_WrapAngle(pEdict->v.v_angle.y + angle);
         view_angles.z = 0;
         
         v_src = pEdict->v.origin + pEdict->v.view_ofs;
         v_dest = v_src + UTIL_AnglesToForward(view_angles) * 30;

         UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters, 
                         pEdict->v.pContainingEntity, &tr);

         if (tr.flFraction < 1.0f)  // hit something?
         {
            //if (FIsClassname("func_wall", tr.pHit))
            if (!FNullEnt(tr.pHit) && tr.pHit->v.solid == SOLID_BSP)
            {
               // square up to the wall...
               view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

               // Normal comes OUT from wall, so flip it around...
               view_angles.y += 180;

               if (view_angles.y > 180)
                  view_angles.y -= 360;

               pEdict->v.ideal_yaw = view_angles.y;

               BotFixIdealYaw(pEdict);

               done = TRUE;
            }
         }
         else
         {
            // try looking in the other direction (forward - angle)
            view_angles.x = pEdict->v.v_angle.x;
            view_angles.y = UTIL_WrapAngle(pEdict->v.v_angle.y - angle);
            view_angles.z = 0;

            v_src = pEdict->v.origin + pEdict->v.view_ofs;
            v_dest = v_src + UTIL_AnglesToForward(view_angles) * 30;

            UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr);

            if (tr.flFraction < 1.0f)  // hit something?
            {
               //if (FIsClassname("func_wall", tr.pHit))
               if (!FNullEnt(tr.pHit) && tr.pHit->v.solid == SOLID_BSP)
               {
                  // square up to the wall...
                  view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

                  // Normal comes OUT from wall, so flip it around...
                  view_angles.y += 180;

                  if (view_angles.y > 180)
                     view_angles.y -= 360;

                  pEdict->v.ideal_yaw = view_angles.y;

                  BotFixIdealYaw(pEdict);

                  done = TRUE;
               }
            }
         }

         i++;
      }

      if (!done)  // if didn't find a wall, just reset ideal_yaw...
      {
         // set ideal_yaw to current yaw (so bot won't keep turning)
         pEdict->v.ideal_yaw = pEdict->v.v_angle.y;

         BotFixIdealYaw(pEdict);
      }
   }

   // moves the bot up or down a ladder.  if the bot can't move
   // (i.e. get's stuck with someone else on ladder), the bot will
   // change directions and go the other way on the ladder.

   if (pBot.ladder_dir == LADDER_UP)  // is the bot currently going up?
   {
      pEdict->v.v_angle.x = -60;  // look upwards

      // check if the bot hasn't moved much since the last location...
      if ((moved_distance <= 1) && (pBot.f_prev_speed >= 1.0))
      {
         // the bot must be stuck, change directions...

         pEdict->v.v_angle.x = 60;  // look downwards
         pBot.ladder_dir = LADDER_DOWN;
      }
   }
   else if (pBot.ladder_dir == LADDER_DOWN)  // is the bot currently going down?
   {
      pEdict->v.v_angle.x = 60;  // look downwards

      // check if the bot hasn't moved much since the last location...
      if ((moved_distance <= 1) && (pBot.f_prev_speed >= 1.0))
      {
         // the bot must be stuck, change directions...

         pEdict->v.v_angle.x = -60;  // look upwards
         pBot.ladder_dir = LADDER_UP;
      }
   }
   else  // the bot hasn't picked a direction yet, try going up...
   {
      pEdict->v.v_angle.x = -60;  // look upwards
      pBot.ladder_dir = LADDER_UP;
   }

   // move forward (i.e. in the direction the bot is looking, up or down)
   pEdict->v.button |= IN_FORWARD;
}


void BotUnderWater( bot_t &pBot )
{
   qboolean found_waypoint = FALSE;

   edict_t *pEdict = pBot.pEdict;

   // are there waypoints in this level (and not trying to exit water)?
   if (num_waypoints > 0 && pBot.f_exit_water_time < gpGlobals->time)
   {
      // head towards a waypoint
      found_waypoint = BotHeadTowardWaypoint(pBot);
   }

   if (found_waypoint == FALSE)
   {
      // handle movements under water.  right now, just try to keep from
      // drowning by swimming up towards the surface and look to see if
      // there is a surface the bot can jump up onto to get out of the
      // water.  bots DON'T like water!

      Vector v_src, v_forward;
      TraceResult tr;
      int contents;
   
      // swim up towards the surface
      pEdict->v.v_angle.x = -60;  // look upwards
   
      // move forward (i.e. in the direction the bot is looking, up or down)
      pEdict->v.button |= IN_FORWARD;
   
      // look from eye position straight forward (remember: the bot is looking
      // upwards at a 60 degree angle so TraceLine will go out and up...
   
      v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
      v_forward = v_src + UTIL_AnglesToForward(pEdict->v.v_angle) * 90;
   
      // trace from the bot's eyes straight forward...
      UTIL_TraceMove( v_src, v_forward, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr);
   
      // check if the trace didn't hit anything (i.e. nothing in the way)...
      if (tr.flFraction > 0.999999f)
      {
         // find out what the contents is of the end of the trace...
         contents = POINT_CONTENTS( tr.vecEndPos );
   
         // check if the trace endpoint is in open space...
         if (contents == CONTENTS_EMPTY)
         {
            // ok so far, we are at the surface of the water, continue...
   
            v_src = tr.vecEndPos;
            v_forward = v_src;
            v_forward.z -= 90;
   
            // trace from the previous end point straight down...
            UTIL_TraceMove( v_src, v_forward, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr);
   
            // check if the trace hit something...
            if (tr.flFraction < 1.0f)
            {
               contents = POINT_CONTENTS( tr.vecEndPos );
   
               // if contents isn't water then assume it's land, jump!
               if (contents != CONTENTS_WATER)
               {
                  pEdict->v.button |= IN_JUMP;
               }
            }
         }
      }
   }
}


void BotUseLift( bot_t &pBot, float moved_distance )
{
   edict_t *pEdict = pBot.pEdict;

   // just need to press the button once, when the flag gets set...
   if (pBot.f_use_button_time + 0.0 >= gpGlobals->time)
   {     
      // aim at the button..
      Vector v_target = pBot.v_use_target - GetGunPosition( pEdict );
      Vector target_angle = UTIL_VecToAngles(v_target);

      pEdict->v.idealpitch = UTIL_WrapAngle(target_angle.x);
      pEdict->v.ideal_yaw = UTIL_WrapAngle(target_angle.y);
      
      pBot.b_not_maxspeed = TRUE;
      pBot.f_move_speed = 120.0f;
      
      // block strafing before using hev
      pBot.f_strafe_time = gpGlobals->time + 1.0f;
      pBot.f_strafe_direction = 0.0f;

      BotFixIdealYaw(pEdict);
      BotFixIdealPitch(pEdict);
      
      if (pBot.f_use_button_time + 0.40 >= gpGlobals->time)
      {
         pEdict->v.button |= IN_USE;
      
         // find near by W_FL_LIFT_START and make it current waypoint
         int index = WaypointFindNearestGoal(pEdict->v.origin, pEdict, 150.0f, W_FL_LIFT_START);
      
         if(index != -1)
         {
            pBot.prev_waypoint_index[4] = pBot.prev_waypoint_index[3];
            pBot.prev_waypoint_index[3] = pBot.prev_waypoint_index[2];
            pBot.prev_waypoint_index[2] = pBot.prev_waypoint_index[1];
            pBot.prev_waypoint_index[1] = pBot.prev_waypoint_index[0];
            pBot.prev_waypoint_index[0] = pBot.curr_waypoint_index;
         
            pBot.curr_waypoint_index = index;
         }
      }
   }

   // check if the bot has waited too long for the lift to move...
   else if (((pBot.f_use_button_time + 2.0) < gpGlobals->time) &&
       (!pBot.b_lift_moving))
   {
      // clear use button flag
      pBot.b_use_button = FALSE;

      // bot doesn't have to set f_find_item since the bot
      // should already be facing away from the button
      
      pBot.b_not_maxspeed = FALSE;
      pBot.f_move_speed = pBot.f_max_speed;
      pBot.f_strafe_time = gpGlobals->time;
      pBot.f_move_speed = pBot.f_max_speed;
   }

   // check if lift has started moving...
   if ((moved_distance > 1) && (!pBot.b_lift_moving))
   {
      pBot.b_lift_moving = TRUE;
   }

   // check if lift has stopped moving...
   if ((moved_distance <= 1) && (pBot.b_lift_moving))
   {
      TraceResult tr1, tr2;
      Vector v_src, v_forward, v_right, v_left;
      Vector v_down, v_forward_down, v_right_down, v_left_down;
      Vector angle_v_forward, angle_v_right, angle_v_left;
      
      pBot.b_use_button = FALSE;
      
      pBot.b_not_maxspeed = FALSE;
      pBot.f_move_speed = pBot.f_max_speed;
      pBot.f_strafe_time = gpGlobals->time;
      pBot.f_move_speed = pBot.f_max_speed;

      // TraceLines in 4 directions to find which way to go...

      UTIL_MakeVectorsPrivate( pEdict->v.v_angle, angle_v_forward, angle_v_right, angle_v_left );

      v_src = pEdict->v.origin + pEdict->v.view_ofs;
      v_forward = v_src + angle_v_forward * 90;
      v_right = v_src + angle_v_right * 90;
      v_left = v_src + angle_v_right * -90;

      v_down = pEdict->v.v_angle;
      v_down.x = v_down.x + 45;  // look down at 45 degree angle

      UTIL_MakeVectorsPrivate( v_down, angle_v_forward, angle_v_right, angle_v_left );

      v_forward_down = v_src + angle_v_forward * 100;
      v_right_down = v_src + angle_v_right * 100;
      v_left_down = v_src + angle_v_right * -100;

      // try tracing forward first...
      UTIL_TraceMove( v_src, v_forward, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr1);
      UTIL_TraceMove( v_src, v_forward_down, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr2);

      // check if we hit a wall or didn't find a floor...
      if ((tr1.flFraction < 1.0f) || (tr2.flFraction >= 1.0f))
      {
         // try tracing to the RIGHT side next...
         UTIL_TraceMove( v_src, v_right, dont_ignore_monsters, 
                         pEdict->v.pContainingEntity, &tr1);
         UTIL_TraceMove( v_src, v_right_down, dont_ignore_monsters, 
                         pEdict->v.pContainingEntity, &tr2);

         // check if we hit a wall or didn't find a floor...
         if ((tr1.flFraction < 1.0f) || (tr2.flFraction >= 1.0f))
         {
            // try tracing to the LEFT side next...
            UTIL_TraceMove( v_src, v_left, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr1);
            UTIL_TraceMove( v_src, v_left_down, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr2);

            // check if we hit a wall or didn't find a floor...
            if ((tr1.flFraction < 1.0f) || (tr2.flFraction >= 1.0f))
            {
               // only thing to do is turn around...
               pEdict->v.ideal_yaw += 180;  // turn all the way around
            }
            else
            {
               pEdict->v.ideal_yaw += 90;  // turn to the LEFT
            }
         }
         else
         {
            pEdict->v.ideal_yaw -= 90;  // turn to the RIGHT
         }

         BotFixIdealYaw(pEdict);
      }

      BotChangeYaw( pBot, pEdict->v.yaw_speed );

      pBot.f_move_speed = pBot.f_max_speed;
   }
}


qboolean BotStuckInCorner( bot_t &pBot )
{
   TraceResult tr;
   Vector v_src, v_dest;
   edict_t *pEdict = pBot.pEdict;
   const float offsets[1] = {0};
   int right_first = RANDOM_LONG2(0,1);
   Vector v_forward, v_right, v_up;
   
   UTIL_MakeVectorsPrivate(pEdict->v.v_angle, v_forward, v_right, v_up);

   for(int i = 0; i < 1; i++) {
     // trace 45 degrees to the right...
     v_src = pEdict->v.origin;
     v_src.z += offsets[i];
     v_dest = v_src + v_forward*20 + (v_right*20) * (right_first?1:-1);
     
     UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,  pEdict->v.pContainingEntity, &tr);
     
     if (tr.flFraction > 0.999999f)
        return FALSE;
     
     // trace 45 degrees to the left...
     v_dest = v_src + v_forward*20 - (v_right*20) * (right_first?1:-1);
     
     UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,  pEdict->v.pContainingEntity, &tr);
     
     if (tr.flFraction > 0.999999f)
        return FALSE;
   }

   return TRUE;  // bot is in a corner
}


void BotTurnAtWall( bot_t &pBot, TraceResult *tr, qboolean negative )
{
   edict_t *pEdict = pBot.pEdict;
   Vector Normal;
   float Y, Y1, Y2, D1, D2, Z;

   // Find the normal vector from the trace result.  The normal vector will
   // be a vector that is perpendicular to the surface from the TraceResult.

   Normal = UTIL_VecToAngles(tr->vecPlaneNormal);

   // Since the bot keeps it's view angle in -180 < x < 180 degrees format,
   // and since TraceResults are 0 < x < 360, we convert the bot's view
   // angle (yaw) to the same format at TraceResult.

   Y = pEdict->v.v_angle.y;
   Y = Y + 180;
   if (Y > 359) Y -= 360;

   if (negative)
   {
      // Turn the normal vector around 180 degrees (i.e. make it point towards
      // the wall not away from it.  That makes finding the angles that the
      // bot needs to turn a little easier.

      Normal.y = Normal.y - 180;
   }

   if (Normal.y < 0)
      Normal.y += 360;

   // Here we compare the bots view angle (Y) to the Normal - 90 degrees (Y1)
   // and the Normal + 90 degrees (Y2).  These two angles (Y1 & Y2) represent
   // angles that are parallel to the wall surface, but heading in opposite
   // directions.  We want the bot to choose the one that will require the
   // least amount of turning (saves time) and have the bot head off in that
   // direction.

   Y1 = Normal.y - 90;
   if (RANDOM_LONG2(1, 100) <= 50)
   {
      Y1 = Y1 - RANDOM_FLOAT2(5.0, 20.0);
   }
   if (Y1 < 0) Y1 += 360;

   Y2 = Normal.y + 90;
   if (RANDOM_LONG2(1, 100) <= 50)
   {
      Y2 = Y2 + RANDOM_FLOAT2(5.0, 20.0);
   }
   if (Y2 > 359) Y2 -= 360;

   // D1 and D2 are the difference (in degrees) between the bot's current
   // angle and Y1 or Y2 (respectively).

   D1 = fabs(Y - Y1);
   if (D1 > 179) D1 = fabs(D1 - 360);
   D2 = fabs(Y - Y2);
   if (D2 > 179) D2 = fabs(D2 - 360);

   // If difference 1 (D1) is more than difference 2 (D2) then the bot will
   // have to turn LESS if it heads in direction Y1 otherwise, head in
   // direction Y2.  I know this seems backwards, but try some sample angles
   // out on some graph paper and go through these equations using a
   // calculator, you'll see what I mean.

   if (D1 > D2)
      Z = Y1;
   else
      Z = Y2;

   // convert from TraceResult 0 to 360 degree format back to bot's
   // -180 to 180 degree format.

   if (Z > 180)
      Z -= 360;

   // set the direction to head off into...
   pEdict->v.ideal_yaw = Z;

   BotFixIdealYaw(pEdict);
}


qboolean BotCantMoveForward( bot_t &pBot, TraceResult *tr )
{
   edict_t *pEdict = pBot.pEdict;

   // use some TraceLines to determine if anything is blocking the current
   // path of the bot.

   Vector v_src, v_forward, v_angle_forward;

   v_angle_forward = UTIL_AnglesToForward( pEdict->v.v_angle );

   // first do a trace from the bot's eyes forward...
   // bot's head is clear, check at waist level...
   const Vector offsets[2] = {pEdict->v.view_ofs, Vector(0,0,0)};

   for(int i = 0; i < 2; i++) {
      v_src = pEdict->v.origin + offsets[i];  // EyePosition()
      v_forward = v_src + v_angle_forward * 40;

      UTIL_TraceMove( v_src, v_forward, dont_ignore_monsters,  pEdict->v.pContainingEntity, tr);

      // check if the trace hit something...
      if (tr->flFraction < 1.0f)
      {
         return TRUE;  // bot's head will hit something
      }
   }
   
   return FALSE;  // bot can move forward, return false
}


qboolean BotCanJumpUp( bot_t &pBot, qboolean *bDuckJump)
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the highest normal jumping distance.  I trace once at the center of
   // the body, once at the right side, and once at the left side.  If all
   // three of these TraceLines don't hit an obstruction then I know the
   // area to jump to is clear.  I then need to trace from head level,
   // above where the bot will jump to, downward to see if there is anything
   // blocking the jump.  There could be a narrow opening that the body
   // will not fit into.  These horizontal and vertical TraceLines seem
   // to catch most of the problems with falsely trying to jump on something
   // that the bot can not get onto.

   TraceResult tr;
   qboolean check_duck = FALSE;
   Vector v_jump, v_source, v_dest, v_forward, v_right, v_up;
   edict_t *pEdict = pBot.pEdict;

   *bDuckJump = FALSE;

   // convert current view angle to vectors for TraceLine math...

   v_jump = pEdict->v.v_angle;
   v_jump.x = 0;  // reset pitch to 0 (level horizontally)
   v_jump.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectorsPrivate( v_jump, v_forward, v_right, v_up );

   // use center of the body first...

   // maximum normal jump height is 45, so check one unit above that (46)
   v_source = pEdict->v.origin + Vector(0, 0, -36 + 46);
   v_dest = v_source + v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, check duck jump...
   if (tr.flFraction < 1.0f)
      check_duck = TRUE;

   if (!check_duck)
   {
      // now check same height to one side of the bot...
      v_source = pEdict->v.origin + v_right * 16 + Vector(0, 0, -36 + 46);
      v_dest = v_source + v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, check duck jump...
      if (tr.flFraction < 1.0f)
         check_duck = TRUE;
   }

   if (!check_duck)
   {
      // now check same height on the other side of the bot...
      v_source = pEdict->v.origin + v_right * -16 + Vector(0, 0, -36 + 46);
      v_dest = v_source + v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, check duck jump...
      if (tr.flFraction < 1.0f)
         check_duck = TRUE;
   }

   if (check_duck)
   {
      // maximum crouch jump height is 63, so check one unit above that (64)
      v_source = pEdict->v.origin + Vector(0, 0, -36 + 64);
      v_dest = v_source + v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, return FALSE
      if (tr.flFraction < 1.0f)
         return FALSE;

      // now check same height on the other side of the bot...
      v_source = pEdict->v.origin + v_right * -16 + Vector(0, 0, -36 + 64);
      v_dest = v_source + v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, return FALSE
      if (tr.flFraction < 1.0f)
         return FALSE;

      // now check same height on the other side of the bot...
      v_source = pEdict->v.origin + v_right * -16 + Vector(0, 0, -36 + 64);
      v_dest = v_source + v_forward * 24;

      // trace a line forward at maximum jump height...
      UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                      pEdict->v.pContainingEntity, &tr);

      // if trace hit something, return FALSE
      if (tr.flFraction < 1.0f)
         return FALSE;
   }

   // now trace from head level downward to check for obstructions...

   // start of trace is 24 units in front of bot...
   v_source = pEdict->v.origin + v_forward * 24;

   if (check_duck)
      // offset 36 units if crouch-jump (36 + 36)
      v_source.z = v_source.z + 72;
   else
      // offset 72 units from top of head (72 + 36)
      v_source.z = v_source.z + 108;


   if (check_duck)
      // end point of trace is 27 units straight down from start...
      // (27 is 72 minus the jump limit height which is 63 - 18 = 45)
      v_dest = v_source + Vector(0, 0, -27);
   else
      // end point of trace is 99 units straight down from start...
      // (99 is 108 minus the jump limit height which is 45 - 36 = 9)
      v_dest = v_source + Vector(0, 0, -99);


   // trace a line straight down toward the ground...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0f)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + v_right * 16 + v_forward * 24;

   if (check_duck)
      v_source.z = v_source.z + 72;
   else
      v_source.z = v_source.z + 108;

   if (check_duck)
      v_dest = v_source + Vector(0, 0, -27);
   else
      v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0f)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + v_right * -16 + v_forward * 24;

   if (check_duck)
      v_source.z = v_source.z + 72;
   else
      v_source.z = v_source.z + 108;

   if (check_duck)
      v_dest = v_source + Vector(0, 0, -27);
   else
      v_dest = v_source + Vector(0, 0, -99);

   // trace a line straight down toward the ground...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0f)
      return FALSE;

   return TRUE;
}


qboolean BotCanDuckUnder( bot_t &pBot )
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the ducking height.  I trace once at the center of the body, once
   // at the right side, and once at the left side.  If all three of these
   // TraceLines don't hit an obstruction then I know the area to duck to
   // is clear.  I then need to trace from the ground up, 72 units, to make
   // sure that there is something blocking the TraceLine.  Then we know
   // we can duck under it.

   TraceResult tr;
   Vector v_duck, v_source, v_dest, v_forward, v_right, v_up;
   edict_t *pEdict = pBot.pEdict;

   // convert current view angle to vectors for TraceLine math...

   v_duck = pEdict->v.v_angle;
   v_duck.x = 0;  // reset pitch to 0 (level horizontally)
   v_duck.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectorsPrivate( v_duck, v_forward, v_right, v_up );

   // use center of the body first...

   // duck height is 36, so check one unit above that (37)
   v_source = pEdict->v.origin + Vector(0, 0, -36 + 37);
   v_dest = v_source + v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0f)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + v_right * 16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0f)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + v_right * -16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0f)
      return FALSE;

   // now trace from the ground up to check for object to duck under...

   // start of trace is 24 units in front of bot near ground...
   v_source = pEdict->v.origin + v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up

   // end point of trace is 72 units straight up from start...
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction > 0.999999f)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + v_right * 16 + v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction > 0.999999f)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + v_right * -16 + v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceMove( v_source, v_dest, dont_ignore_monsters, 
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction > 0.999999f)
      return FALSE;

   return TRUE;
}


qboolean BotEdgeForward( bot_t &pBot, const Vector &v_move_dir )
{
   edict_t *pEdict = pBot.pEdict;
   
   // pre checks
   if (FBitSet(pEdict->v.button, IN_DUCK) || FBitSet(pEdict->v.button, IN_JUMP) ||
       !pBot.b_on_ground || pBot.b_on_ladder)
      return FALSE;
   
   // 1. Trace from origin 32 units forward
   //     then
   //      Trace for edge, 91 units down
   //       if hit ground -> no edge

   TraceResult tr;
   Vector v_source, v_dest, v_forward, v_ground;

   // convert current view angle to vectors for TraceLine math...

   v_forward = v_move_dir;
   v_forward.z = 0;
   if(v_forward.Length() <= 1)
      return(FALSE);
   
   v_forward = v_forward.Normalize();
   
   // trace down to ground 
   v_source = pEdict->v.origin;
   v_dest = v_source - Vector(0, 0, 72/2);
   
   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
   
   v_ground = tr.vecEndPos;
   v_ground.z += 1.0f;
      
   // 32 units forward
   v_source = v_ground;
   v_dest = v_source + v_forward * 32;

   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

   // if trace hit something -> no edge
   if (tr.flFraction < 1.0f)
      return(FALSE);
   
   // 45+1+1 units down from 32 units forward
   v_source = v_ground + v_forward * 32;
   v_dest = v_source - Vector(0, 0, 45+1+1);

   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

   // if trace hit something -> no edge
   if (tr.flFraction < 1.0f)
      return(FALSE);

   // 
   return(TRUE);
}


qboolean BotEdgeRight( bot_t &pBot, const Vector &v_move_dir )
{
   edict_t *pEdict = pBot.pEdict;
   
   // pre checks
   if (FBitSet(pEdict->v.button, IN_DUCK) || FBitSet(pEdict->v.button, IN_JUMP) ||
       !pBot.b_on_ground || pBot.b_on_ladder)
      return FALSE;

   // 1. Trace from origin 32 units right
   //     then
   //      Trace for edge, 91 units down
   //       if hit ground -> no edge

   TraceResult tr;
   Vector v_source, v_dest, v_forward, v_right, v_ground;

   // convert current view angle to vectors for TraceLine math...

   v_forward = v_move_dir;
   v_forward.z = 0;
   if(v_forward.Length() <= 1)
      return(FALSE);
   
   v_right = UTIL_AnglesToRight(UTIL_VecToAngles(v_forward.Normalize()));
   
   // trace down to ground
   v_source = pEdict->v.origin;
   v_dest = v_source - Vector(0, 0, 72/2);
   
   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
   
   v_ground = tr.vecEndPos;
   v_ground.z += 1.0f;
      
   // 32 units right
   v_source = v_ground;
   v_dest = v_source + v_right * 32;

   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

   // if trace hit something -> no edge
   if (tr.flFraction < 1.0f)
      return(FALSE);
   
   // 45+1+1 units down from 32 units right
   v_source = v_ground + v_right * 32;
   v_dest = v_source - Vector(0, 0, 45+1+1);

   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

   // if trace hit something -> no edge
   if (tr.flFraction < 1.0f)
      return(FALSE);

   // 
   return(TRUE);
}


qboolean BotEdgeLeft( bot_t &pBot, const Vector &v_move_dir )
{
   edict_t *pEdict = pBot.pEdict;
   
   // pre checks
   if (FBitSet(pEdict->v.button, IN_DUCK) || FBitSet(pEdict->v.button, IN_JUMP) ||
       !pBot.b_on_ground || pBot.b_on_ladder)
      return FALSE;

   // 1. Trace from origin 32 units left
   //     then
   //      Trace for edge, 91 units down
   //       if hit ground -> no edge

   TraceResult tr;
   Vector v_source, v_dest, v_forward, v_right, v_ground;

   // convert current view angle to vectors for TraceLine math...

   v_forward = v_move_dir;
   v_forward.z = 0;
   if(v_forward.Length() <= 1)
      return(FALSE);
      
   v_right = UTIL_AnglesToRight(UTIL_VecToAngles(v_forward.Normalize()));
   
   // trace down to ground
   v_source = pEdict->v.origin;
   v_dest = v_source - Vector(0, 0, 72/2);
   
   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
   
   v_ground = tr.vecEndPos;
   v_ground.z += 1.0f;
      
   // 32 units right
   v_source = v_ground;
   v_dest = v_source - v_right * 32;

   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

   // if trace hit something -> no edge
   if (tr.flFraction < 1.0f)
      return(FALSE);
   
   // 45+1+1 units down from 32 units right
   v_source = v_ground - v_right * 32;
   v_dest = v_source - Vector(0, 0, 45+1+1);

   UTIL_TraceDuck( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

   // if trace hit something -> no edge
   if (tr.flFraction < 1.0f)
      return(FALSE);

   // 
   return(TRUE);
}


void BotRandomTurn( bot_t &pBot )
{
   pBot.f_move_speed = 0;  // don't move while turning
            
   if (RANDOM_LONG2(1, 100) <= 10)
   {
      // 10 percent of the time turn completely around...
      pBot.pEdict->v.ideal_yaw += 180;
   }
   else
   {
      // turn randomly between 30 and 60 degress
      if (pBot.wander_dir == WANDER_LEFT)
         pBot.pEdict->v.ideal_yaw += RANDOM_LONG2(30, 60);
      else
         pBot.pEdict->v.ideal_yaw -= RANDOM_LONG2(30, 60);
   }
            
   BotFixIdealYaw(pBot.pEdict);
}


qboolean BotCheckWallOnLeft( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;
   Vector v_src, v_left;
   TraceResult tr;

   // do a trace to the left...

   v_src = pEdict->v.origin;
   v_left = v_src + UTIL_AnglesToRight(pEdict->v.v_angle) * -40;  // 40 units to the left

   UTIL_TraceMove( v_src, v_left, dont_ignore_monsters,  pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f)
   {
      if (pBot.f_wall_on_left < 1.0)
         pBot.f_wall_on_left = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


qboolean BotCheckWallOnRight( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;
   Vector v_src, v_right;
   TraceResult tr;

   // do a trace to the right...

   v_src = pEdict->v.origin;
   v_right = v_src + UTIL_AnglesToRight(pEdict->v.v_angle) * 40;  // 40 units to the right

   UTIL_TraceMove( v_src, v_right, dont_ignore_monsters,  pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f)
   {
      if (pBot.f_wall_on_right < 1.0)
         pBot.f_wall_on_right = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


qboolean BotCheckWallOnForward( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;
   Vector v_src, v_right;
   TraceResult tr;

   // do a trace to the right...

   v_src = pEdict->v.origin;
   v_right = v_src + UTIL_AnglesToForward(pEdict->v.v_angle) * 40;  // 40 units to the forawrd

   UTIL_TraceMove( v_src, v_right, dont_ignore_monsters,  pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f)
   {
      if (pBot.f_wall_on_right < 1.0)
         pBot.f_wall_on_right = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


qboolean BotCheckWallOnBack( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;
   Vector v_src, v_right;
   TraceResult tr;

   // do a trace to the right...

   v_src = pEdict->v.origin;
   v_right = v_src + UTIL_AnglesToForward(pEdict->v.v_angle) * -40;  // 40 units to the back

   UTIL_TraceMove( v_src, v_right, dont_ignore_monsters,  pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0f)
   {
      if (pBot.f_wall_on_right < 1.0)
         pBot.f_wall_on_right = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


void BotLookForDrop( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;

   Vector v_src, v_dest, v_ahead;
   float scale, direction;
   TraceResult tr;
   int contents;
   qboolean need_to_turn, done;
   int turn_count;

   scale = 80 + (pBot.f_max_speed / 10);

   v_ahead = pEdict->v.v_angle;
   v_ahead.x = 0;  // set pitch to level horizontally

   v_src = pEdict->v.origin;
   v_dest = v_src + UTIL_AnglesToForward(v_ahead) * scale;

   UTIL_TraceMove( v_src, v_dest, ignore_monsters,  pEdict->v.pContainingEntity, &tr );

   // check if area in front of bot was clear...
   if (tr.flFraction > 0.999999f)
   {
      v_src = v_dest;  // start downward trace from endpoint of open trace
      v_dest.z = v_dest.z - max_drop_height;

      UTIL_TraceMove( v_src, v_dest, ignore_monsters,  pEdict->v.pContainingEntity, &tr );

      need_to_turn = FALSE;

      // if trace did not hit anything then drop is TOO FAR...
      if (tr.flFraction > 0.999999f) 
      {
         need_to_turn = TRUE;
      }
      else
      {
         // we've hit something, see if it's water or lava
         contents = POINT_CONTENTS( tr.vecEndPos );

         if (contents == CONTENTS_LAVA)
         {
            need_to_turn = TRUE;
         }

         if (contents == CONTENTS_WATER)
         {
            // if you don't like water, set need_to_turn = TRUE here
         }
      }

      if (need_to_turn)
      {

         // if we have an enemy, stop heading towards enemy...
         if (pBot.pBotEnemy)
         {
            BotRemoveEnemy(pBot, TRUE);
            
            // level look
            pEdict->v.idealpitch = 0;
         }

         // don't look for items for a while...
         pBot.f_find_item = gpGlobals->time + 1.0;

         // change the bot's ideal yaw by finding surface normal
         // slightly below where the bot is standing

         v_dest = pEdict->v.origin;

         if (pBot.b_ducking)
         {
            v_src.z = v_src.z - 22;  // (36/2) + 4 units
            v_dest.z = v_dest.z - 22;
         }
         else
         {
            v_src.z = v_src.z - 40;  // (72/2) + 4 units
            v_dest.z = v_dest.z - 40;
         }

         UTIL_TraceMove( v_src, v_dest, ignore_monsters, 
                         pEdict->v.pContainingEntity, &tr );

         if (tr.flFraction < 1.0f)
         {
            // hit something the bot is standing on...
            BotTurnAtWall( pBot, &tr, FALSE );
         }
         else
         {
            // pick a random direction to turn...
            if (RANDOM_LONG2(1, 100) <= 50)
               direction = 1.0f;
            else
               direction = -1.0f;

            // turn 30 degrees at a time until bot is on solid ground
            v_ahead = pEdict->v.v_angle;
            v_ahead.x = 0;  // set pitch to level horizontally

            done = FALSE;
            turn_count = 0;

            while (!done)
            {
               v_ahead.y = UTIL_WrapAngle(v_ahead.y + 30.0 * direction);

               v_src = pEdict->v.origin;
               v_dest = v_src + UTIL_AnglesToForward(v_ahead) * scale;

               UTIL_TraceMove( v_src, v_dest, ignore_monsters, 
                               pEdict->v.pContainingEntity, &tr );

               // check if area in front of bot was clear...
               if (tr.flFraction > 0.999999f)
               {
                  v_src = v_dest;  // start downward trace from endpoint of open trace
                  v_dest.z = v_dest.z - max_drop_height;

                  UTIL_TraceMove( v_src, v_dest, ignore_monsters, 
                                  pEdict->v.pContainingEntity, &tr );

                  // if trace hit something then drop is NOT TOO FAR...
                  if (tr.flFraction > 0.999999f)
                     done = TRUE;
               }

               turn_count++;
               if (turn_count == 6)  // 180 degrees? (30 * 6 = 180)
                  done = TRUE;
            }

            pBot.pEdict->v.ideal_yaw = v_ahead.y;
            BotFixIdealYaw(pEdict);
         }
      }
   }
}

