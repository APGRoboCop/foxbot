//
// JK_Botti - be more human!
//
// bot_combat.cpp
//

#define BOTCOMBAT

#ifndef _WIN32
#include <string.h>
#endif

#include <malloc.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "bot_sound.h"
#include "player.h"

extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern qboolean b_observer_mode;
extern qboolean is_team_play;
extern qboolean checked_teamplay;
extern int num_logos;
extern int submod_id;
extern qboolean b_botdontshoot;

char g_team_list[TEAMPLAY_TEAMLISTLENGTH];
char g_team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
int g_team_scores[MAX_TEAMS];
int g_num_teams = 0;
qboolean g_team_limit = FALSE;

typedef struct select_list_s
{
   struct select_list_s *next;
   qboolean use_primary, use_secondary;
   int percent_start, percent_end, select_index;
} select_list_t;


//
static qboolean BotAimsAtSomething (bot_t &pBot)
{
   if(!pBot.pBotEnemy)
      return FALSE;
   
   qboolean visible = FPredictedVisible(pBot);

   return(visible);
}


//
static void BotPointGun(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   
   // this function purpose is to make the bot move its crosshair to the direction 
   // where it wants to look. There is some kind of filtering for the view, to make 
   // it human-like.

   float frame_time = pBot.f_frame_time / skill_settings[pBot.bot_skill].turn_slowness;
   float speed; // speed : 0.1 - 1
   Vector v_deviation;
   float turn_skill = skill_settings[pBot.bot_skill].turn_skill;
   
   v_deviation = UTIL_WrapAngles (Vector (pEdict->v.idealpitch, pEdict->v.ideal_yaw, 0) - pEdict->v.v_angle);

   // if bot is aiming at something, aim fast, else take our time...
   if (pBot.b_combat_longjump)
      speed = 0.7 + (turn_skill - 1) / 10; // fast aim
   else if(BotAimsAtSomething (pBot))
   {
      float custom = 1.0;
      
      // customized aim speed with weapons
      bot_weapon_select_t * pSelect = GetWeaponSelect(pBot.current_weapon.iId);
      if(pSelect)
         custom = (pSelect->aim_speed <= 1.0 && pSelect->aim_speed >= 0.0) ? pSelect->aim_speed : 1.0;
      
      speed = (0.5 * custom + 0.2) + (turn_skill - 1) / (10 * (1.0 - custom) + 10);
   }
   else if(pBot.curr_waypoint_index != -1 || !FNullEnt(pBot.pBotPickupItem))
      speed = 0.5 + (turn_skill - 1) / 15; // medium aim
   else
      speed = 0.2 + (turn_skill - 1) / 20; // slow aim

   // thanks Tobias "Killaruna" Heimann and Johannes "@$3.1415rin" Lampel for this one
   pEdict->v.yaw_speed = (pEdict->v.yaw_speed * exp (log (speed / 2) * frame_time * 20)
                             + speed * v_deviation.y * (1 - exp (log (speed / 2) * frame_time * 20)))
                            * frame_time * 20;
   pEdict->v.pitch_speed = (pEdict->v.pitch_speed * exp (log (speed / 2) * frame_time * 20)
                               + speed * v_deviation.x * (1 - exp (log (speed / 2) * frame_time * 20)))
                              * frame_time * 20;

   // influence of y movement on x axis, based on skill (less influence than x on y since it's
   // easier and more natural for the bot to "move its mouse" horizontally than vertically)
   if (pEdict->v.pitch_speed > 0)
      pEdict->v.pitch_speed += pEdict->v.yaw_speed / (skill_settings[pBot.bot_skill].updown_turn_ration * (1 + turn_skill));
   else
      pEdict->v.pitch_speed -= pEdict->v.yaw_speed / (skill_settings[pBot.bot_skill].updown_turn_ration * (1 + turn_skill));

   // influence of x movement on y axis, based on skill
   if (pEdict->v.yaw_speed > 0)
      pEdict->v.yaw_speed += pEdict->v.pitch_speed / (1 + turn_skill);
   else
      pEdict->v.yaw_speed -= pEdict->v.pitch_speed / (1 + turn_skill);
   
   // move the aim cursor
   pEdict->v.v_angle = UTIL_WrapAngles (pEdict->v.v_angle + Vector (pEdict->v.pitch_speed, pEdict->v.yaw_speed, 0)); 
   
   // set the body angles to point the gun correctly
   pEdict->v.angles.x = UTIL_WrapAngle (-pEdict->v.v_angle.x / 3);
   pEdict->v.angles.y = UTIL_WrapAngle (pEdict->v.v_angle.y);
   pEdict->v.angles.z = 0;
}


// Called before g_engfuncs.pfnRunPlayerMove
void BotAimPre( bot_t &pBot ) 
{   
   // make bot aim and turn
   BotPointGun(pBot); // update and save this bot's view angles
      
   // wrap angles that were not wrapped in pointgun
   pBot.pEdict->v.idealpitch = UTIL_WrapAngle(pBot.pEdict->v.idealpitch);
   pBot.pEdict->v.ideal_yaw = UTIL_WrapAngle(pBot.pEdict->v.ideal_yaw);
   
   // special aiming angle for mp5 grenade
   if(pBot.b_set_special_shoot_angle)
   {
      float old_angle = pBot.pEdict->v.v_angle.z;
      
      pBot.pEdict->v.v_angle.z = pBot.f_special_shoot_angle;
      pBot.pEdict->v.angles.x = UTIL_WrapAngle (-pBot.pEdict->v.v_angle.x / 3);
      
      pBot.f_special_shoot_angle = old_angle;
   }
}


// Called after g_engfuncs.pfnRunPlayerMove
void BotAimPost( bot_t &pBot )
{
   // special aiming angle for mp5 grenade
   if(pBot.b_set_special_shoot_angle)
   {
      pBot.pEdict->v.v_angle.z = pBot.f_special_shoot_angle;
      pBot.pEdict->v.angles.x = UTIL_WrapAngle (-pBot.pEdict->v.v_angle.x / 3);
      
      pBot.b_set_special_shoot_angle = FALSE;
      pBot.f_special_shoot_angle = 0.0;
   }
   
   // special case for m249
   if(pBot.current_weapon.iId == GEARBOX_WEAPON_M249)
   {
      if((pBot.pEdict->v.flags & FL_DUCKING) == FL_DUCKING)
         pBot.f_recoil /= 4;
   }
   // special case for eagle
   else if(pBot.current_weapon.iId == GEARBOX_WEAPON_EAGLE)
   {
      if(pBot.eagle_secondary_state != 0)
         pBot.f_recoil /= 15;
   }
   
   // add any recoil left to punch angle now
   pBot.pEdict->v.punchangle.x += pBot.f_recoil;
   
   pBot.f_recoil = 0;
}


// flavour xy axis over z on finding closest enemy
static Vector GetModifiedEnemyDistance(bot_t &pBot, const Vector & distance)
{
   return( Vector(distance.x, distance.y, distance.z * skill_settings[pBot.bot_skill].updown_turn_ration) );
}


//
static void BotResetReactionTime(bot_t &pBot, qboolean have_slow_reaction = FALSE) 
{
   float delay_min = skill_settings[pBot.bot_skill].react_delay_min;
   float delay_max = skill_settings[pBot.bot_skill].react_delay_max;

   float react_delay = RANDOM_FLOAT2(delay_min, delay_max);

   if(have_slow_reaction)
      react_delay *= 4;

   pBot.f_reaction_target_time = gpGlobals->time + react_delay;

   pBot.f_next_find_visible_sound_enemy_time = gpGlobals->time + 0.2f;
}


// called in clientdisconnect
void free_posdata_list(int idx) 
{
   memset(players[idx].posdata_mem, 0, sizeof(players[idx].posdata_mem));
   
   players[idx].position_oldest = 0;
   players[idx].position_latest = 0;
}


static posdata_t *get_posdata_slot(int idx)
{
   int i, oldest_idx = -1;
   float oldest_time = gpGlobals->time;

   for(i = 0; i < POSDATA_SIZE; i++)
   {
      if(!players[idx].posdata_mem[i].inuse)
         break;

      /* ugly fix */
      /* we cannot have future time, there has been some error. */
      if (players[idx].posdata_mem[i].time > gpGlobals->time)
          players[idx].posdata_mem[i].time = gpGlobals->time;

      if (players[idx].posdata_mem[i].time <= oldest_time)
      {
         oldest_time = players[idx].posdata_mem[i].time;
         oldest_idx = i;
      }
   }
   
   if(i >= POSDATA_SIZE)
   {
      if(oldest_idx == -1)
         return(NULL);
      
      i = oldest_idx;
   }
   
   memset(&players[idx].posdata_mem[i], 0, sizeof(players[idx].posdata_mem[i]));
   players[idx].posdata_mem[i].time = gpGlobals->time;
   players[idx].posdata_mem[i].inuse = TRUE;
   
   return(&players[idx].posdata_mem[i]);
}


//
static void add_next_posdata(int idx, edict_t *pEdict)
{
   posdata_t * new_latest = get_posdata_slot(idx);
   
   JKASSERT(new_latest == NULL);
   if(new_latest == NULL)
      return;
   
   posdata_t * curr_latest = players[idx].position_latest;
   players[idx].position_latest = new_latest;
   
   if(curr_latest != NULL) 
   {
      curr_latest->newer = players[idx].position_latest;
   }
   
   players[idx].position_latest->older = curr_latest;
   players[idx].position_latest->newer = NULL;
   
   players[idx].position_latest->origin = pEdict->v.origin;
   players[idx].position_latest->velocity = pEdict->v.basevelocity + pEdict->v.velocity;   
   
   players[idx].position_latest->was_alive = !!IsAlive(pEdict);
   players[idx].position_latest->ducking = (pEdict->v.flags & FL_DUCKING) == FL_DUCKING;
   
   players[idx].position_latest->time = gpGlobals->time;
   
   if(!players[idx].position_oldest)
      players[idx].position_oldest = players[idx].position_latest;
}


// remove data older than max + 100ms
static void timetrim_posdata(int idx) 
{
   posdata_t * list;
   
   if(!(list = players[idx].position_oldest))
      return;
   
   while(list) 
   {
      // max + 100ms
      // max is maximum by skill + max randomness added in GetPredictedPlayerPosition()
      if(list->time + (skill_settings[4].ping_emu_latency + 0.1) <= gpGlobals->time) 
      {
         posdata_t * next = list->newer;
         
         //mark slot free
         list->inuse = FALSE;
         
         list = next;
         list->older = 0;
         players[idx].position_oldest = list;
      }
      else 
      {
         // new enough.. so are all the rest
         break;
      }
   }
   
   if(!players[idx].position_oldest) 
   {
	   JKASSERT(players[idx].position_latest != 0);
	  
      players[idx].position_oldest = 0;
      players[idx].position_latest = 0;
   }
}


// called every PlayerPostThink
void GatherPlayerData(edict_t * pEdict) 
{
   int idx = -1;
   
   if(!FNullEnt(pEdict) && !FBitSet(pEdict->v.flags, FL_PROXY))
      idx = ENTINDEX(pEdict) - 1;
   
   if(idx < 0 || idx >= gpGlobals->maxClients)
      return;
   
   add_next_posdata(idx, pEdict);
   timetrim_posdata(idx);
}


//
static Vector AddPredictionVelocityVaritation(bot_t &pBot, const Vector & velocity)
{
   if(velocity.x == 0 && velocity.y == 0)
      return velocity;
   
   float maxvar = (1.0 + skill_settings[pBot.bot_skill].ping_emu_speed_varitation);
   float minvar = (1.0 - skill_settings[pBot.bot_skill].ping_emu_speed_varitation);

#if 0
   Vector2D flat = Vector2D(velocity.x, velocity.y);
   
   flat = flat.Normalize() * (flat.Length() * RANDOM_FLOAT2(minvar, maxvar));

   return Vector(flat.x, flat.y, velocity.z);
#else
   return velocity.Normalize() * (velocity.Length() * RANDOM_FLOAT2(minvar, maxvar));
#endif
}

//
static Vector AddPredictionPositionVaritation(bot_t &pBot)
{
   Vector v_rnd_angles;
   
   v_rnd_angles.x = UTIL_WrapAngle(RANDOM_FLOAT2(-90, 90));
   v_rnd_angles.y = UTIL_WrapAngle(RANDOM_FLOAT2(-180, 180));
   v_rnd_angles.z = 0;
   
   return UTIL_AnglesToForward(v_rnd_angles) * skill_settings[pBot.bot_skill].ping_emu_position_varitation;
}


// Prevent bots from shooting at on ground when aiming on falling player that hits ground (Z axis fixup only)
static Vector TracePredictedMovement(bot_t &pBot, edict_t *pPlayer, const Vector &v_src, const Vector &cv_velocity, float time, qboolean ducking, qboolean without_velocity)
{
   if(without_velocity)
      return(v_src);
   
   Vector v_dest, v_velocity;
   
   v_velocity = AddPredictionVelocityVaritation(pBot, cv_velocity);
   v_dest = v_src + v_velocity * time + AddPredictionPositionVaritation(pBot);

   TraceResult tr;
   UTIL_TraceHull( v_src, v_dest, ignore_monsters, (ducking) ? head_hull : human_hull, pPlayer ? pPlayer->v.pContainingEntity : NULL, &tr);
   
   if(!tr.fStartSolid && tr.flFraction < 1.0f)
      v_dest.z = tr.vecEndPos.z;
   
   return(v_dest);
}


// used instead of using pBotEnemy->v.origin in aim code.
//  if bot's aim lags behind moving target increase value of AHEAD_MULTIPLIER.
#define AHEAD_MULTIPLIER 1.5
static Vector GetPredictedPlayerPosition(bot_t &pBot, edict_t * pPlayer, qboolean without_velocity = FALSE) 
{
   posdata_t * newer;
   posdata_t * older;
   float time;
   int idx;
   posdata_t newertmp;
   
   if(FNullEnt(pPlayer))
      return(Vector(0,0,0));
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients || !players[idx].position_latest || !players[idx].position_oldest)
      return(UTIL_GetOrigin(pPlayer));
   
   // get prediction time based on bot skill
   time = gpGlobals->time - skill_settings[pBot.bot_skill].ping_emu_latency;
   
   // find position data slots that are around 'time'
   newer = players[idx].position_latest;
   older = 0;
   while(newer) 
   {
      if(newer->time > time) 
      {
         //this is too new for us.. proceed
         newer = newer->older;
         continue;
      }
      if(newer->time == time) 
      {
         return(TracePredictedMovement(pBot, pPlayer, newer->origin, newer->velocity, fabs(gpGlobals->time - newer->time) * AHEAD_MULTIPLIER, newer->ducking, without_velocity)); 
      }
      
      //this time is older than previous..
      older = newer;
      newer = older->newer;
      break;
   }
   
   if(!older) 
   {
      return(TracePredictedMovement(pBot, pPlayer, players[idx].position_oldest->origin, players[idx].position_oldest->velocity, fabs(gpGlobals->time - players[idx].position_oldest->time) * AHEAD_MULTIPLIER, players[idx].position_oldest->ducking, without_velocity)); 
   }
   
   if(!newer) 
   {
      memset(&newertmp, 0, sizeof(newertmp));
      
      newertmp.origin = pPlayer->v.origin;
      newertmp.velocity = pPlayer->v.basevelocity + pPlayer->v.velocity;   
      newertmp.time = gpGlobals->time;  
      newertmp.was_alive = !!IsAlive(pPlayer);
      
      newer = &newertmp;
   }
   
   // don't mix dead data with alive data
   if(!newer->was_alive && older->was_alive) 
   {
      return(TracePredictedMovement(pBot, pPlayer, newer->origin, newer->velocity, fabs(gpGlobals->time - newer->time) * AHEAD_MULTIPLIER, newer->ducking, without_velocity)); 
   }
   if(!older->was_alive && newer->was_alive) 
   {
      return(TracePredictedMovement(pBot, pPlayer, older->origin, older->velocity, fabs(gpGlobals->time - older->time) * AHEAD_MULTIPLIER, older->ducking, without_velocity)); 
   }
   
   float newer_diff = fabs(newer->time - time);
   float older_diff = fabs(older->time - time);
   float total_diff = newer_diff + older_diff;
   
   if(total_diff <= 0.0) 
   {
      // zero div would crash server.. 
      // zero diff means that both data are from same time
      return(TracePredictedMovement(pBot, pPlayer, newer->origin, newer->velocity, fabs(gpGlobals->time - newer->time) * AHEAD_MULTIPLIER, newer->ducking, without_velocity)); 
   }
   
   // make weighted average
   Vector pred_origin = (older_diff/total_diff) * newer->origin + (newer_diff/total_diff) * older->origin;
   Vector pred_velocity;
   if(!without_velocity)
      pred_velocity = (older_diff/total_diff) * newer->velocity + (newer_diff/total_diff) * older->velocity;
   
   // use old origin and use old velocity to predict current position
   return(TracePredictedMovement(pBot, pPlayer, pred_origin, pred_velocity, fabs(gpGlobals->time - time) * AHEAD_MULTIPLIER, newer->ducking, without_velocity)); 
}


//
qboolean FPredictedVisible(bot_t &pBot)
{
   if(!pBot.pBotEnemy)
      return(FALSE);
   
   Vector v_enemy = GetPredictedPlayerPosition(pBot, pBot.pBotEnemy, TRUE); //only get position

   return(FVisibleEnemy(v_enemy, pBot.pEdict, pBot.pBotEnemy));
}


#if 0
//
static qboolean GetPredictedIsAlive(edict_t * pPlayer, float time) 
{
   posdata_t * newer;
   int idx;
   
   idx = ENTINDEX(pPlayer) - 1;
   if(idx < 0 || idx >= gpGlobals->maxClients || !players[idx].position_latest || !players[idx].position_oldest)
      return(!!IsAlive(pPlayer));
   
   // find position data slots that are around 'time'
   newer = players[idx].position_latest;
   while(newer) 
   {
      if(newer->time > time) 
      {
         //this is too new for us.. proceed
         newer = newer->older;
         continue;
      }
      if(newer->time == time) 
      {
         return(newer->was_alive);
      }
      
      //this time is older than previous..
      newer = newer->newer;
      break;
   }
   
   if(!newer) 
   {
      return(!!IsAlive(pPlayer));
   }
   
   return(newer->was_alive);
}


//
static qboolean HaveSameModels(edict_t * pEnt1, edict_t * pEnt2) 
{
   char *infobuffer;
   char model_name1[32];
   char model_name2[32];
   
   infobuffer = GET_INFOKEYBUFFER( pEnt1 );
   *model_name1=0; strncat(model_name1, INFOKEY_VALUE (infobuffer, "model"), sizeof(model_name1));
   
   infobuffer = GET_INFOKEYBUFFER( pEnt2 );
   *model_name2=0; strncat(model_name2, INFOKEY_VALUE (infobuffer, "model"), sizeof(model_name2));
   
   return(!stricmp(model_name1, model_name2));
}
#endif


//
static qboolean FCanShootInHead(edict_t * pEdict, edict_t * pTarget, const Vector & v_dest)
{
   if(!FIsClassname("player", pTarget))
      return FALSE;
   
   float neg = pTarget->v.view_ofs.z >= 2.0f ? 2.0f : 0.0f;
   
   // first check for if head is visible
   if(!FVisible( pTarget->v.origin + (pTarget->v.view_ofs - Vector(0,0,neg)), pEdict, pTarget ))
      return FALSE;
   
   // check center/feet
   if(!FVisible( pTarget->v.origin, pEdict, pTarget ))
      if(!FVisible( pTarget->v.origin - (pTarget->v.view_ofs - Vector(0,0,neg)), pEdict, pTarget ))
         return TRUE; //only head visible
   
   float distance = (v_dest - pEdict->v.origin).Length();
   
   Vector2D triangle;
   
   triangle.x = distance;
   triangle.y = pTarget->v.view_ofs.z - neg;
   
   if(cos(deg2rad(12.5)) < (distance / triangle.Length()))
      return FALSE; //greater angle, smaller cosine
   
   return TRUE;
}


static qboolean AreTeamMates(edict_t * pOther, edict_t * pEdict) 
{
   // is team play enabled?
   if (is_team_play)
   {
      char other_model[MAX_TEAMNAME_LENGTH];
      char edict_model[MAX_TEAMNAME_LENGTH];
      
      return(!stricmp(UTIL_GetTeam(pOther, other_model, sizeof(other_model)), UTIL_GetTeam(pEdict, edict_model, sizeof(edict_model))));
   }
   
   return FALSE;
}


//
static edict_t *BotFindEnemyNearestToPoint(bot_t &pBot, const Vector &v_point, float radius, Vector *v_found)
{
   edict_t *pBotEdict = pBot.pEdict;
   
   float nearestdistance = radius;
   edict_t * pNearestEnemy = NULL;
   Vector v_nearestorigin = Vector(0,0,0);

   // search the world for players...
   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      Vector v_player;
      edict_t *pPlayer = INDEXENT(i);

      // skip invalid players and skip self (i.e. this bot)
      if ((pPlayer) && (!pPlayer->free) && (pPlayer != pBotEdict) && !FBitSet(pPlayer->v.flags, FL_PROXY))
      {
         if ((b_observer_mode) && !(FBitSet(pPlayer->v.flags, FL_FAKECLIENT) || FBitSet(pPlayer->v.flags, FL_THIRDPARTYBOT)))
            continue;

         // check first.. since this is very fast check
         Vector v_origin = pPlayer->v.origin;
         float distance = (v_origin - v_point).Length();
         if (distance > nearestdistance)
            continue;

         // skip this player if not alive (i.e. dead or dying)
         if (!IsAlive(pPlayer))
            continue;

         // skip this player if respawned lately
         float time_since_respawn = UTIL_GetTimeSinceRespawn(pPlayer);
         if(time_since_respawn != -1.0 && time_since_respawn < skill_settings[pBot.bot_skill].respawn_react_delay)
            continue;

         // skip this player if facing wall
         if(IsPlayerChatProtected(pPlayer))
            continue;

         // don't target teammates
         if(AreTeamMates(pPlayer, pBotEdict))
            continue;

         // see if bot can't see the player...
         if (!FVisible( v_point, pPlayer, pBotEdict ))
            continue;

         nearestdistance = distance;
         pNearestEnemy = pPlayer;
         v_nearestorigin = v_origin;
      }
   }
   
   if(nearestdistance <= radius)
   {
      if(v_found)
         *v_found = v_nearestorigin;
      
      return(pNearestEnemy);
   }
   
   return(NULL);
}


// called on every think frame
void BotUpdateHearingSensitivity(bot_t &pBot)
{
   if (pBot.pBotEnemy != NULL)
   {
      // have enemy, use best hearing sensitivity for all
      pBot.f_current_hearing_sensitivity = skill_settings[BEST_BOT_LEVEL].hearing_sensitivity;
      return;
   }

   // reduce from best to worst in 3 sec
   pBot.f_current_hearing_sensitivity -= (skill_settings[BEST_BOT_LEVEL].hearing_sensitivity - skill_settings[WORST_BOT_LEVEL].hearing_sensitivity) * pBot.f_frame_time / 3;
   if (pBot.f_current_hearing_sensitivity < skill_settings[pBot.bot_skill].hearing_sensitivity)
      pBot.f_current_hearing_sensitivity = skill_settings[pBot.bot_skill].hearing_sensitivity;
}


//
static float BotGetHearingSensitivity(bot_t &pBot)
{
   if(pBot.f_current_hearing_sensitivity < skill_settings[pBot.bot_skill].hearing_sensitivity)
      return skill_settings[pBot.bot_skill].hearing_sensitivity;
   return pBot.f_current_hearing_sensitivity;
}


//
static edict_t *BotFindVisibleSoundEnemy( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;
   
   int iSound;
   CSound *pCurrentSound;
   float mindistance;
   edict_t *pMinDistanceEdict = NULL;
   
   mindistance = 99999.0;
      
   // walk through active sound linked list
   for(iSound = CSoundEnt::ActiveList(); iSound != SOUNDLIST_EMPTY; iSound = pCurrentSound->m_iNext)
   {
      pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );
      
      if(!pCurrentSound)
         continue;
      
      // ignore sounds created by bot itself
      if(pCurrentSound->m_iBotOwner == (&pBot - &bots[0]))
         continue;
      
      // is sound too far away? (bot cannot hear)
      float s_distance = (pCurrentSound->m_vecOrigin - pEdict->v.origin).Length();
      if(s_distance > pCurrentSound->m_iVolume * BotGetHearingSensitivity(pBot))
         continue;

      // more distant than what we got already?
      float distance = (pCurrentSound->m_vecOrigin - pEdict->v.origin).Length();      
      if (distance >= mindistance)
         continue;
      
      // any enemy near sound?
      Vector v_enemy = Vector(0,0,0);
      edict_t * pNewEnemy = BotFindEnemyNearestToPoint(pBot, pCurrentSound->m_vecOrigin, 512.0, &v_enemy);
      if(FNullEnt(pNewEnemy))
         continue;
      
      // is enemy visible?
      if (!FVisible( v_enemy, pEdict, pNewEnemy ))
         continue;
      
      mindistance = distance;
      pMinDistanceEdict = pNewEnemy;
   }
   
   return pMinDistanceEdict;
}


//
void BotRemoveEnemy( bot_t &pBot, qboolean b_keep_tracking )
{
   edict_t *pEdict = pBot.pEdict;
   
   JKASSERT(pBot.pBotEnemy == NULL);
   
   // track enemy?
   if(b_keep_tracking)
   {
      float track_time = RANDOM_FLOAT2(skill_settings[pBot.bot_skill].track_sound_time_min, skill_settings[pBot.bot_skill].track_sound_time_max);
      
      pBot.wpt_goal_type = WPT_GOAL_TRACK_SOUND;
      pBot.waypoint_goal = -1;
      
      pBot.pTrackSoundEdict = pBot.pBotEnemy;
      pBot.f_track_sound_time = gpGlobals->time + track_time; //TODO: bot skill 20.0f .. ~5.0f
      
      if(BotUpdateTrackSoundGoal(pBot) && pBot.waypoint_goal == -1)
      {
         int waypoint = WaypointFindNearest(pBot.pTrackSoundEdict, 1024);
         
         //if(pBot.waypoint_goal != waypoint)
         //   UTIL_ConsolePrintf("[%s] Couldn't find sound to track, using edict-wpt: %d -> %d", pBot.name, pBot.waypoint_goal, waypoint);
         
         pBot.waypoint_goal = waypoint;
      }
   }
   
   // don't have an enemy anymore so null out the pointer...
   pBot.pBotEnemy = NULL;
   
   // reset reactions
   BotResetReactionTime(pBot);
   pBot.f_next_find_visible_sound_enemy_time = gpGlobals->time + 0.2f;
   
   // update bot waypoint after combat
   pBot.curr_waypoint_index = WaypointFindNearest(pEdict, 1024);
}


//
void BotFindEnemy( bot_t &pBot )
{
//#define DEBUG_ENEMY_SELECT 1
#if DEBUG_ENEMY_SELECT
   char info[1024];
   const char *enemy_type = "";
#endif
   edict_t *pNewEnemy; 
   Vector v_newenemy;
   float nearestdistance;
   qboolean chatprot = FALSE;
   int i;

   edict_t *pEdict = pBot.pEdict;
   
   if (b_botdontshoot)
   {
      pBot.f_bot_see_enemy_time = -1;  // so we won't keep reloading
      pBot.v_bot_see_enemy_origin = Vector(-99999,-99999,-99999);

      pEdict->v.button |= IN_RELOAD;  // press reload button
      
      BotRemoveEnemy(pBot, FALSE);
      
      return;
   }

   if (pBot.pBotEnemy != NULL)  // does the bot already have an enemy?
   {
      // if the enemy is dead?
      // if the enemy is chat protected?
      // is the enemy dead?, assume bot killed it
      if (!IsAlive(pBot.pBotEnemy) || TRUE == (chatprot = IsPlayerChatProtected(pBot.pBotEnemy))) 
      {
         // the enemy is dead, jump for joy about 10% of the time
         if (!chatprot && RANDOM_LONG2(1, 100) <= 10)
            pEdict->v.button |= IN_JUMP;

         // don't have an enemy anymore so null out the pointer...
         BotRemoveEnemy(pBot, FALSE);
         
         // level look
         pEdict->v.idealpitch = 0;
      }
      else  // enemy is still alive
      {
         Vector vecEnd;
         Vector vecPredEnemy;

         vecPredEnemy = UTIL_AdjustOriginWithExtent(pBot, GetPredictedPlayerPosition(pBot, pBot.pBotEnemy), pBot.pBotEnemy);

         vecEnd = vecPredEnemy;
         if(FCanShootInHead(pEdict, pBot.pBotEnemy, vecPredEnemy))
            vecEnd += pBot.pBotEnemy->v.view_ofs;
         
         if( FInViewCone( vecEnd, pEdict ) && FVisibleEnemy( vecEnd, pEdict, pBot.pBotEnemy ))
         {            
            // face the enemy
            Vector v_enemy = vecPredEnemy - pEdict->v.origin;
            Vector bot_angles = UTIL_VecToAngles( v_enemy );

            pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(pEdict);

            // keep track of when we last saw an enemy
            pBot.f_bot_see_enemy_time = gpGlobals->time;
            pBot.v_bot_see_enemy_origin = UTIL_GetOrigin(pBot.pBotEnemy);

            return; // don't change pBot.pBotEnemy
         }
         
         // enemy has gone out of bot's line of sight
         if( pBot.f_bot_see_enemy_time > 0 && pBot.f_bot_see_enemy_time + 0.5 >= gpGlobals->time)
         {
            // start sound tracking
            BotRemoveEnemy(pBot, TRUE);
            
            // level look
            pEdict->v.idealpitch = 0;
         }
      }
   }

   pNewEnemy = NULL;
   v_newenemy = Vector(0,0,0);
   nearestdistance = 99999;

   if (pNewEnemy == NULL)
   {
      breakable_list_t * pBreakable;
      edict_t *pMonster;
      Vector vecEnd;

      // search func_breakables that we collected at map start (We need to collect in order to get the material value)
      pBreakable = NULL;
      while((pBreakable = UTIL_FindBreakable(pBreakable)) != NULL) 
      {
         // removed? null?
         if(FNullEnt (pBreakable->pEdict))
            continue;
         
         // cannot take damage
         if((int)pBreakable->pEdict->v.takedamage == DAMAGE_NO ||
            pBreakable->pEdict->v.solid == SOLID_NOT || 
            pBreakable->pEdict->v.deadflag == DEAD_DEAD ||
            !pBreakable->material_breakable ||
            pBreakable->pEdict->v.health <= 0)
            continue;

         if (pBreakable->pEdict->v.health > 8000)
	    continue; // skip breakables with large health
         
         Vector v_origin = UTIL_GetOriginWithExtent(pBot, pBreakable->pEdict);

         // 0,0,0 is considered invalid
         if(v_origin.is_zero_vector())
            continue;
         
         float distance = GetModifiedEnemyDistance(pBot, v_origin - pEdict->v.origin).Length();
         if (distance >= nearestdistance)
            continue;
         
         // see if bot can't see ...
         if (!(FInViewCone( v_origin, pEdict ) && FVisible( v_origin, pEdict, pBreakable->pEdict )))
            continue;

         nearestdistance = distance;
         pNewEnemy = pBreakable->pEdict;
         v_newenemy = v_origin;

#if DEBUG_ENEMY_SELECT
	 enemy_type = "breakable";
	 snprintf(info, sizeof(info), "%s[or:e-%.1f:%.1f:%.1f, or:o-%.1f:%.1f:%.1f, mins-%.1f:%.1f:%.1f, maxs-%.1f:%.1f:%.1f, absmin-%.1f:%.1f:%.1f, size-%.1f:%.1f:%.1f, td-%.0f, s-%d, df-%d, h-%.0f]",
		  enemy_type, v_origin.x, v_origin.y, v_origin.z,
		  UTIL_GetOrigin(pBreakable->pEdict).x, UTIL_GetOrigin(pBreakable->pEdict).y, UTIL_GetOrigin(pBreakable->pEdict).z,
		  pBreakable->pEdict->v.mins.x, pBreakable->pEdict->v.mins.y, pBreakable->pEdict->v.mins.z,
		  pBreakable->pEdict->v.maxs.x, pBreakable->pEdict->v.maxs.y, pBreakable->pEdict->v.maxs.z,
	          pBreakable->pEdict->v.absmin.x, pBreakable->pEdict->v.absmin.y, pBreakable->pEdict->v.absmin.z,
		  pBreakable->pEdict->v.size.x, pBreakable->pEdict->v.size.y, pBreakable->pEdict->v.size.z,
		  pBreakable->pEdict->v.takedamage,
		  pBreakable->pEdict->v.solid,
		  pBreakable->pEdict->v.deadflag,
		  pBreakable->pEdict->v.health
 		);
	 enemy_type = info;
#endif
      }

      // search the world for monsters...
      pMonster = NULL;
      while (!FNullEnt (pMonster = UTIL_FindEntityInSphere (pMonster, pEdict->v.origin, 1000)))
      {
         if (!(pMonster->v.flags & FL_MONSTER) || (int)pMonster->v.takedamage == DAMAGE_NO)
            continue; // discard anything that is not a monster
                  
         if (!IsAlive (pMonster))
            continue; // discard dead or dying monsters

         // 0,0,0 is considered invalid
         if(pMonster->v.origin.is_zero_vector())
            continue;

         if (FIsClassname(pMonster, "hornet"))
            continue; // skip hornets
         
         if (FIsClassname(pMonster, "monster_snark"))
            continue; // skip snarks

         if (pMonster->v.health > 4000)
	    continue; // skip monsters with large health

         float distance = GetModifiedEnemyDistance(pBot, UTIL_GetOriginWithExtent(pBot, pMonster) - pEdict->v.origin).Length();
         if (distance >= nearestdistance)
            continue;

         vecEnd = pMonster->v.origin + pMonster->v.view_ofs;

         // see if bot can't see ...
         if (!(FInViewCone( vecEnd, pEdict ) && FVisibleEnemy( vecEnd, pEdict, pMonster )))
            continue;

         nearestdistance = distance;
         pNewEnemy = pMonster;
         v_newenemy = pMonster->v.origin;

#if DEBUG_ENEMY_SELECT
	 enemy_type = "monster";
	 snprintf(info, sizeof(info), "%s[or:o-%.1f:%.1f:%.1f, or:g-%.1f:%.1f:%.1f, td-%.0f(%f), df-%d, h-%.0f, f-%d, s-%d]",
		  enemy_type,
		  pMonster->v.origin.x, pMonster->v.origin.y, pMonster->v.origin.z,
		  UTIL_GetOrigin(pMonster).x, UTIL_GetOrigin(pMonster).y, UTIL_GetOrigin(pMonster).z,
		  pMonster->v.takedamage, pMonster->v.takedamage - (long long)pMonster->v.takedamage,
		  pMonster->v.deadflag,
		  pMonster->v.health,
		  pMonster->v.flags,
		  pMonster->v.solid
		);
	 enemy_type = info;
#endif
      }

      // search the world for players...
      for (i = 1; i <= gpGlobals->maxClients; i++)
      {
         Vector v_player;
         edict_t *pPlayer = INDEXENT(i);

         // skip invalid players and skip self (i.e. this bot)
         if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict) && !FBitSet(pPlayer->v.flags, FL_PROXY))
         {
            if ((b_observer_mode) && !(FBitSet(pPlayer->v.flags, FL_FAKECLIENT) || FBitSet(pPlayer->v.flags, FL_THIRDPARTYBOT)))
               continue;

            // 0,0,0 is considered invalid
            if(pPlayer->v.origin.is_zero_vector())
               continue;

            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
               continue;

            // skip this player if facing wall
            if(IsPlayerChatProtected(pPlayer))
               continue;

            // don't target teammates
            if(AreTeamMates(pPlayer, pEdict))
               continue;
            
            float distance = GetModifiedEnemyDistance(pBot, UTIL_GetOriginWithExtent(pBot, pPlayer) - pEdict->v.origin).Length();
            if (distance >= nearestdistance)
               continue;
              
            // skip this player if respawned lately
            float time_since_respawn = UTIL_GetTimeSinceRespawn(pPlayer);
            if(time_since_respawn != -1.0 && time_since_respawn < skill_settings[pBot.bot_skill].respawn_react_delay)
               continue;

            vecEnd = GetGunPosition(pPlayer);

            // see if bot can't see the player...
            if (!(FInViewCone( vecEnd, pEdict ) && FVisibleEnemy( vecEnd, pEdict, pPlayer )))
               continue;

            nearestdistance = distance;
            pNewEnemy = pPlayer;
            v_newenemy = v_player;

#if DEBUG_ENEMY_SELECT
	    enemy_type = "player";
	    snprintf(info, sizeof(info), "%s[or:o-%.1f:%.1f:%.1f, or:g-%.1f:%.1f:%.1f, mins-%.1f:%.1f:%.1f, maxs-%.1f:%.1f:%.1f, absmin-%.1f:%.1f:%.1f, size-%.1f:%.1f:%.1f, td-%.0f(%f), df-%d, h-%.0f, f-%d, s-%d]",
		  enemy_type,
		  pPlayer->v.origin.x, pPlayer->v.origin.y, pPlayer->v.origin.z,
		  UTIL_GetOrigin(pPlayer).x, UTIL_GetOrigin(pPlayer).y, UTIL_GetOrigin(pPlayer).z,
		  pPlayer->v.mins.x, pPlayer->v.mins.y, pPlayer->v.mins.z,
		  pPlayer->v.maxs.x, pPlayer->v.maxs.y, pPlayer->v.maxs.z,
	          pPlayer->v.absmin.x, pPlayer->v.absmin.y, pPlayer->v.absmin.z,
		  pPlayer->v.size.x, pPlayer->v.size.y, pPlayer->v.size.z,
		  pPlayer->v.takedamage, pPlayer->v.takedamage - (long long)pPlayer->v.takedamage,
		  pPlayer->v.deadflag,
		  pPlayer->v.health,
		  pPlayer->v.flags,
		  pPlayer->v.solid
		);
	    enemy_type = info;
#endif
         }
      }
   }

   qboolean is_sound_enemy = FALSE;

   // check sounds for any potential targets
   if (FNullEnt(pNewEnemy) && pBot.f_next_find_visible_sound_enemy_time <= gpGlobals->time)
   {
      // only run this 5fps
      pNewEnemy = BotFindVisibleSoundEnemy(pBot);
      pBot.f_next_find_visible_sound_enemy_time = gpGlobals->time + 0.2f;
      
      if(!FNullEnt(pNewEnemy))
      {
         //char msg[32];
         //safevoid_snprintf(msg, sizeof(msg), "Found sound enemy! %d", RANDOM_LONG2(0,0x7fffffff));
         //UTIL_HostSay(pEdict, 0, msg);
         
         is_sound_enemy = TRUE;

#if DEBUG_ENEMY_SELECT
         enemy_type = "sound-enemy";
#endif
      }
   }

   // 
   if (!FNullEnt(pNewEnemy))
   {
      // face the enemy
      Vector v_enemy = v_newenemy - pEdict->v.origin;
      Vector bot_angles = UTIL_VecToAngles( v_enemy );

      pEdict->v.ideal_yaw = bot_angles.y;

      BotFixIdealYaw(pEdict);

      // keep track of when we last saw an enemy
      pBot.f_bot_see_enemy_time = gpGlobals->time;
      pBot.v_bot_see_enemy_origin = UTIL_GetOrigin(pNewEnemy);

      BotResetReactionTime(pBot, is_sound_enemy);

#if DEBUG_ENEMY_SELECT
      if(pBot.waypoint_goal != -1)
         UTIL_ConsolePrintf("[%s] Found enemy, forget goal: %d -> %d", pBot.name, pBot.waypoint_goal, -1);

      UTIL_ConsolePrintf("[%s] Found enemy, type: %s", pBot.name, enemy_type);
#endif

      // clear goal waypoint
      pBot.waypoint_goal = -1;
      pBot.wpt_goal_type = WPT_GOAL_ENEMY;
   }

   // has the bot NOT seen an ememy for at least 5 seconds (time to reload)?
   else if ((pBot.f_bot_see_enemy_time > 0) && ((pBot.f_bot_see_enemy_time + 5.0) <= gpGlobals->time))
   {
      pBot.f_bot_see_enemy_time = -1;  // so we won't keep reloading
      pBot.v_bot_see_enemy_origin = Vector(-99999,-99999,-99999);

      pEdict->v.button |= IN_RELOAD;  // press reload button
   }
   
   pBot.pBotEnemy = pNewEnemy;
}


//
static qboolean HaveRoomForThrow(bot_t & pBot)
{
   edict_t *pEdict = pBot.pEdict;
   qboolean feet_ok, center_ok, head_ok;
      
   // trace from feet to feet and head to head
   TraceResult tr;
   Vector v_start, v_end;
      
   //feet
   v_start = pEdict->v.origin - pEdict->v.view_ofs;
   v_end = pBot.pBotEnemy->v.origin - pBot.pBotEnemy->v.view_ofs;
      
   UTIL_TraceMove(v_start, v_end, ignore_monsters, pEdict, &tr);
     
   feet_ok = (tr.flFraction > 0.999999f || tr.pHit == pBot.pBotEnemy);
      
   //center
   v_start = pEdict->v.origin;
   v_end = pBot.pBotEnemy->v.origin;
      
   UTIL_TraceMove(v_start, v_end, ignore_monsters, pEdict, &tr);
      
   center_ok = (tr.flFraction > 0.999999f || tr.pHit == pBot.pBotEnemy);
      
   if(center_ok && !feet_ok)
   {
      //head
      v_start = pEdict->v.origin + pEdict->v.view_ofs;
      v_end = pBot.pBotEnemy->v.origin + pBot.pBotEnemy->v.view_ofs;
      
      UTIL_TraceMove(v_start, v_end, ignore_monsters, pEdict, &tr);
         
      head_ok = (tr.flFraction > 0.999999f || tr.pHit == pBot.pBotEnemy);
   }
   else
      head_ok = FALSE;
      
   return((center_ok && feet_ok) || (center_ok && head_ok));
}


//
static qboolean CheckWeaponFireConditions(bot_t & pBot, const bot_weapon_select_t &select, qboolean &use_primary, qboolean &use_secondary) 
{
   edict_t *pEdict = pBot.pEdict;
   
   //
   if ((select.type & WEAPON_MELEE) == WEAPON_MELEE)
   {
      // check if bot needs to duck down to hit enemy...
      if (fabs(pBot.pBotEnemy->v.origin.z - pEdict->v.origin.z) > 16 &&
          (pBot.pBotEnemy->v.origin - pEdict->v.origin).Length() < 64)
         pBot.f_duck_time = gpGlobals->time + 1.0;
   }
   else if((select.type & WEAPON_THROW) == WEAPON_THROW && !HaveRoomForThrow(pBot))
   {
      return FALSE;
   }
   else if (select.iId == VALVE_WEAPON_MP5 && use_secondary)
   {
      qboolean ok = FALSE;
      
      if(HaveRoomForThrow(pBot))
      {
         // setup bot aim angle by distance and height to enemy
         Vector v_enemy = pBot.pBotEnemy->v.origin - (pEdict->v.origin + GetGunPosition(pEdict));
         
         float angle = ValveWeaponMP5_GetBestLaunchAngleByDistanceAndHeight(v_enemy.Length(), v_enemy.z);
         if(angle >= -89.0 && angle <= 89.0)
         {
            pBot.b_set_special_shoot_angle = TRUE;
            pBot.f_special_shoot_angle = angle;
            ok = TRUE;
         }
      }
      
      if(!ok)
         use_primary = !(use_secondary = FALSE);
   }
   
   return TRUE;
}


//
static qboolean BotFireSelectedWeapon(bot_t & pBot, const bot_weapon_select_t &select, const bot_fire_delay_t &delay, qboolean use_primary, qboolean use_secondary)
{
   edict_t *pEdict = pBot.pEdict;
   const int iId = select.iId;

   // Select primary or secondary based on bot skill and random percent
   if(use_primary && use_secondary)
   {
      BotSelectAttack(pBot, select, use_primary, use_secondary);
   }

   // Check if weapon can be fired and setup bot for firing this weapon
   if(!CheckWeaponFireConditions(pBot, select, use_primary, use_secondary))
      return FALSE;

   // use secondary once to enable zoom
   if(use_primary && (select.type & WEAPON_FIRE_ZOOM) == WEAPON_FIRE_ZOOM && pEdict->v.fov == 0)
      use_primary = !(use_secondary = TRUE);
   
   // use secondary once to enable aimspot
   if(use_primary && select.iId == GEARBOX_WEAPON_EAGLE && pBot.eagle_secondary_state == 0)
   {
      use_primary = !(use_secondary = TRUE);
      pBot.eagle_secondary_state = 1;
   }
   
   //duck for better aim
   if(select.iId == GEARBOX_WEAPON_M249)
   {
      pBot.f_duck_time = gpGlobals->time + 0.5f;
      pEdict->v.button |= IN_DUCK;
   }

   if (use_primary)
   {
      pEdict->v.button |= IN_ATTACK;  // use primary attack

      if (select.primary_fire_charge)
      {
         pBot.charging_weapon_id = iId;

         // release primary fire after the appropriate delay...
         pBot.f_primary_charging = gpGlobals->time +
                        select.primary_charge_delay;

         pBot.f_shoot_time = gpGlobals->time;  // keep charging
      }
      else
      {
         // set next time to shoot
         if (select.primary_fire_hold)
            pBot.f_shoot_time = gpGlobals->time;  // don't let button up
         else
         {
            int skill = pBot.bot_skill;
            float base_delay, min_delay, max_delay;

            base_delay = delay.primary_base_delay;
            min_delay = delay.primary_min_delay[skill];
            max_delay = delay.primary_max_delay[skill];
            
            if(min_delay == 0 && max_delay == 0)
               pBot.f_shoot_time = gpGlobals->time + base_delay;
            else
               pBot.f_shoot_time = gpGlobals->time + base_delay + RANDOM_FLOAT2(min_delay, max_delay);
         }
      }
   }
   else // MUST be use_secondary...
   {      
      pEdict->v.button |= IN_ATTACK2;  // use secondary attack

      if (select.secondary_fire_charge)
      {
         pBot.charging_weapon_id = iId;

         // release secondary fire after the appropriate delay...
         pBot.f_secondary_charging = gpGlobals->time +
                        select.secondary_charge_delay;

         pBot.f_shoot_time = gpGlobals->time;  // keep charging
      }
      else
      {
         // set next time to shoot
         if (select.secondary_fire_hold)
            pBot.f_shoot_time = gpGlobals->time;  // don't let button up
         else
         {
            int skill = pBot.bot_skill;
            float base_delay, min_delay, max_delay;

            base_delay = delay.secondary_base_delay;
            min_delay = delay.secondary_min_delay[skill];
            max_delay = delay.secondary_max_delay[skill];

            if(min_delay == 0 && max_delay == 0)
               pBot.f_shoot_time = gpGlobals->time + base_delay;
            else
               pBot.f_shoot_time = gpGlobals->time + base_delay + RANDOM_FLOAT2(min_delay, max_delay);
         }
      }
   }

   return TRUE;  // weapon was fired
}

//
static qboolean TrySelectWeapon(bot_t &pBot, const int select_index, const bot_weapon_select_t &select, const bot_fire_delay_t &delay)
{   
   // select this weapon if it isn't already selected
   if (pBot.current_weapon.iId != select.iId)
   {
      UTIL_SelectItem(pBot.pEdict, select.weapon_name);
   }
   
   if (delay.iId != select.iId)
   {
      pBot.current_weapon_index = -1;
      
      return FALSE;
   }
   
   pBot.current_weapon_index = select_index;
   pBot.f_weaponchange_time = gpGlobals->time + 
      RANDOM_FLOAT2(skill_settings[pBot.bot_skill].weaponchange_rate_min, 
                    skill_settings[pBot.bot_skill].weaponchange_rate_max);
   
   return TRUE;
}


// specifing a weapon_choice allows you to choose the weapon the bot will
// use (assuming enough ammo exists for that weapon)
// BotFireWeapon will return TRUE if weapon was fired, FALSE otherwise
static qboolean BotFireWeapon(const Vector & v_enemy, bot_t &pBot, int weapon_choice)
{
   bot_weapon_select_t *pSelect;
   bot_fire_delay_t *pDelay;
   int select_index, better_index;
   int iId;
   qboolean use_primary;
   qboolean use_secondary;
   float distance;
   float height;
   int min_skill;
   int min_index;
   qboolean min_use_primary;
   qboolean min_use_secondary;
   int avoid_skill;
   int avoid_index;
   qboolean avoid_use_primary;
   qboolean avoid_use_secondary;
   select_list_t * tmp_select_list;
   
   distance = v_enemy.Length();  // how far away is the enemy?
   height = v_enemy.z; // how high is enemy?

   pSelect = &weapon_select[0];
   pDelay = &fire_delay[0];

   // keep weapon only if can be used underwater
   if(pBot.b_in_water)
   {
      select_index = pBot.current_weapon_index;
      
      if(!pSelect[select_index].can_use_underwater)
      {
         pBot.current_weapon_index = -1;
         pBot.f_primary_charging = -1;
         pBot.f_secondary_charging = -1;
      }
   }
   
   // are we charging the primary fire?
   if (pBot.f_primary_charging > 0)
   {
      iId = pBot.charging_weapon_id;

      // is it time to fire the charged weapon?
      if (pBot.f_primary_charging <= gpGlobals->time)
      {
         // we DON'T set pEdict->v.button here to release the
         // fire button which will fire the charged weapon

         pBot.f_primary_charging = -1;  // -1 means not charging

         // find the correct fire delay for this weapon
         select_index = 0;

         while ((pSelect[select_index].iId) &&
                (pSelect[select_index].iId != iId))
            select_index++;

         // set next time to shoot
         int skill = pBot.bot_skill;
         float base_delay, min_delay, max_delay;

         base_delay = pDelay[select_index].primary_base_delay;
         min_delay = pDelay[select_index].primary_min_delay[skill];
         max_delay = pDelay[select_index].primary_max_delay[skill];

         pBot.f_shoot_time = gpGlobals->time + base_delay +
            RANDOM_FLOAT2(min_delay, max_delay);

         return TRUE;
      }
      else
      {
         pBot.pEdict->v.button |= IN_ATTACK;   // charge the weapon
         pBot.f_shoot_time = gpGlobals->time;  // keep charging

         return TRUE;
      }
   }

   // are we charging the secondary fire?
   if (pBot.f_secondary_charging > 0)
   {
      iId = pBot.charging_weapon_id;

      // is it time to fire the charged weapon?
      if (pBot.f_secondary_charging <= gpGlobals->time)
      {
         // we DON'T set pEdict->v.button here to release the
         // fire button which will fire the charged weapon

         pBot.f_secondary_charging = -1;  // -1 means not charging

         // find the correct fire delay for this weapon
         select_index = 0;

         while ((pSelect[select_index].iId) &&
                (pSelect[select_index].iId != iId))
            select_index++;

         // set next time to shoot
         int skill = pBot.bot_skill;
         float base_delay, min_delay, max_delay;

         base_delay = pDelay[select_index].secondary_base_delay;
         min_delay = pDelay[select_index].secondary_min_delay[skill];
         max_delay = pDelay[select_index].secondary_max_delay[skill];

         pBot.f_shoot_time = gpGlobals->time + base_delay +
            RANDOM_FLOAT2(min_delay, max_delay);

         return TRUE;
      }
      else
      {
         pBot.pEdict->v.button |= IN_ATTACK2;  // charge the weapon
         pBot.f_shoot_time = gpGlobals->time;  // keep charging

         return TRUE;
      }
   }
   
   // check if we can reuse currently active weapon
   if(pBot.current_weapon_index >= 0)
   {
      select_index = pBot.current_weapon_index;
      
      use_primary = FALSE;
      use_secondary = FALSE;
      
      // Check if we can use this weapon
      if ((weapon_choice == pSelect[select_index].iId || weapon_choice == 0) || 
          (IsValidWeaponChoose(pBot, pSelect[select_index]) && 
           BotCanUseWeapon(pBot, pSelect[select_index]) &&
           IsValidToFireAtTheMoment(pBot, pSelect[select_index])))
      {
         better_index = -1;
         
         if(weapon_choice == 0)
         {
            // Check if we REALLY want to change to other weapon (aka current gun IS shit)
            better_index = BotGetBetterWeaponChoice(pBot, pSelect[select_index], pSelect, distance, height, &use_primary, &use_secondary);
            if(better_index > -1) 
               select_index = better_index;
         }

         if(better_index == -1)
         {
            // Check if this weapon is ok for current contitions
            use_primary = IsValidPrimaryAttack(pBot, pSelect[select_index], distance, height, weapon_choice != 0);
            use_secondary = IsValidSecondaryAttack(pBot, pSelect[select_index], distance, height, weapon_choice != 0);
         }

         // check if bot has enough skill for attack
         if(use_primary && !BotSkilledEnoughForPrimaryAttack(pBot, pSelect[select_index]))
            use_primary = FALSE;
         if(use_secondary && !BotSkilledEnoughForSecondaryAttack(pBot, pSelect[select_index]))
            use_secondary = FALSE;
         
         if(use_primary || use_secondary)
         {
            if(!TrySelectWeapon(pBot, select_index, pSelect[select_index], pDelay[select_index]))
               return FALSE; //error

            return(BotFireSelectedWeapon(pBot, pSelect[select_index], pDelay[select_index], use_primary, use_secondary));
         }
      }
   }

   // don't change weapon too fast
   if(pBot.f_weaponchange_time >= gpGlobals->time && pBot.current_weapon_index >= 0 && weapon_choice == 0)
      return FALSE;

   pBot.current_weapon_index = -1; // forget current weapon

   //
   // 1. check which weapons are available and with which percents
   //
   int total_percent = 0;
   tmp_select_list = NULL;
   
   // loop through all the weapons until terminator is found...
   select_index = -1;
   while (pSelect[++select_index].iId)
   {
      // Check if we can use this weapon
      if(!(weapon_choice == pSelect[select_index].iId || weapon_choice == 0))
         continue;
 
      if(pSelect[select_index].avoid_this_gun)
         continue;

      if(!IsValidWeaponChoose(pBot, pSelect[select_index]) ||
         !BotCanUseWeapon(pBot, pSelect[select_index]) ||
         !IsValidToFireAtTheMoment(pBot, pSelect[select_index]))
         continue;

      // Check if this weapon is ok for current contitions
      use_primary = IsValidPrimaryAttack(pBot, pSelect[select_index], distance, height, weapon_choice != 0);
      use_secondary = IsValidSecondaryAttack(pBot, pSelect[select_index], distance, height, weapon_choice != 0);
      
      // check if bot has enough skill for attack
      if(use_primary && !BotSkilledEnoughForPrimaryAttack(pBot, pSelect[select_index]))
         use_primary = FALSE;
      if(use_secondary && !BotSkilledEnoughForSecondaryAttack(pBot, pSelect[select_index]))
         use_secondary = FALSE;

      if(!use_primary && !use_secondary)
         continue;
      
      // New code, collect weapon percents to linked list
      select_list_t *next = (select_list_t *)alloca(sizeof(select_list_t));
      next->next = NULL;
      
      // fill data
      next->use_primary = use_primary;
      next->use_secondary = use_secondary;
      next->percent_start = total_percent;
      next->percent_end = total_percent + pSelect[select_index].use_percent;
      next->select_index = select_index;
      
      //UTIL_ConsolePrintf("add: %d .. %d [%s]", total_percent, total_percent + pSelect[select_index].use_percent, pSelect[select_index].weapon_name);
      
      total_percent += pSelect[select_index].use_percent;
      
      // add to end of linked list
      select_list_t *prev = tmp_select_list;
      if(prev)
      {
         do {
            if(!prev->next)
            {
               prev->next = next;
               break;
            }
         } while((prev = prev->next) != NULL);
      }
      else
         tmp_select_list = next;
   }
   
   //
   // 2. Choose weapon
   //
   if(total_percent > 0)
   {
      // random choose
      int choose = RANDOM_LONG2(0, total_percent-1);
      
      // find choose
      select_list_t * next = tmp_select_list;
      while(next)
      {
         if(choose < next->percent_start || choose >= next->percent_end)
         {
            next = next->next;
            continue;
         }
         
         // this is our random choosed weapon!
         use_primary = next->use_primary;
         use_secondary = next->use_secondary;
         select_index = next->select_index;
         
         //UTIL_ConsolePrintf("choose: %d [%s]", choose, pSelect[select_index].weapon_name);
         
         if(!TrySelectWeapon(pBot, select_index, pSelect[select_index], pDelay[select_index]))
            return FALSE; //error

         return(BotFireSelectedWeapon(pBot, pSelect[select_index], pDelay[select_index], use_primary, use_secondary));
      }
   }

   // AT THIS POINT:
   // We didn't find good weapon, now try find least skilled weapon that bot has, but avoid avoidable weapons
   // Also get best avoidable weapon bot can use
   min_index = -1;
   min_skill = -1;
   min_use_primary = FALSE;
   min_use_secondary = FALSE;
   
   avoid_index = -1;
   avoid_skill = 6;
   avoid_use_primary = FALSE;
   avoid_use_secondary = FALSE;
   
   select_index = -1;
   while (pSelect[++select_index].iId)
   {         
      // Check if we can use this weapon
      if(!(weapon_choice == pSelect[select_index].iId || weapon_choice == 0))
         continue;

      if(!IsValidWeaponChoose(pBot, pSelect[select_index]))
         continue;
      
      // Underwater: only use avoidable weapon if can be used underwater
      if(pBot.b_in_water)
      {
         if(!pSelect[select_index].can_use_underwater)
            continue;
      }

      // Check if this weapon is ok for current contitions
      use_primary = IsValidPrimaryAttack(pBot, pSelect[select_index], distance, height, weapon_choice != 0);
      use_secondary = IsValidSecondaryAttack(pBot, pSelect[select_index], distance, height, weapon_choice != 0);
      if(use_primary || use_secondary)
      {
         if(pSelect[select_index].avoid_this_gun)
         {
            if(((use_primary && pSelect[select_index].primary_skill_level < avoid_skill) ||
               (use_secondary && pSelect[select_index].secondary_skill_level < avoid_skill)) &&
               BotCanUseWeapon(pBot, pSelect[select_index]))
            {
               avoid_skill = max(pSelect[select_index].primary_skill_level, pSelect[select_index].secondary_skill_level);
               avoid_index = select_index;
               avoid_use_primary = use_primary;
               avoid_use_secondary = use_secondary;
            }
         }
         else if((use_primary && pSelect[select_index].primary_skill_level > min_skill) || 
            (use_secondary && pSelect[select_index].secondary_skill_level > min_skill))
         {
            min_skill = max(pSelect[select_index].primary_skill_level, pSelect[select_index].secondary_skill_level);
            min_index = select_index;
            min_use_primary = use_primary;
            min_use_secondary = use_secondary;
         }
      }
   }
   
   if(min_index > -1 && min_skill > -1)
   {
      select_index = min_index;
      use_primary = min_use_primary;
      use_secondary = min_use_secondary;
      
      if(!TrySelectWeapon(pBot, select_index, pSelect[select_index], pDelay[select_index]))
         return FALSE; //error

      if(use_primary && use_secondary)
      {
         //use least skilled
         if(pSelect[select_index].primary_skill_level < pSelect[select_index].secondary_skill_level)
            use_primary = FALSE;
         else if(pSelect[select_index].primary_skill_level > pSelect[select_index].secondary_skill_level)
            use_secondary = FALSE;
      }

      return(BotFireSelectedWeapon(pBot, pSelect[select_index], pDelay[select_index], use_primary, use_secondary));
   }
   
   if(avoid_index > -1 && avoid_skill < 6)
   {
      select_index = avoid_index;
      use_primary = avoid_use_primary;
      use_secondary = avoid_use_secondary;
      
      if(!TrySelectWeapon(pBot, select_index, pSelect[select_index], pDelay[select_index]))
         return FALSE; //error

      // check if bot has enough skill for attack
      if(use_primary && !BotSkilledEnoughForPrimaryAttack(pBot, pSelect[select_index]))
         use_primary = FALSE;
      if(use_secondary && !BotSkilledEnoughForSecondaryAttack(pBot, pSelect[select_index]))
         use_secondary = FALSE;

      return(BotFireSelectedWeapon(pBot, pSelect[select_index], pDelay[select_index], use_primary, use_secondary));
   }

   // didn't have any available weapons or ammo, return FALSE
   return FALSE;
}


void BotShootAtEnemy( bot_t &pBot )
{
   float f_xy_distance;
   Vector v_enemy;
   Vector v_enemy_aimpos;
   Vector v_predicted_pos;
   
   edict_t *pEdict = pBot.pEdict;

   if (pBot.f_reaction_target_time > gpGlobals->time)
      return;

   v_predicted_pos = UTIL_AdjustOriginWithExtent(pBot, GetPredictedPlayerPosition(pBot, pBot.pBotEnemy), pBot.pBotEnemy);

   // do we need to aim at the feet?
   if (pBot.current_weapon_index != -1 && (weapon_select[pBot.current_weapon_index].type & WEAPON_FIRE_AT_FEET) == WEAPON_FIRE_AT_FEET)
   {
      Vector v_src, v_dest;
      TraceResult tr;

      v_src = pEdict->v.origin + pEdict->v.view_ofs;  // bot's eyes
      v_dest = pBot.pBotEnemy->v.origin - pBot.pBotEnemy->v.view_ofs;

      UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      // can the bot see the enemies feet?

      if ((tr.flFraction > 0.999999f) ||
          ((tr.flFraction >= 0.95f) &&
           FIsClassname("player", tr.pHit)))
      {
         // aim at the feet for RPG type weapons
         v_enemy_aimpos = v_predicted_pos - pBot.pBotEnemy->v.view_ofs;
      }
      else
      {
         v_enemy_aimpos = v_predicted_pos;
         if(FCanShootInHead(pEdict, pBot.pBotEnemy, v_predicted_pos))
            v_enemy_aimpos += pBot.pBotEnemy->v.view_ofs;// aim for the head...
      }
   }
   else
   {
      v_enemy_aimpos = v_predicted_pos;
      if(FCanShootInHead(pEdict, pBot.pBotEnemy, v_predicted_pos))
         v_enemy_aimpos += pBot.pBotEnemy->v.view_ofs;// aim for the head...
   }
   
   // Enemy not visible?
   if(!pBot.b_combat_longjump && !FVisibleEnemy(v_enemy_aimpos, pEdict, pBot.pBotEnemy))
   {
      BotRemoveEnemy(pBot, TRUE);
      
      return;
   }
   
   v_enemy = v_enemy_aimpos - GetGunPosition(pEdict);
   
   Vector enemy_angle = UTIL_VecToAngles( v_enemy );

   if (enemy_angle.x > 180)
      enemy_angle.x -=360;

   if (enemy_angle.y > 180)
      enemy_angle.y -=360;

   // adjust the view angle pitch to aim correctly
   enemy_angle.x = -enemy_angle.x;
   
   if(!pBot.b_combat_longjump)
   {
      pEdict->v.idealpitch = enemy_angle.x;
      BotFixIdealPitch(pEdict);

      pEdict->v.ideal_yaw = enemy_angle.y;
      BotFixIdealYaw(pEdict);
   }
   
   Vector v_enemy_xy = v_enemy;
   v_enemy_xy.z = 0;  // ignore z component (up & down)

   f_xy_distance = v_enemy_xy.Length();  // how far away is the enemy scum?

   if (f_xy_distance > 20)      // run if distance to enemy is far
      pBot.f_move_speed = pBot.f_max_speed;
   else                     // don't move too fast if close enough
   {
      pBot.b_not_maxspeed = TRUE;
      pBot.f_move_speed = pBot.f_max_speed/2;
   }
   
   // is it time to shoot yet?
   if (pBot.f_shoot_time <= gpGlobals->time)
   {
      // only fire if aiming target circle with specific max radius
      if(FVisibleEnemy(v_enemy_aimpos, pEdict, pBot.pBotEnemy)) 
      {
         float shootcone_diameter = skill_settings[pBot.bot_skill].shootcone_diameter;
         float shootcone_minangle = skill_settings[pBot.bot_skill].shootcone_minangle;
                  
         // check if it is possible to hit target
         if(FInShootCone(v_enemy_aimpos, pEdict, v_enemy.Length(), shootcone_diameter, shootcone_minangle)) 
         {
            // select the best weapon to use at this distance and fire...
            if(!BotFireWeapon(v_enemy, pBot, 0))
            {
               BotRemoveEnemy(pBot, TRUE);
               
               // level look
               pEdict->v.idealpitch = 0;
            }
         }
      } 
      else 
      {
         // not visible.. reset reaction times
         BotResetReactionTime(pBot);
      }
   }
}


qboolean BotShootTripmine( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;

   if (pBot.b_shoot_tripmine != TRUE)
      return FALSE;

   // aim at the tripmine and fire the glock...
   Vector v_enemy = pBot.v_tripmine - GetGunPosition( pEdict );
   Vector enemy_angle = UTIL_VecToAngles(v_enemy);

   pEdict->v.idealpitch = UTIL_WrapAngle(enemy_angle.x);
   pEdict->v.ideal_yaw = UTIL_WrapAngle(enemy_angle.y);
   
   //TODO: check if glock is available!!!!
   // if not try find another weapon which can do this (type: WEAPON_FIRE or FIRE_ZOOM).
   //TODO: or maybe throw grenade????
   return (BotFireWeapon( v_enemy, pBot, VALVE_WEAPON_GLOCK ));
}

