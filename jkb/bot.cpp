//
// JK_Botti - be more human!
//
// bot.cpp
//

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
#include "waypoint.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "bot_config_init.h"

#include <sys/types.h>
#include <sys/stat.h>


extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;  // number of waypoints currently in use
extern int default_bot_skill;
extern int bot_add_level_tag;
extern int bot_chat_percent;
extern int bot_taunt_percent;
extern int bot_whine_percent;
extern int bot_endgame_percent;
extern int bot_logo_percent;
extern int bot_chat_tag_percent;
extern int bot_chat_drop_percent;
extern int bot_chat_swap_percent;
extern int bot_chat_lower_percent;
extern qboolean b_random_color;
extern qboolean debug_minmax;

extern qboolean g_in_intermission;

extern bot_chat_t bot_chat[MAX_BOT_CHAT];
extern bot_chat_t bot_whine[MAX_BOT_CHAT];
extern int bot_chat_count;
extern int bot_whine_count;
extern int recent_bot_chat[];
extern int recent_bot_whine[];
extern qboolean checked_teamplay;
extern qboolean is_team_play;
extern float bot_think_spf;
extern int team_balancetype;
extern char *team_blockedlist;

extern char g_team_list[TEAMPLAY_TEAMLISTLENGTH];
extern char g_team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
extern int g_team_scores[MAX_TEAMS];
extern int g_num_teams;
extern qboolean g_team_limit;

extern int number_skins;
extern skin_t bot_skins[MAX_SKINS];

bot_t bots[32];   // max of 32 bots in a game
qboolean b_observer_mode = FALSE;
qboolean b_botdontshoot = FALSE;


//
static qboolean BotLowHealth( bot_t &pBot )
{
   return(pBot.pEdict->v.health + 0.8f * pBot.pEdict->v.armorvalue < VALVE_MAX_NORMAL_HEALTH * 0.5f);
}


//
void BotKick(bot_t &pBot)
{
   char cmd[64];

   if(pBot.userid <= 0)  // user id filled in yet?
      pBot.userid = GETPLAYERUSERID(pBot.pEdict);
   
   JKASSERT(pBot.is_used == FALSE);
   JKASSERT(FNullEnt(pBot.pEdict));   
   JKASSERT(pBot.userid <= 0);
   
   safevoid_snprintf(cmd, sizeof(cmd), "kick # %d\n", pBot.userid);
   
   SERVER_COMMAND(cmd);  // kick the bot using 'kick # <userid>'
   SERVER_EXECUTE();
}


//
static void BotSpawnInit( bot_t &pBot )
{
   pBot.bot_think_time = -1.0;
   pBot.f_last_think_time = gpGlobals->time - 0.1;
   
   pBot.f_recoil = 0;
   
   pBot.v_prev_origin = Vector(9999.0, 9999.0, 9999.0);
   pBot.f_speed_check_time = gpGlobals->time;

   pBot.waypoint_origin = Vector(0, 0, 0);
   pBot.f_waypoint_time = 0.0;
   pBot.curr_waypoint_index = -1;
   pBot.prev_waypoint_index[0] = -1;
   pBot.prev_waypoint_index[1] = -1;
   pBot.prev_waypoint_index[2] = -1;
   pBot.prev_waypoint_index[3] = -1;
   pBot.prev_waypoint_index[4] = -1;

   pBot.f_random_waypoint_time = gpGlobals->time;
   pBot.waypoint_goal = -1;
   pBot.wpt_goal_type = WPT_GOAL_NONE;
   pBot.f_waypoint_goal_time = 0.0;
   pBot.prev_waypoint_distance = 0.0;
   pBot.f_last_item_found = 0.0;

   pBot.pTrackSoundEdict = NULL;
   pBot.f_track_sound_time = 0.0;
   
   memset(pBot.exclude_points, 0, sizeof(pBot.exclude_points));

   pBot.b_on_ground = 0;
   pBot.b_on_ladder = 0;
   pBot.b_in_water = 0;
   pBot.b_ducking = 0;
   pBot.b_has_enough_ammo_for_good_weapon = 0;
   pBot.b_low_health = 0;
   
   pBot.f_last_time_attacked = 0;
   
   pBot.eagle_secondary_state = 0;

   pBot.blinded_time = 0.0;
   pBot.f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");
   pBot.f_prev_speed = 0.0;  // fake "paused" since bot is NOT stuck
   pBot.f_find_item = 0.0;
   pBot.b_not_maxspeed = FALSE;

   pBot.ladder_dir = LADDER_UNKNOWN;
   pBot.f_start_use_ladder_time = 0.0;
   pBot.f_end_use_ladder_time = 0.0;
   pBot.waypoint_top_of_ladder = FALSE;

   pBot.f_wall_check_time = 0.0;
   pBot.f_wall_on_right = 0.0;
   pBot.f_wall_on_left = 0.0;
   pBot.f_dont_avoid_wall_time = 0.0;
   pBot.f_look_for_waypoint_time = 0.0;
   pBot.f_jump_time = 0.0;
   pBot.f_drop_check_time = 0.0;

   // pick a wander direction (50% of the time to the left, 50% to the right)
   pBot.wander_dir = (RANDOM_LONG2(1, 100) <= 50) ? WANDER_LEFT : WANDER_RIGHT;

   pBot.f_move_direction = 1;

   pBot.f_exit_water_time = 0.0;

   pBot.pBotEnemy = NULL;
   pBot.f_bot_see_enemy_time = gpGlobals->time;
   pBot.v_bot_see_enemy_origin = Vector(-99999,-99999,-99999);
   pBot.f_next_find_visible_sound_enemy_time = 0.0f;

   pBot.f_duck_time = 0.0;
   
   pBot.f_random_jump_time = 0.0;
   pBot.f_random_jump_duck_time = 0.0;
   pBot.f_random_jump_duck_end = 0.0;
   pBot.f_random_duck_time = 0.0;
   pBot.prev_random_type = 0;
   
   pBot.f_sniper_aim_time = 0.0;
      
   pBot.b_longjump_do_jump = FALSE;
   pBot.b_longjump = FALSE;
   pBot.f_combat_longjump = 0.0;
   pBot.f_longjump_time = 0.0;
   pBot.b_combat_longjump = FALSE;
   
   pBot.f_shoot_time = gpGlobals->time;
   pBot.f_primary_charging = -1.0;
   pBot.f_secondary_charging = -1.0;
   pBot.charging_weapon_id = 0;
   pBot.f_grenade_search_time = 0.0;
   pBot.f_grenade_found_time = 0.0;
   pBot.current_weapon_index = -1;
   pBot.current_opt_distance = 99999.0;

   pBot.f_pause_time = 0.0;
   pBot.f_sound_update_time = 0.0;

   pBot.b_see_tripmine = FALSE;
   pBot.b_shoot_tripmine = FALSE;
   pBot.v_tripmine = Vector(0,0,0);

   pBot.b_use_health_station = FALSE;
   pBot.f_use_health_time = 0.0;
   pBot.b_use_HEV_station = FALSE;
   pBot.f_use_HEV_time = 0.0;

   pBot.b_use_button = FALSE;
   pBot.f_use_button_time = 0;
   pBot.b_lift_moving = FALSE;
   
   pBot.v_use_target = Vector(0,0,0);

   pBot.b_spray_logo = FALSE;

   pBot.f_reaction_target_time = 0.0;

   pBot.b_set_special_shoot_angle = FALSE;
   pBot.f_special_shoot_angle = 0.0;

   pBot.f_weaponchange_time = 0.0;
   
   pBot.f_pause_look_time = 0.0;

   pBot.f_current_hearing_sensitivity = -1;

   memset(&(pBot.current_weapon), 0, sizeof(pBot.current_weapon));
   memset(&(pBot.m_rgAmmo), 0, sizeof(pBot.m_rgAmmo));
}


static void BotPickLogo(bot_t &pBot)
{
   qboolean used;
   int logo_index;
   int check_count;
   int index;

   pBot.logo_name[0] = 0;

   if (num_logos == 0)
      return;

   logo_index = RANDOM_LONG2(1, num_logos) - 1;  // zero based

   // check make sure this name isn't used
   used = TRUE;
   check_count = 0;

   while ((used) && (check_count < MAX_BOT_LOGOS))
   {
      used = FALSE;

      for (index=0; index < 32; index++)
      {
         if (bots[index].is_used)
         {
            if (strcmp(bots[index].logo_name, bot_logos[logo_index]) == 0)
            {
               used = TRUE;
            }
         }
      }

      if (used)
         logo_index++;

      if (logo_index == MAX_BOT_LOGOS)
         logo_index = 0;

      check_count++;
   }

   safe_strcopy(pBot.logo_name, sizeof(pBot.logo_name), bot_logos[logo_index]);
}


static void BotSprayLogo(bot_t &pBot)
{
   edict_t * pEntity = pBot.pEdict;
   char *logo_name = pBot.logo_name;
   
   int index;
   TraceResult pTrace;
   Vector v_src, v_dest;
   
   v_src = pEntity->v.origin + pEntity->v.view_ofs;
   v_dest = v_src + UTIL_AnglesToForward(pEntity->v.v_angle) * 80;
   UTIL_TraceMove( v_src, v_dest, ignore_monsters, pEntity->v.pContainingEntity, &pTrace );

   index = DECAL_INDEX(logo_name);

   if (index < 0)
      return;

   if ((pTrace.pHit) && (pTrace.flFraction < 1.0f))
   {
      if (pTrace.pHit->v.solid != SOLID_BSP)
         return;

      MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

      if (index > 255)
      {
         WRITE_BYTE( TE_WORLDDECALHIGH);
         index -= 256;
      }
      else
         WRITE_BYTE( TE_WORLDDECAL );

      WRITE_COORD( pTrace.vecEndPos.x );
      WRITE_COORD( pTrace.vecEndPos.y );
      WRITE_COORD( pTrace.vecEndPos.z );
      WRITE_BYTE( index );

      MESSAGE_END();

      EMIT_SOUND_DYN2(pEntity, CHAN_VOICE, "player/sprayer.wav", 1.0, ATTN_NORM, 0, 100);
   }
}


static void BotPickName( char *name_buffer, int sizeof_name_buffer )
{
   int name_index, index;
   qboolean used;
   edict_t *pPlayer;
   int attempts = 0;

   name_index = RANDOM_LONG2(1, number_names) - 1;  // zero based

   // check make sure this name isn't used
   used = TRUE;

   while (used)
   {
      used = FALSE;

      for (index = 1; index <= gpGlobals->maxClients; index++)
      {
         pPlayer = INDEXENT(index);

         if (pPlayer && !pPlayer->free && !FBitSet(pPlayer->v.flags, FL_PROXY))
         {
            const char * netname = STRING(pPlayer->v.netname);
            
            //check if bot name is [lvlX]name format...
            if (strncmp(netname, "[lvl", 4) == 0 && netname[4] >= '1' && netname[4] <= '5' && netname[5] == ']')
               netname += 6;
            
            if (strcmp(bot_names[name_index], netname) == 0)
            {
               used = TRUE;
               break;
            }
         }
      }

      if (used)
      {
         name_index++;

         if (name_index == number_names)
            name_index = 0;

         attempts++;

         if (attempts == number_names)
            used = FALSE;  // break out of loop even if already used
      }
   }

   safe_strcopy(name_buffer, sizeof_name_buffer, bot_names[name_index]);
}


//
static qboolean TeamInTeamBlockList(const char * teamname)
{
   if(!team_blockedlist)
      team_blockedlist = strdup("");
      
   // make a copy because strtok is destructive
   int slen = strlen(team_blockedlist);
   char * blocklist = (char*)alloca(slen+1);
   memcpy(blocklist, team_blockedlist, slen+1);
   
   char *pName = strtok( blocklist, ";" );
   while ( pName != NULL && *pName)
   {
      if(!stricmp(teamname, pName))
      {
         //UTIL_ConsolePrintf("in block list: %s", pName);
         return(TRUE);
      }

      pName = strtok( NULL, ";" );
   }
      
   return(FALSE);
}


//
char * GetSpecificTeam(char * teamstr, size_t slen, qboolean get_smallest, qboolean get_largest, qboolean only_count_bots)
{
   if(!!get_smallest + !!get_largest != 1)
      return(NULL);
   
   // get team
   int find_index = -1;
   int find_count = get_smallest ? 9999 : -1;
   
   for(int i = 0; i < MAX_TEAMS; i++)
   {         
      if(*g_team_names[i] && !TeamInTeamBlockList(g_team_names[i]))
      {
         int count = 0;
         char teamname[MAX_TEAMNAME_LENGTH];
            
         // collect player counts for team
         for(int j = 1; j <= gpGlobals->maxClients; j++)
         {
            edict_t * pClient = INDEXENT(j);
            
            // skip unactive clients
            if(!pClient || pClient->free || FNullEnt(pClient) || GETPLAYERUSERID(pClient) <= 0 || STRING(pClient->v.netname)[0] == 0)
               continue;
            
            // skip bots?
            if(only_count_bots && UTIL_GetBotIndex(pClient) != -1)
               continue;
            
            // match team
            if(!stricmp(UTIL_GetTeam(pClient, teamname, sizeof(teamname)), g_team_names[i]))
            {
               count++;
            }
         }
                        
         
         if(get_smallest)
         {
            // smaller?
            if(count < find_count)
            {
               find_count = count;
               find_index = i;
            }
         }
         else
         {
            // larger?
            if(count > find_count)
            {
               find_count = count;
               find_index = i;
            }
         }
      }
   }
      
   // got it?
   if(find_index != -1)
   {
      if(get_largest && find_count == 0)
         return(NULL);
      
      safe_strcopy(teamstr, slen, g_team_names[find_index]);
      return(teamstr);
   }
   
   return(NULL);
}


//
static int GetTeamIndex( const char *pTeamName )
{
   if ( pTeamName && *pTeamName != 0 )
   {
      // try to find existing team
      for ( int tm = 0; tm < g_num_teams; tm++ )
      {
         if ( !stricmp( g_team_names[tm], pTeamName ) )
            return tm;
      }
   }
   
   return -1;   // No match
}


//
static void RecountTeams(void)
{
   if(!is_team_play)
      return;
	
   // Construct teams list
   char teamlist[TEAMPLAY_TEAMLISTLENGTH];
   char *pName;

   // loop through all teams, recounting everything
   g_num_teams = 0;

   // Copy all of the teams from the teamlist
   // make a copy because strtok is destructive
   safe_strcopy(teamlist, sizeof(teamlist), g_team_list);
   
   pName = teamlist;
   pName = strtok( pName, ";" );
   while ( pName != NULL && *pName )
   {
      if ( GetTeamIndex( pName ) < 0 )
      {
         safe_strcopy(g_team_names[g_num_teams], sizeof(g_team_names[g_num_teams]), pName);
         g_num_teams++;
         
         if(g_num_teams == MAX_TEAMS)
            break;
      }
      
      pName = strtok( NULL, ";" );
   }

   if ( g_num_teams < 2 )
   {
      g_num_teams = 0;
      g_team_limit = FALSE;
   }
   
   // Sanity check
   memset( g_team_scores, 0, sizeof(g_team_scores) );

   // loop through all clients
   for ( int i = 1; i <= gpGlobals->maxClients; i++ )
   {
      edict_t * plr = INDEXENT(i);

      if(!plr || plr->free || FNullEnt(plr) || GETPLAYERUSERID(plr) <= 0 || STRING(plr->v.netname)[0] == 0)
         continue;
      
      char teamname[MAX_TEAMNAME_LENGTH];
      const char *pTeamName;
      
      pTeamName = UTIL_GetTeam(plr, teamname, sizeof(teamname));
      
      // try add to existing team
      int tm = GetTeamIndex( pTeamName );
      
      if ( tm < 0 ) // no team match found
      { 
         if ( !g_team_limit && g_num_teams < MAX_TEAMS)
         {
            // add to new team
            tm = g_num_teams;
            g_num_teams++;
            g_team_scores[tm] = 0;
            safe_strcopy(g_team_names[tm], sizeof(g_team_names[tm]), pTeamName);
         }
      }

      if ( tm >= 0 )
      {
         g_team_scores[tm] += (int)plr->v.frags;
      }
   }
}


//
void BotCheckTeamplay(void)
{
   float f_team_play = CVAR_GET_FLOAT("mp_teamplay");  // teamplay enabled?

   if (f_team_play > 0.0f)
      is_team_play = TRUE;
   else
      is_team_play = FALSE;

   checked_teamplay = TRUE;
   
   // get team list, exactly as in teamplay_gamerules.cpp
   if(is_team_play)
   {
      safe_strcopy(g_team_list, sizeof(g_team_list), CVAR_GET_STRING("mp_teamlist"));
      
      edict_t *pWorld = INDEXENT(0);
      if ( pWorld && pWorld->v.team )
      {
         if ( CVAR_GET_FLOAT("mp_teamoverride") != 0.0f )
         {
            const char *pTeamList = STRING(pWorld->v.team);
            if ( pTeamList && *pTeamList )
            {
               safe_strcopy(g_team_list, sizeof(g_team_list), pTeamList);
            }
         }
      }
      
      // Has the server set teams
      g_team_limit = ( *g_team_list != 0 );
      
      RecountTeams();
   }
   else
   {
      g_team_list[0] = 0;
      g_team_limit = FALSE;
      
      memset(g_team_names, 0, sizeof(g_team_names));
   }
}


// 
void BotCreate( const char *skin, const char *name, int skill, int top_color, int bottom_color, int cfg_bot_index )
{
   edict_t *BotEnt;
   char c_skin[BOT_SKIN_LEN];
   char c_name[BOT_NAME_LEN];
   char balanceskin[MAX_TEAMNAME_LENGTH];
   int index;
   int i, j, length;
   qboolean found = FALSE;
   qboolean got_skill_arg = FALSE;
   char c_topcolor[4], c_bottomcolor[4];
   int  max_skin_index;
   
   max_skin_index = number_skins;
   
   // balance teams, ignore input skin
   if(is_team_play && team_balancetype >= 1 && g_team_limit)
   {
      RecountTeams();
      
      // get smallest team
      if(GetSpecificTeam(balanceskin, sizeof(balanceskin), TRUE, FALSE, FALSE))
      {
         if (skin != NULL && skin[0] != '\0')
            UTIL_ConsolePrintf("[warning] Teambalance overriding input model '%s' with '%s'", skin, balanceskin);
         
         skin = balanceskin;
      }
   }
   
   if (skin == NULL || skin[0] == '\0')
   {
      index = RANDOM_LONG2(0, number_skins-1);

      // check if this skin has already been used...
      while (bot_skins[index].skin_used == TRUE)
      {
         index++;

         if (index == max_skin_index)
            index = 0;
      }

      bot_skins[index].skin_used = TRUE;

      // check if all skins are now used...
      for (i = 0; i < max_skin_index; i++)
      {
         if (bot_skins[i].skin_used == FALSE)
            break;
      }

      // if all skins are used, reset used to FALSE for next selection
      if (i == max_skin_index)
      {
         for (i = 0; i < max_skin_index; i++)
            bot_skins[i].skin_used = FALSE;
      }

      safe_strcopy(c_skin, sizeof(c_skin), bot_skins[index].model_name);
   }
   else
   {
      safe_strcopy(c_skin, sizeof(c_skin), skin);
   }

   for (i = 0; c_skin[i] != 0; i++)
      c_skin[i] = tolower( c_skin[i] );  // convert to all lowercase

   // check existance of player model file only on listenserver, dedicated server doesn't _need_ model.
   if(!IS_DEDICATED_SERVER())
   {
      index = 0;

      while ((!found) && (index < max_skin_index))
      {
         if (strcmp(c_skin, bot_skins[index].model_name) == 0)
            found = TRUE;
         else
            index++;
      }

      if (found == FALSE)
      {
         char dir_name[128];
         char filename[256];

         struct stat stat_str;

         GetGameDir(dir_name);

         safevoid_snprintf(filename, sizeof(filename), "%s/models/player/%s", dir_name, c_skin);

         if (stat(filename, &stat_str) != 0)
         {
            safevoid_snprintf(filename, sizeof(filename), "valve/models/player/%s", c_skin);

            if (stat(filename, &stat_str) != 0)
            {
               UTIL_ConsolePrintf("model \"%s\" is unknown.\n", c_skin );
               UTIL_ConsolePrintf("use barney, gina, gman, gordon, helmet, hgrunt,\n");
               UTIL_ConsolePrintf("    recon, robo, scientist, or zombie\n");
               return;
            }
         }
      }
   }
   
   if ((name != NULL) && (*name != 0))
   {
      safe_strcopy(c_name, sizeof(c_name), name);
   }
   else
   {
      if (number_names > 0)
         BotPickName( c_name, sizeof(c_name) );
      else
      {
         // copy the name of the model to the bot's name...
         safe_strcopy(c_name, sizeof(c_name), c_skin);
      }
   }
   
   if (skill >= 1 && skill <= 5)
   {
      got_skill_arg = TRUE;
   }
   if (skill < 1 || skill > 5)
   {
      skill = default_bot_skill;
      got_skill_arg = FALSE;
   }

   if ((top_color < 0) || (top_color > 255))
      top_color = RANDOM_LONG2(0, 255);
   if ((bottom_color < 0) || (bottom_color > 255))
      bottom_color = RANDOM_LONG2(0, 255);
   
   safevoid_snprintf(c_topcolor, sizeof(c_topcolor), "%d", top_color);
   safevoid_snprintf(c_bottomcolor, sizeof(c_bottomcolor), "%d", bottom_color);

   length = strlen(c_name);

   // remove any illegal characters from name...
   for (i = 0; i < length; i++)
   {
      if ((c_name[i] <= ' ') || (c_name[i] > '~') ||
          (c_name[i] == '"'))
      {
         for (j = i; j < length; j++)  // shuffle chars left (and null)
            c_name[j] = c_name[j+1];
         length--;
      }
   }
   
   //
   // Bug fix: remove "(1)" tags from name, HLDS adds "(1)" on duplicated names (and "(2), (3), ...")
   //
   if(c_name[0] == '(' && (c_name[1] >= '1' && c_name[1] <= '9') && c_name[2] == ')') 
   {
      length = strlen(&c_name[3]) + 1; // str+null
      for(i = 0; i < length; i++)
         c_name[i] = (&c_name[3])[i];
   }
   
   //
   // Bug fix: remove [lvlX] tags always
   //
   if(!strncmp(c_name, "[lvl", 4) && (c_name[4] >= '1' && c_name[4] <= '5') && c_name[5] == ']')
   {
      // Read skill from config file name
      if(!got_skill_arg)
      	 skill = c_name[4] - '0';
      
      length = strlen(&c_name[6]) + 1; // str+null
      for(i = 0; i < length; i++)
         c_name[i] = (&c_name[6])[i];
   }
   
   //
   if(bot_add_level_tag) 
   {
      char tmp[sizeof(c_name)];
      
      safevoid_snprintf(tmp, sizeof(tmp), "[lvl%d]%s", skill, c_name);
      tmp[sizeof(tmp)-1] = 0;  // make sure c_name is null terminated
      strcpy(c_name, tmp);
      c_name[sizeof(c_name)-1] = 0;  // make sure c_name is null terminated
   }
   
   if(UTIL_GetBotCount() < 32)
      BotEnt = (*g_engfuncs.pfnCreateFakeClient)( c_name );
   else
      BotEnt = NULL;

   if (FNullEnt( BotEnt ))
   {
      UTIL_ConsolePrintf("%s\n", "Max. Players reached. Can't create bot!");
   }
   else
   {
      char ptr[128];  // allocate space for message from ClientConnect
      char *infobuffer;
      int clientIndex;
      int index;

      UTIL_ConsolePrintf("Creating bot...\n");

      index = 0;
      while ((bots[index].is_used) && (index < 32))
         index++;

      if (index == 32)
      {
         UTIL_ConsolePrintf("Can't create bot!\n");
         return;
      }

      // create the player entity by calling MOD's player function
      // (from LINK_ENTITY_TO_CLASS for player object)

      CALL_GAME_ENTITY (PLID, "player", VARS(BotEnt));

      infobuffer = GET_INFOKEYBUFFER( BotEnt );
      clientIndex = ENTINDEX( BotEnt );

      // is this a MOD that supports model colors AND it's not teamplay?
      SET_CLIENT_KEYVALUE( clientIndex, infobuffer, "model", c_skin );

      if (top_color != -1)
         SET_CLIENT_KEYVALUE( clientIndex, infobuffer, "topcolor", c_topcolor );

      if (bottom_color != -1)
         SET_CLIENT_KEYVALUE( clientIndex, infobuffer, "bottomcolor", c_bottomcolor );

      // JK_Botti fix, call our own ClientConnect first.
      jkbotti_ClientConnect( BotEnt, c_name, "::::local:jk_botti", ptr );
      MDLL_ClientConnect( BotEnt, c_name, "127.0.0.1", ptr );

      // JK_Botti fix, call our own ClientPutInServer first.
      jkbotti_ClientPutInServer( BotEnt );
      MDLL_ClientPutInServer( BotEnt );

      BotEnt->v.flags |= FL_THIRDPARTYBOT | FL_FAKECLIENT;

      // initialize all the variables for this bot...

      bot_t &pBot = bots[index];
      memset(&pBot, 0, sizeof(pBot));

      pBot.is_used = TRUE;
      pBot.userid = 0;
      pBot.cfg_bot_index = cfg_bot_index;
      pBot.f_create_time = gpGlobals->time;
      pBot.name[0] = 0;  // name not set by server yet

      safe_strcopy(pBot.skin, sizeof(pBot.skin), c_skin);

      pBot.top_color = top_color;
      pBot.bottom_color = bottom_color;

      pBot.pEdict = BotEnt;

      BotPickLogo(pBot);

      BotSpawnInit(pBot);

      pBot.need_to_initialize = FALSE;  // don't need to initialize yet

      BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
      BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;

      // these should REALLY be MOD dependant...
      BotEnt->v.pitch_speed = 270;  // slightly faster than HLDM of 225
      BotEnt->v.yaw_speed = 250; // slightly faster than HLDM of 210

      pBot.idle_angle = 0.0;
      pBot.idle_angle_time = 0.0;

      pBot.chat_percent = bot_chat_percent;
      pBot.taunt_percent = bot_taunt_percent;
      pBot.whine_percent = bot_whine_percent;
      pBot.endgame_percent = bot_endgame_percent;
      pBot.chat_tag_percent = bot_chat_tag_percent;
      pBot.chat_drop_percent = bot_chat_drop_percent;
      pBot.chat_swap_percent = bot_chat_swap_percent;
      pBot.chat_lower_percent = bot_chat_lower_percent;
      pBot.logo_percent = bot_logo_percent;
      pBot.f_strafe_direction = 0.0;  // not strafing
      pBot.f_strafe_time = 0.0;

      pBot.bot_skill = skill - 1;  // 0 based for array indexes
      pBot.weapon_skill = skill;

      pBot.b_bot_say = FALSE;
      pBot.f_bot_say = 0.0;
      pBot.bot_say_msg[0] = 0;
      pBot.f_bot_chat_time = gpGlobals->time;
      pBot.b_bot_endgame = FALSE;
      
      // use system wide timer for connection times
      // bot will stay 30-160 minutes
      pBot.stay_time = 60 * (double)RANDOM_FLOAT2(30, 160);
      // bot has been already here for 10%-50% of the total stay time
      pBot.connect_time = UTIL_GetSecs() - pBot.stay_time * (double)RANDOM_FLOAT2(0.2, 0.8);
   }
}


//
void BotReplaceConnectionTime(const char * name, float * timeslot)
{
   for(int i = 0; i < 32; i++)
   {
      bot_t &pBot = bots[i];
      
      // find bot by name
      if(strcmp(pBot.name, name) != 0)
         continue;
      
      double current_time = UTIL_GetSecs();
      
      // check if stay time has been exceeded
      if(current_time - pBot.connect_time > pBot.stay_time || current_time - pBot.connect_time <= 0)
      {
         // use system wide timer for connection times
         // bot will stay 30-160 minutes
         pBot.stay_time = 60 * (double)RANDOM_FLOAT2(30, 160);
         // bot has been already here for 20%-80% of the total stay time
         pBot.connect_time = current_time - pBot.stay_time * (double)RANDOM_FLOAT2(0.2, 0.8);
      }
      
      *timeslot = (float)(current_time - pBot.connect_time);
      
      break;
   }
}


//
static int BotInFieldOfView(bot_t &pBot, const Vector & dest)
{
   // find angles from source to destination...
   Vector entity_angles = UTIL_VecToAngles( dest );

   // make yaw angle 0 to 360 degrees if negative...
   if (entity_angles.y < 0)
      entity_angles.y += 360;

   // get bot's current view angle...
   float view_angle = pBot.pEdict->v.v_angle.y;

   // make view angle 0 to 360 degrees if negative...
   if (view_angle < 0)
      view_angle += 360;

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead,  45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle

   // rsm - START angle bug fix
   int angle = abs((int)view_angle - (int)entity_angles.y);

   if (angle > 180)
      angle = 360 - angle;

   return angle;
   // rsm - END
}


static qboolean BotEntityIsVisible( bot_t &pBot, const Vector & dest )
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   UTIL_TraceLine( pBot.pEdict->v.origin + pBot.pEdict->v.view_ofs,
                   dest, ignore_monsters,
                   pBot.pEdict->v.pContainingEntity, &tr );

   // check if line of sight to object is not blocked (i.e. visible)
   if (tr.flFraction > 0.999999f)
      return TRUE;
   else
      return FALSE;
}


static void BotFindItem( bot_t &pBot )
{
   edict_t *pent = NULL;
   edict_t *pPickupEntity = NULL;
   Vector pickup_origin;
   Vector entity_origin;
   float radius = 500;
   qboolean can_pickup;
   float min_distance;
   char item_name[40];
   TraceResult tr;
   Vector vecStart;
   Vector vecEnd;
   int angle_to_entity;
   int itemflag, select_index;
   bot_weapon_select_t *pSelect = &weapon_select[0];
   edict_t *pEdict = pBot.pEdict;

   // forget about our item if it's been five seconds
   if (pBot.f_last_item_found > 0 && pBot.f_last_item_found + 5.0 < gpGlobals->time)
   {
      pBot.f_find_item = gpGlobals->time + 2.0;
      pBot.f_last_item_found = -1;
      pBot.pBotPickupItem = NULL;
   }

   // forget about item if it we picked it up
   if (pBot.pBotPickupItem && ((pBot.pBotPickupItem->v.effects & EF_NODRAW) ||
      !BotEntityIsVisible(pBot, UTIL_GetOriginWithExtent(pBot, pBot.pBotPickupItem))))
   {
      pBot.f_last_item_found = -1;
      pBot.pBotPickupItem = NULL;
   }

   // halt the rest of the function
   if (pBot.f_find_item > gpGlobals->time || pBot.pBotPickupItem)
      return;
   
   // try find again after 0.1sec
   pBot.f_find_item = gpGlobals->time + 0.1;

   // use a smaller search radius when waypoints are available
   if ((num_waypoints > 0) && (pBot.curr_waypoint_index != -1))
      radius = 320.0;
   else
      radius = 520.0;

   min_distance = radius + 1.0;

   while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, radius )) != NULL)
   {
      can_pickup = FALSE;  // assume can't use it until known otherwise

      safe_strcopy(item_name, sizeof(item_name), STRING(pent->v.classname));

      // see if this is a "func_" type of entity (func_button, etc.)...
      if (strncmp("func_", item_name, 5) == 0)
      {      	
         // BModels have 0,0,0 for origin so must use VecBModelOrigin...
         entity_origin = VecBModelOrigin(pent);

         vecStart = pEdict->v.origin + pEdict->v.view_ofs;
         vecEnd = entity_origin;

         angle_to_entity = BotInFieldOfView( pBot, vecEnd - vecStart );

         // check if entity is outside field of view (+/- 45 degrees)
         if (angle_to_entity > 45)
            continue;  // skip this item if bot can't "see" it

         // check if entity is a ladder (ladders are a special case)
         // DON'T search for ladders if there are waypoints in this level...
         if ((strcmp("func_ladder", item_name) == 0) && (num_waypoints == 0))
         {
            // force ladder origin to same z coordinate as bot since
            // the VecBModelOrigin is the center of the ladder.  For
            // LONG ladders, the center MAY be hundreds of units above
            // the bot.  Fake an origin at the same level as the bot...

            entity_origin.z = pEdict->v.origin.z;
            vecEnd = entity_origin;

            // trace a line from bot's eyes to func_ladder entity...
            UTIL_TraceMove( vecStart, vecEnd, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr);

            // check if traced all the way up to the entity (didn't hit wall)
            if (tr.flFraction > 0.999999f)
            {
               // find distance to item for later use...
               float distance = (vecEnd - vecStart).Length( );

               // use the ladder about 100% of the time, if haven't
               // used a ladder in at least 5 seconds...
               if ((RANDOM_LONG2(1, 100) <= 100) &&
                   ((pBot.f_end_use_ladder_time + 5.0) < gpGlobals->time))
               {
                  // if close to ladder...
                  if (distance < 100)
                  {
                     // don't avoid walls for a while
                     pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                  }

                  can_pickup = TRUE;
               }
            }
         }
         else
         {
            // trace a line from bot's eyes to func_ entity...
            UTIL_TraceMove( vecStart, vecEnd, dont_ignore_monsters, 
                            pEdict->v.pContainingEntity, &tr);

            // check if traced all the way up to the entity (didn't hit wall)

            //NOTE: disable check...
            // wall chargers can be used behind walls.. known bug in hldm
            // hlsdk/dlls/player.cpp - CBasePlayer::PlayerUse() -> "!!!UNDONE: traceline here to prevent USEing buttons through walls"
            //if(FIsClassname(item_name, tr.pHit))
            {
               // find distance to item for later use...
               float distance = (vecEnd - vecStart).Length( );

               // check if entity is wall mounted health charger...
               // check if the bot can use this item and
               // check if the recharger is ready to use (has power left)...
               if (strcmp("func_healthcharger", item_name) == 0 && 
               	   (pEdict->v.health < VALVE_MAX_NORMAL_HEALTH) && (pent->v.frame == 0))
               {
                  // check if flag not set...
                  if (!pBot.b_use_health_station)
                  {
                     // check if close enough and facing it directly...
                     if ((distance < PLAYER_SEARCH_RADIUS) &&
                         (angle_to_entity <= 10))
                     {
                        pBot.b_use_health_station = TRUE;
                        pBot.f_use_health_time = gpGlobals->time;
                        pBot.v_use_target = UTIL_GetOriginWithExtent(pBot, pent);
                     }

                     // if close to health station...
                     if (distance < 100)
                     {
                        // don't avoid walls for a while
                        pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                     }

                     can_pickup = TRUE;
                  }
                  
                  if(can_pickup)
                     goto endloop;
               }
               else
               {
                  // don't need or can't use this item...
                  pBot.b_use_health_station = FALSE;
               }

               // check if entity is wall mounted HEV charger...
               // check if the bot can use this item and
               // check if the recharger is ready to use (has power left)...
               if (strcmp("func_recharge", item_name) == 0 &&
               	   (pEdict->v.armorvalue < VALVE_MAX_NORMAL_BATTERY) && (pent->v.frame == 0))
               {
                  // check if flag not set and facing it...
                  if (!pBot.b_use_HEV_station)
                  {
                     // check if close enough and facing it directly...
                     if ((distance < PLAYER_SEARCH_RADIUS) &&
                         (angle_to_entity <= 10))
                     {
                        pBot.b_use_HEV_station = TRUE;
                        pBot.f_use_HEV_time = gpGlobals->time;
                        pBot.v_use_target = UTIL_GetOriginWithExtent(pBot, pent);
                     }

                     // if close to HEV recharger...
                     if (distance < 100)
                     {
                        // don't avoid walls for a while
                        pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                     }

                     can_pickup = TRUE;
                  }
                  
                  if(can_pickup)
                     goto endloop;
               }
               else
               {
                  // don't need or can't use this item...
                  pBot.b_use_HEV_station = FALSE;
               }

               // check if entity is a button...
               if (FIsClassname(item_name, tr.pHit) && strcmp("func_button", item_name) == 0)
               {               	  
                  // use the button about 100% of the time, if haven't
                  // used a button in at least 5 seconds...
                  if ((RANDOM_LONG2(1, 100) <= 100) &&
                      ((pBot.f_use_button_time + 5) < gpGlobals->time))
                  {
                     // check if flag not set and facing it...
                     if (!pBot.b_use_button)
                     {
                        // check if close enough and facing it directly...
                        if ((distance < PLAYER_SEARCH_RADIUS) &&
                            (angle_to_entity <= 10))
                        {
                           pBot.b_use_button = TRUE;
                           pBot.b_lift_moving = FALSE;
                           pBot.v_use_target = UTIL_GetOriginWithExtent(pBot, pent);
                           pBot.f_use_button_time = gpGlobals->time;
                        }

                        // if close to button...
                        if (distance < 100)
                        {
                           // don't avoid walls for a while
                           pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;
                        }

                        can_pickup = TRUE;
                     }
                     
                     if(can_pickup)
                        goto endloop;
                  }
                  else
                  {
                     // don't need or can't use this item...
                     pBot.b_use_button = FALSE;
                  }
               }
            }
         }
      }
      else  // everything else...
      {
         entity_origin = pent->v.origin;

         vecStart = pEdict->v.origin + pEdict->v.view_ofs;
         vecEnd = entity_origin;

         // find angles from bot origin to entity...
         angle_to_entity = BotInFieldOfView( pBot, vecEnd - vecStart );

         // check if entity is outside field of view (+/- 45 degrees)
         if (angle_to_entity > 45)
            continue;  // skip this item if bot can't "see" it

         // check if line of sight to object is not blocked (i.e. visible)
         if (BotEntityIsVisible( pBot, vecEnd ))
         {
            // check if entity is a weapon...
            if ((itemflag = GetWeaponItemFlag(item_name)) != 0)
            {
               if (pent->v.effects & EF_NODRAW)
               {
                  // someone owns this weapon or it hasn't respawned yet
                  continue;
               }
               
               // check if player has already
               select_index = -1;
               qboolean found = FALSE;
               while(pSelect[++select_index].iId)
               {
                  if((itemflag & pSelect[select_index].waypoint_flag) == 0)
                     continue;
                  
                  found = TRUE;
                  break;
               }
               
               if(found)
               {
                  //does player have this weapon
                  if(pEdict->v.weapons & (1<<pSelect[select_index].iId))
                  {
                     // is ammo low?
                     if(BotPrimaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                     {
                        can_pickup = TRUE;
                     }
                  }
               }
               else
               {
                  can_pickup = TRUE;
               }
            }

            // pick up ammo
            else if ((itemflag = GetAmmoItemFlag(item_name)) != 0)
            {
               if (pent->v.effects & EF_NODRAW)
               {
                  // someone owns this weapon or it hasn't respawned yet
                  continue;
               }
               
               // check if player is running out of this ammo on any weapon
               select_index = -1;
               while(pSelect[++select_index].iId)
               {
                  if(itemflag & pSelect[select_index].ammo1_waypoint_flag)
                  {
                     if(BotPrimaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                     else if(BotSecondaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                  }
                  else if(itemflag & pSelect[select_index].ammo2_waypoint_flag)
                  {
                     if(BotPrimaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                     else if(BotSecondaryAmmoLow(pBot, pSelect[select_index]) == AMMO_LOW)
                        can_pickup = TRUE;
                  }
                  
                  if(can_pickup)
                  {
                     break;
                  }
               }
            }
            
            // pick up longjump
            else if (strcmp("item_longjump", item_name) == 0)
            {
               if (pent->v.effects & EF_NODRAW)
               {
                  // someone owns this weapon or it hasn't respawned yet
                  continue;
               }
               
               // check if the bot can use this item...
               if (!pBot.b_longjump && skill_settings[pBot.bot_skill].can_longjump)
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a battery...
            else if (strcmp("item_battery", item_name) == 0)
            {
               // check if the item is not visible (i.e. has not respawned)
               if (pent->v.effects & EF_NODRAW)
                  continue;

               // check if the bot can use this item...
               if (pEdict->v.armorvalue < VALVE_MAX_NORMAL_BATTERY)
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a healthkit...
            else if (strcmp("item_healthkit", item_name) == 0)
            {
               // check if the item is not visible (i.e. has not respawned)
               if (pent->v.effects & EF_NODRAW)
                  continue;

               // check if the bot can use this item...
               if (pEdict->v.health < VALVE_MAX_NORMAL_HEALTH)
               {
                  can_pickup = TRUE;
               }
            }

            // check if entity is a packed up weapons box...
            else if (strcmp("weaponbox", item_name) == 0)
            {
               can_pickup = TRUE;
            }

            // check if entity is the spot from RPG laser
            else if (strcmp("laser_spot", item_name) == 0)
            {
            }

            // check if entity is an armed tripmine
            else if (strcmp("monster_tripmine", item_name) == 0)
            {
               float distance = (pent->v.origin - pEdict->v.origin).Length( );

               if (pBot.b_see_tripmine)
               {
                  // see if this tripmine is closer to bot...
                  if (distance < (pBot.v_tripmine - pEdict->v.origin).Length())
                  {
                     pBot.v_tripmine = pent->v.origin;
                     pBot.b_shoot_tripmine = FALSE;

                     // see if bot is far enough to shoot the tripmine...
                     if (distance >= 375)
                     {
                        pBot.b_shoot_tripmine = TRUE;
                     }
                  }
               }
               else
               {
                  pBot.b_see_tripmine = TRUE;
                  pBot.v_tripmine = pent->v.origin;
                  pBot.b_shoot_tripmine = FALSE;

                  // see if bot is far enough to shoot the tripmine...
                  if (distance >= 375)  // 375 is damage radius
                  {
                     pBot.b_shoot_tripmine = TRUE;
                  }
               }
            }

            // check if entity is an armed satchel charge
            else if (strcmp("monster_satchel", item_name) == 0)
            {
            }

            // check if entity is a snark (squeak grenade)
            else if (strcmp("monster_snark", item_name) == 0)
            {
            }
            
         }  // end if object is visible
      }  // end else not "func_" entity

endloop:
      if (can_pickup) // if the bot found something it can pickup...
      {
         float distance = (entity_origin - pEdict->v.origin).Length( );

         // see if it's the closest item so far...
         if (distance < min_distance)
         {
            min_distance = distance;        // update the minimum distance
            pPickupEntity = pent;        // remember this entity
            pickup_origin = entity_origin;  // remember location of entity
         }
      }
   }  // end while loop

   if (pPickupEntity != NULL)
   {
      // let's head off toward that item...
      Vector v_item = pickup_origin - pEdict->v.origin;

      Vector bot_angles = UTIL_VecToAngles( v_item );

      pEdict->v.ideal_yaw = bot_angles.y;

      BotFixIdealYaw(pEdict);

      pBot.pBotPickupItem = pPickupEntity;  // save the item bot is trying to get
   }
}


static qboolean BotLookForGrenades( bot_t &pBot )
{
   static const char * grenade_names[] = {
      "grenade", "monster_satchel", /*"monster_snark",*/ "rpg_rocket", "hvr_rocket",
      NULL,
   };

   // not run in corners when have enemy
   if(pBot.pBotEnemy)
      return(FALSE);

   edict_t *pent;
   Vector entity_origin;
   float radius = 500;
   edict_t *pEdict = pBot.pEdict;

   pent = NULL;
   while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, radius )) != NULL)
   {
      entity_origin = UTIL_GetOriginWithExtent(pBot, pent);

      if (FInViewCone( entity_origin, pEdict ) && FVisible( entity_origin, pEdict, pent ))
      {
         const char ** p_grna = grenade_names;
          
         while(*p_grna) {
            if (FIsClassname(*p_grna, pent))
               return TRUE;
            p_grna++;
         }
      }
   }

   return FALSE;  // no grenades were found
}


static void HandleWallOnLeft( bot_t &pBot ) 
{
   edict_t *pEdict = pBot.pEdict;
        
   // if there was a wall on the left over 1/2 a second ago then
   // 20% of the time randomly turn between 45 and 60 degrees

   if ((pBot.f_wall_on_left != 0) &&
       (pBot.f_wall_on_left <= gpGlobals->time - 0.5) &&
       (RANDOM_LONG2(1, 100) <= 20))
   {
      pEdict->v.ideal_yaw += RANDOM_LONG2(45, 60);

      BotFixIdealYaw(pEdict);

      //pBot.f_move_speed /= 3;  // move slowly while turning
      pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;
   }

   pBot.f_wall_on_left = 0;  // reset wall detect time
}


static void HandleWallOnRight( bot_t &pBot ) 
{
   edict_t *pEdict = pBot.pEdict;
        
   // if there was a wall on the right over 1/2 a second ago then
   // 20% of the time randomly turn between 45 and 60 degrees

   if ((pBot.f_wall_on_right != 0) &&
       (pBot.f_wall_on_right <= gpGlobals->time - 0.5) &&
       (RANDOM_LONG2(1, 100) <= 20))
   {
      pEdict->v.ideal_yaw -= RANDOM_LONG2(45, 60);

      BotFixIdealYaw(pEdict);

      //pBot.f_move_speed /= 3;  // move slowly while turning
      pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;
   }

   pBot.f_wall_on_right = 0;  // reset wall detect time
}


//
static void BotCheckLogoSpraying(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;

   // took too long trying to spray logo?...
   if ((pBot.b_spray_logo) &&
       ((pBot.f_spray_logo_time + 3.0) < gpGlobals->time))
   {
      pBot.b_spray_logo = FALSE;
      pEdict->v.idealpitch = 0.0f;
   }

   if (pBot.b_spray_logo)  // trying to spray a logo?
   {
      Vector v_src, v_dest, angle;
      TraceResult tr;

      // find the nearest wall to spray logo on (or floor)...

      angle = pEdict->v.v_angle;
      angle.x = 0;  // pitch is level horizontally

      v_src = pEdict->v.origin + pEdict->v.view_ofs;
      v_dest = v_src + UTIL_AnglesToForward(angle) * 100;

      UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      if (tr.flFraction < 1.0f)
      {
         // already facing the correct yaw, just set pitch...
         pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
      }
      else
      {
         v_dest = v_src + gpGlobals->v_right * 100;  // to the right

         UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                         pEdict->v.pContainingEntity, &tr);

         if (tr.flFraction < 1.0f)
         {
            // set the ideal yaw and pitch...
            Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);

            pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(pEdict);

            pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
         }
         else
         {
            v_dest = v_src + gpGlobals->v_right * -100;  // to the left

            UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr);

            if (tr.flFraction < 1.0f)
            {
               // set the ideal yaw and pitch...
               Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);

               pEdict->v.ideal_yaw = bot_angles.y;

               BotFixIdealYaw(pEdict);

               pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
            }
            else
            {
               v_dest = v_src + gpGlobals->v_forward * -100;  // behind

               UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                               pEdict->v.pContainingEntity, &tr);

               if (tr.flFraction < 1.0f)
               {
                  // set the ideal yaw and pitch...
                  Vector bot_angles = UTIL_VecToAngles(v_dest - v_src);

                  pEdict->v.ideal_yaw = bot_angles.y;

                  BotFixIdealYaw(pEdict);

                  pEdict->v.idealpitch = RANDOM_FLOAT2(0.0, 30.0) - 15.0;
               }
               else
               {
                  // on the ground...

                  angle = pEdict->v.v_angle;
                  angle.x = 85.0f;  // 85 degrees is downward

                  v_src = pEdict->v.origin + pEdict->v.view_ofs;
                  v_dest = v_src + UTIL_AnglesToForward(angle) * 80;

                  UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                                  pEdict->v.pContainingEntity, &tr);

                  if (tr.flFraction < 1.0f)
                  {
                     // set the pitch...

                     pEdict->v.idealpitch = 85.0f;

                     BotFixIdealPitch(pEdict);
                  }
               }
            }
         }
      }

      pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;

      // is there a wall close to us?

      v_src = pEdict->v.origin + pEdict->v.view_ofs;
      v_dest = v_src + UTIL_AnglesToForward(pEdict->v.v_angle) * 80;

      UTIL_TraceMove( v_src, v_dest, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);

      if (tr.flFraction < 1.0f)
      {
         BotSprayLogo(pBot);

         pBot.b_spray_logo = FALSE;

         pEdict->v.idealpitch = 0.0f;
      }
   }

   if (!pBot.b_in_water &&   // is bot NOT under water?
       !pBot.b_spray_logo) // AND not trying to spray a logo
   {
      // reset pitch to 0 (level horizontally)
      pEdict->v.idealpitch = 0;
      pEdict->v.v_angle.x = 0;
   }
}


//
static void BotJustWanderAround(bot_t &pBot, float moved_distance)
{
   edict_t *pEdict = pBot.pEdict;

   // no enemy, let's just wander around...

   // logo spraying...
   BotCheckLogoSpraying(pBot);

   // Check if walking to edge.. jump for it!
   if(!FBitSet(pEdict->v.button, IN_DUCK) && !FBitSet(pEdict->v.button, IN_JUMP) && pBot.b_on_ground && !pBot.b_on_ladder &&
       !(pBot.curr_waypoint_index != -1 && waypoints[pBot.curr_waypoint_index].flags & (W_FL_LADDER | W_FL_LIFT_START | W_FL_LIFT_END)) &&
       pEdict->v.velocity.Length() > 50 && BotEdgeForward(pBot, pEdict->v.velocity))
   {
      pEdict->v.button |= IN_JUMP;
   }

   // check if bot should look for items now or not...
   if (pBot.f_find_item <= gpGlobals->time)
   {
      BotFindItem( pBot );  // see if there are any visible items
   }

   // check if bot sees a tripmine...
   if (pBot.b_see_tripmine)
   {
      // check if bot can shoot the tripmine...
      if ((pBot.b_shoot_tripmine) && BotShootTripmine( pBot ))
      {
         // shot at tripmine, may or may not have hit it, clear
         // flags anyway, next BotFindItem will see it again if
         // it is still there...

         pBot.b_shoot_tripmine = FALSE;
         pBot.b_see_tripmine = FALSE;

         // pause for a while to allow tripmine to explode...
         pBot.f_pause_time = gpGlobals->time + 0.5;
      }
      else  // run away!!!
      {
         Vector tripmine_angles;

         tripmine_angles = UTIL_VecToAngles( pBot.v_tripmine - pEdict->v.origin );

         // face away from the tripmine
         pEdict->v.ideal_yaw += 180;  // rotate 180 degrees

         BotFixIdealYaw(pEdict);

         pBot.b_see_tripmine = FALSE;

         pBot.f_move_speed = 0;  // don't run while turning
      }
   }

   // check if should use wall mounted health station...
   else if (pBot.b_use_health_station)
   {
      if ((pBot.f_use_health_time + 10.0) > gpGlobals->time)
      {
         pBot.f_move_speed = 0;  // don't move while using health station
         
         // block strafing before using health station
         pBot.f_strafe_time = gpGlobals->time + 1.0f;
         pBot.f_strafe_direction = 0.0f;
         
         // aim at the button..
         Vector v_target = pBot.v_use_target - GetGunPosition( pEdict );
         Vector target_angle = UTIL_VecToAngles(v_target);

         pEdict->v.idealpitch = UTIL_WrapAngle(target_angle.x);
         pEdict->v.ideal_yaw = UTIL_WrapAngle(target_angle.y);
         
         pBot.f_move_speed = pBot.f_max_speed;

         pEdict->v.button = IN_USE;
      }
      else
      {
         // bot is stuck trying to "use" a health station...

         pBot.b_use_health_station = FALSE;

         // don't look for items for a while since the bot
         // could be stuck trying to get to an item
         pBot.f_find_item = gpGlobals->time + 0.5;
      }
   }

   // check if should use wall mounted HEV station...
   else if (pBot.b_use_HEV_station)
   {
      if ((pBot.f_use_HEV_time + 10.0) > gpGlobals->time)
      {
         pBot.f_move_speed = 0;  // don't move while using HEV station
         
         // block strafing before using hev
         pBot.f_strafe_time = gpGlobals->time + 1.0f;
         pBot.f_strafe_direction = 0.0f;

         // aim at the button..
         Vector v_target = pBot.v_use_target - GetGunPosition( pEdict );
         Vector target_angle = UTIL_VecToAngles(v_target);

         pEdict->v.idealpitch = UTIL_WrapAngle(target_angle.x);
         pEdict->v.ideal_yaw = UTIL_WrapAngle(target_angle.y);
         
         pBot.f_move_speed = pBot.f_max_speed;

         pEdict->v.button = IN_USE;
      }
      else
      {
         // bot is stuck trying to "use" a HEV station...

         pBot.b_use_HEV_station = FALSE;

         // don't look for items for a while since the bot
         // could be stuck trying to get to an item
         pBot.f_find_item = gpGlobals->time + 0.5;
      }
   }

   else if (pBot.b_use_button)
   {
      pBot.f_move_speed = 0;  // don't move while using elevator
      
      // block strafing before using lift
      pBot.f_strafe_time = gpGlobals->time + 2.0f;
      pBot.f_strafe_direction = 0.0f;

      BotUseLift( pBot, moved_distance );
   }

   else
   {
      if (pBot.b_in_water)  // check if the bot is underwater...
      {
         BotUnderWater( pBot );
      }

      qboolean found_waypoint = FALSE;

      // if the bot is not trying to get to something AND
      // it is time to look for a waypoint AND
      // there are waypoints in this level...

      if ((pBot.pBotPickupItem == NULL) &&
          (pBot.f_look_for_waypoint_time <= gpGlobals->time) &&
          (num_waypoints != 0))
      {
         found_waypoint = BotHeadTowardWaypoint(pBot);
      }

      // check if the bot is on a ladder...
      if (pBot.b_on_ladder)
      {
         // check if bot JUST got on the ladder...
         if ((pBot.f_end_use_ladder_time + 1.0) < gpGlobals->time)
            pBot.f_start_use_ladder_time = gpGlobals->time;

         // go handle the ladder movement
         BotOnLadder( pBot, moved_distance );

         pBot.f_dont_avoid_wall_time = gpGlobals->time + 2.0;
         pBot.f_end_use_ladder_time = gpGlobals->time;
      }
      else
      {
         // check if the bot JUST got off the ladder...
         if ((pBot.f_end_use_ladder_time + 1.0) > gpGlobals->time)
         {
            pBot.ladder_dir = LADDER_UNKNOWN;
         }
      }

      // if the bot isn't headed toward a waypoint...
      if (found_waypoint == FALSE)
      {
         TraceResult tr;

         // check if we should be avoiding walls
         if (pBot.f_dont_avoid_wall_time <= gpGlobals->time)
         {
            // let's just randomly wander around
            if (BotStuckInCorner( pBot ))
            {
               pEdict->v.ideal_yaw += 180;  // turn 180 degrees

               BotFixIdealYaw(pEdict);

               pBot.f_move_speed /= 3;  // move slowly while turning
               pBot.b_not_maxspeed = TRUE;
               
               pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;

               moved_distance = 2.0;  // dont use bot stuck code
            }
            else
            {
               Vector v_forward = UTIL_AnglesToForward(pEdict->v.v_angle);
               
               if(RANDOM_LONG2(0,1)) 
               {
                  // check if there is a wall on the left first...
                  if (BotCheckWallOnLeft( pBot ) || BotEdgeLeft(pBot, v_forward))
                  {
                     HandleWallOnLeft(pBot);
                  }
                  else if (BotCheckWallOnRight( pBot ) || BotEdgeRight(pBot, v_forward))
                  {
                     HandleWallOnRight(pBot);
                  }
               }
               else 
               {
                  // check if there is a wall on the right first...
                  if (BotCheckWallOnRight( pBot ) || BotEdgeRight(pBot, v_forward))
                  {
                     HandleWallOnRight(pBot);
                  }
                  else if (BotCheckWallOnLeft( pBot ) || BotEdgeLeft(pBot, v_forward))
                  {
                     HandleWallOnLeft(pBot);
                  }
               }
            }
         }
         
         // check if bot is about to hit a wall.  TraceResult gets returned
         if ((pBot.f_dont_avoid_wall_time <= gpGlobals->time) &&
             BotCantMoveForward( pBot, &tr ))
         {
            // ADD LATER
            // need to check if bot can jump up or duck under here...
            // ADD LATER

            BotTurnAtWall( pBot, &tr, TRUE );
         }
      }

      // check if bot is on a ladder and has been on a ladder for
      // more than 5 seconds...
      if (pBot.b_on_ladder && (pBot.f_start_use_ladder_time > 0.0f) &&
          ((pBot.f_start_use_ladder_time + 5.0) <= gpGlobals->time))
      {
         // bot is stuck on a ladder...
         BotRandomTurn(pBot);

         // don't look for items for 2 seconds
         pBot.f_find_item = gpGlobals->time + 2.0;

         pBot.f_start_use_ladder_time = 0.0;  // reset start ladder time
      }
      
      // check if the bot hasn't moved much since the last location
      // and IS UNDERWATER
      if ((moved_distance <= 1.0f) && (pBot.f_prev_speed >= 1.0f) && pBot.b_in_water)
      {
         // the bot must be stuck!

         pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;
         pBot.f_look_for_waypoint_time = gpGlobals->time + 1.0;
         
         // try duck
         pEdict->v.button |= IN_DUCK;  // duck down and move forward
      }
      // check if the bot hasn't moved much since the last location
      // (and NOT on a ladder since ladder stuck handled elsewhere)
      else if ((moved_distance <= 1.0f) && (pBot.f_prev_speed >= 1.0f) && !pBot.b_on_ladder)
      {
         qboolean bCrouchJump = FALSE;
         
         // the bot must be stuck!

         pBot.f_dont_avoid_wall_time = gpGlobals->time + 1.0;
         pBot.f_look_for_waypoint_time = gpGlobals->time + 1.0;

         if (BotCanJumpUp( pBot, &bCrouchJump ))  // can the bot jump onto something?
         {
            if ((pBot.f_jump_time + 2.0) <= gpGlobals->time)
            {
               pBot.f_jump_time = gpGlobals->time;
               pEdict->v.button |= IN_JUMP;  // jump up and move forward

               if (bCrouchJump)
                  pEdict->v.button |= IN_DUCK;  // also need to duck
            }
            else
            {
               // bot already tried jumping less than two seconds ago, just turn
               BotRandomTurn(pBot);
            }
         }
         else if (BotCanDuckUnder( pBot ))  // can the bot duck under something?
         {
            pEdict->v.button |= IN_DUCK;  // duck down and move forward
         }
         else
         {
            BotRandomTurn(pBot);

            // is the bot trying to get to an item?...
            if (pBot.pBotPickupItem != NULL)
            {
               // don't look for items for a while since the bot
               // could be stuck trying to get to an item
               pBot.f_find_item = gpGlobals->time + 0.5;
            }
         }
      }

      // should the bot pause for a while here?
      // (don't pause on ladders or while being "used"...
      if (!pBot.b_on_ladder && (RANDOM_LONG2(1, 1000) <= skill_settings[pBot.bot_skill].pause_frequency))
      {
         // set the time that the bot will stop "pausing"
         pBot.f_pause_time = gpGlobals->time +
            RANDOM_FLOAT2(skill_settings[pBot.bot_skill].pause_time_min,
                         skill_settings[pBot.bot_skill].pause_time_max);
      }
   }
}


static void BotDoStrafe(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   
   // combat strafe
   if(pBot.pBotEnemy != NULL && FInViewCone(UTIL_GetOriginWithExtent(pBot, pBot.pBotEnemy), pEdict) && FVisibleEnemy( UTIL_GetOriginWithExtent(pBot, pBot.pBotEnemy), pEdict, pBot.pBotEnemy ))
   {
      // don't go too close to enemy
      // strafe instead
      if(RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].battle_strafe) 
      {
         if(pBot.f_strafe_time <= gpGlobals->time) 
         {
            pBot.f_strafe_time = gpGlobals->time + RANDOM_FLOAT2(0.5, 1.2);
            pBot.f_strafe_direction = (RANDOM_LONG2(1, 100) <= 50) ? 1.0 : -1.0;
            pBot.f_move_direction = (RANDOM_LONG2(1, 100) <= 50) ? 1.0 : -1.0;
         }
         
         pBot.f_move_speed = 1.0;
      }
      else
      {
         if(pBot.f_strafe_time <= gpGlobals->time) 
         {
            pBot.f_strafe_time = gpGlobals->time + RANDOM_FLOAT2(0.5, 1.2);
            pBot.f_strafe_direction = RANDOM_FLOAT(-0.3, 0.3);
            
            if(RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].keep_optimal_dist)
            {
               if((UTIL_GetOriginWithExtent(pBot, pBot.pBotEnemy) - pEdict->v.origin).Length() < pBot.current_opt_distance)
                  pBot.f_move_direction = -1;
               else
                  pBot.f_move_direction = 1;
            }
            else
               pBot.f_move_direction = (RANDOM_LONG2(1, 100) <= 50) ? 1.0 : -1.0;
         }
         
         pBot.f_move_speed = 32.0;
      }
      
      if(pBot.f_dont_avoid_wall_time <= gpGlobals->time)
      {
         Vector v_forward = UTIL_AnglesToForward(pEdict->v.v_angle);
         
         // check for walls, left and right
         if(pBot.f_strafe_direction > 0.0f)
         {
            if(BotCheckWallOnRight(pBot) || BotEdgeRight(pBot, v_forward))
               pBot.f_strafe_direction = -pBot.f_strafe_direction;
         }
         else if(pBot.f_strafe_direction < 0.0f)
         {
            if(BotCheckWallOnLeft(pBot) || BotEdgeLeft(pBot, v_forward))
               pBot.f_strafe_direction = -pBot.f_strafe_direction;
         }
      
         // check for walls, back and forward
         if(pBot.f_move_direction > 0.0f)
         {
            if(BotCheckWallOnForward(pBot))
               pBot.f_move_direction = -pBot.f_move_direction;
         }
         else if(pBot.f_move_direction < 0.0f)
         {
            if(BotCheckWallOnBack(pBot))
               pBot.f_move_direction = -pBot.f_move_direction;
         }
      }
      
      // set move direction
      pBot.f_move_speed *= pBot.f_move_direction;
   }
   // ladder strage - on ladder and know about ladder
   else if (pBot.b_on_ladder)
   {
      // do we have ladder waypoint?
      if(pBot.curr_waypoint_index != -1 && waypoints[pBot.curr_waypoint_index].flags & W_FL_LADDER)
      {
         if(pEdict->v.velocity.Make2D().Length() > 20.0f)
         {
      	    // First kill off sideways movement
      	    float move_dir = UTIL_VecToAngles(pEdict->v.velocity).y;
      	    float look_dir = pEdict->v.v_angle.y;
      	    
      	    float angle_diff = UTIL_WrapAngle(move_dir - look_dir);
      	    
      	    if(angle_diff > 135.0f || angle_diff <= 135.0f)
      	    {
      	       // move forward
      	       pBot.f_strafe_direction = 0.0;
      	       pBot.f_move_direction = 1.0;
      	    }
      	    if(angle_diff > 45.0f)
            {
               // strafe on right
               pBot.f_strafe_direction = -1.0;
               pBot.f_move_direction = 1.0;
            }
            else if(angle_diff <= -45.0f)
            {
               // strafe on left          
               pBot.f_strafe_direction = 1.0;
               pBot.f_move_direction = 1.0;
            }
            else
            {
               // move backwards
      	       pBot.f_strafe_direction = 0.0;
               pBot.f_move_direction = -1.0;
            }
      	 }
      	 else
      	 {
      	    // Then move towards waypoint sideways
            Vector v_wp = waypoints[pBot.curr_waypoint_index].origin;
            Vector v_dir = (v_wp - pEdict->v.origin);
         
            Vector v_angles = UTIL_VecToAngles(v_dir);
         
            float waypoint_dir = v_angles.y;
            float look_dir = pEdict->v.v_angle.y;
         
            float angle_diff = UTIL_WrapAngle(waypoint_dir - look_dir);
         
      	    if(angle_diff > 135.0f || angle_diff <= 135.0f)
      	    {
      	       // move backwards
      	       pBot.f_strafe_direction = 0.0;
      	       pBot.f_move_direction = -1.0;
      	    }
      	    if(angle_diff > 45.0f)
            {
               // strafe on left
               pBot.f_strafe_direction = 1.0;
               pBot.f_move_direction = 1.0;
            }
            else if(angle_diff <= -45.0f)
            {
               // strafe on right          
               pBot.f_strafe_direction = -1.0;
               pBot.f_move_direction = 1.0;
            }
            else
            {
               // move forward
      	       pBot.f_strafe_direction = 0.0;
               pBot.f_move_direction = 1.0;
            }
         }
      }
      else
      {
         // move forward
         pBot.f_strafe_direction = 0.0;
         pBot.f_move_direction = 1.0;
      }
      
      pBot.f_move_speed = 150.0f;
      pBot.b_not_maxspeed = TRUE;
      
      // set move direction
      pBot.f_move_speed *= pBot.f_move_direction;
   }
   // normal strafe - time to strafe yet?
   else if (pBot.f_strafe_time < gpGlobals->time)
   {
      pBot.f_strafe_time = gpGlobals->time + RANDOM_FLOAT2(0.1, 1.0);

      if (RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].normal_strafe)
      {
         if (RANDOM_LONG2(1, 100) <= 50)
            pBot.f_strafe_direction = -1.0;
         else
            pBot.f_strafe_direction = 1.0;
      }
      else
         pBot.f_strafe_direction = 0.0 + RANDOM_FLOAT2(-0.005, 0.005);

      if(pBot.f_dont_avoid_wall_time <= gpGlobals->time)
      {
         Vector v_forward = UTIL_AnglesToForward(pEdict->v.v_angle);
         
         // check for walls, left and right
         if(pBot.f_strafe_direction > 0.0f)
         {
            if(BotCheckWallOnRight(pBot) || BotEdgeRight(pBot, v_forward))
               pBot.f_strafe_direction = -pBot.f_strafe_direction;
         }
         else if(pBot.f_strafe_direction < 0.0f)
         {
            if(BotCheckWallOnLeft(pBot) || BotEdgeLeft(pBot, v_forward))
               pBot.f_strafe_direction = -pBot.f_strafe_direction;
         }
      }
   }
}


static void BotDoRandomJumpingAndDuckingAndLongJumping(bot_t &pBot, float moved_distance)
{
   edict_t *pEdict = pBot.pEdict;
   float max_lj_distance = 0.0, target_distance;

   // 
   // PART1 -- handle actions that are not finished
   // 

   // fix bot animation on longjumping
   if (pBot.b_longjump_do_jump)
   {
      pBot.b_longjump_do_jump = FALSE;
      pEdict->v.button |= IN_DUCK;
      pEdict->v.button |= IN_JUMP;
      
      pBot.f_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.3, 0.5); //stay duck for some time to fix animation
      
      return;
   }

   // duck in mid-air after random jump!
   if (pBot.f_random_jump_duck_time > 0.0f && pBot.f_random_jump_duck_time <= gpGlobals->time) 
   {
       pEdict->v.button |= IN_DUCK;  // need to duck (random duckjump)
       
       if(pBot.f_random_jump_duck_end > 0.0f && pBot.f_random_jump_duck_end <= gpGlobals->time) 
       {
          pBot.f_random_jump_duck_time = 0.0;
          pBot.f_random_jump_duck_end = 0.0;
          pBot.f_random_jump_time = gpGlobals->time + RANDOM_FLOAT2(0.2, 0.4);
       }
       
       return;
   }

   // stop trying to longjump after half a second
   if (pBot.f_combat_longjump < gpGlobals->time - 0.5 && pBot.b_combat_longjump)
      pBot.b_combat_longjump = FALSE;

   // longjump
   if (pBot.b_longjump && !pBot.b_in_water && pBot.b_on_ground &&
       !FBitSet(pEdict->v.button, IN_DUCK) && !FBitSet(pEdict->v.button, IN_JUMP) &&
       pEdict->v.velocity.Length() > 50 && pBot.b_combat_longjump &&
       fabs(pEdict->v.v_angle.y - pEdict->v.ideal_yaw) <= 10)
   {
      // don't try to move for 1.0 seconds, otherwise the longjump is fucked up	
      pBot.f_longjump_time = gpGlobals->time + 1.0;
      
      // we're done trying to do a longjump
      pBot.b_combat_longjump = FALSE;
      
      // don't do another one for a certain amount of time
      if (RANDOM_LONG2(1,100) > 10)
         pBot.f_combat_longjump = gpGlobals->time + 1.0;
      else
         pBot.f_combat_longjump = gpGlobals->time + RANDOM_FLOAT2(3.0, 5.0);
      
      // actually do the longjump
      pEdict->v.button |= IN_DUCK;
      pBot.b_longjump_do_jump = TRUE;
      
      //UTIL_ConsolePrintf("%s doing longjump! - combat\n", STRING(pEdict->v.netname));
      return;
   }

   //
   // PART2 -- return here if actions unfinished
   //
   
   // random jump or duck or longjump?
   if (pBot.f_random_jump_time > gpGlobals->time || 
       pBot.f_random_duck_time > gpGlobals->time ||
       pBot.f_combat_longjump > gpGlobals->time ||
       pBot.b_combat_longjump) 
   {
      return;
   }
   
   // Already jumping or ducking, or not on ground, or on ladder
   if(FBitSet(pEdict->v.button, IN_DUCK) || FBitSet(pEdict->v.button, IN_JUMP) ||
       !pBot.b_on_ground || pBot.b_on_ladder || pBot.b_in_water)
   {
      return;
   }
   
   //
   // PART3 -- start new actions, first check if contitions are right, then select, then act
   //
   
   // don't jump if trying to pick up item.
   if(pBot.pBotEnemy == NULL && !FNullEnt(pBot.pBotPickupItem) && !(pBot.pBotPickupItem->v.effects & EF_NODRAW) && !(pBot.pBotPickupItem->v.frame > 0))
      return;

   qboolean jump, duck, lj;
   
   // if in combat mode jump more
   jump = (pBot.prev_random_type != 1 && pBot.f_random_jump_time <= gpGlobals->time &&
           (moved_distance >= 10.0f || pBot.pBotEnemy != NULL) && pBot.f_move_speed > 1.0f &&
            RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_jump_frequency);
   
   duck = (pBot.prev_random_type != 2 && pBot.f_random_duck_time <= gpGlobals->time &&
           pBot.pBotEnemy != NULL && 
           RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_duck_frequency);
   
   lj = (pBot.prev_random_type != 3 && pBot.b_longjump && pBot.f_combat_longjump <= gpGlobals->time && !pBot.b_combat_longjump && 
         skill_settings[pBot.bot_skill].can_longjump &&
         pBot.pBotEnemy != NULL && fabs(pEdict->v.v_angle.y - pEdict->v.ideal_yaw) <= 30.0f &&
         RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_longjump_frequency);
   
   if(lj)
   {
      max_lj_distance = LONGJUMP_DISTANCE * (800 / CVAR_GET_FLOAT("sv_gravity"));
      target_distance = (UTIL_GetOriginWithExtent(pBot, pBot.pBotEnemy) - pEdict->v.origin).Length();
      
      lj = (target_distance > 128.0f && target_distance < max_lj_distance);
   }
   
   // ofcourse if we get multiple matches here we need to select
   if(lj)
   {
      // lj happens so rarely that it overrides others
      jump = FALSE;
      duck = FALSE;
   }
   else if(jump || duck)
   {
      if(jump && duck)
      {
         jump = RANDOM_FLOAT2(0, 100) <= 50.0f;
         duck = !jump;
      }
   }
   else if(!jump && !duck && !lj)
   {
      switch(pBot.prev_random_type)
      {
      case 1:
         pBot.f_random_jump_time = gpGlobals->time + RANDOM_FLOAT2(0.3,0.5);
         break;
      case 2:
         pBot.f_random_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.3,0.5);
         break;
      case 3:
         pBot.f_combat_longjump = gpGlobals->time + RANDOM_FLOAT2(0.3,0.5);
         break;
      default:
         break;
      }
      
      pBot.prev_random_type = 0;
      return;
   }
   
   // if in combat mode jump more
   if(jump)
   {
      pEdict->v.button |= IN_JUMP;
      
      pBot.prev_random_type = 1;
      //UTIL_ConsolePrintf("%s - Random jump\n", STRING(pEdict->v.netname));
      
      // duck mid-air?
      if(RANDOM_LONG2(1, 100) <= skill_settings[pBot.bot_skill].random_jump_duck_frequency)
      {
         //UTIL_ConsolePrintf("%s - Random duck-jump\n", STRING(pEdict->v.netname));
         pBot.f_random_jump_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.15, 0.25);
         pBot.f_random_jump_duck_end = pBot.f_random_jump_duck_time + RANDOM_FLOAT2(0.3, 0.5);
         pBot.f_random_jump_time = pBot.f_random_jump_duck_end + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
      }
      else
         pBot.f_random_jump_time = pBot.f_random_jump_duck_end + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
      
      return;
   }
   
   // combat mode random ducking
   if(duck)
   {
      pEdict->v.button |= IN_DUCK;
      
      pBot.prev_random_type = 2;
      //UTIL_ConsolePrintf("%s - Random duck\n", STRING(pEdict->v.netname));
      
      pBot.f_duck_time = gpGlobals->time + RANDOM_FLOAT2(0.3, 0.45);
      pBot.f_random_duck_time = pBot.f_duck_time + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
      
      return;
   }
   
   // combat mode random longjumping
   if(lj)
   {
      Vector vecSrc, target_angle;
      TraceResult tr;
      int mod;
         
      vecSrc = pEdict->v.origin;
      mod = RANDOM_LONG2(1, 100) <= 50 ? -1 : 1;
      
      // get a random angle (-30 or 30)
      for (int i = 1; i >= -1; i-=2)
      {
         target_angle.x = -pEdict->v.v_angle.x;
         target_angle.y = UTIL_WrapAngle(pEdict->v.v_angle.y + 30 * (mod * i));
         target_angle.z = pEdict->v.v_angle.z;
         
         // trace a hull toward the current waypoint the distance of a longjump (depending on gravity)
         UTIL_TraceMove(vecSrc, vecSrc + UTIL_AnglesToForward(target_angle) * max_lj_distance, dont_ignore_monsters, pEdict, &tr);
         
         // make sure it's clear
         if (tr.flFraction > 0.999999f)
         {
            //UTIL_ConsolePrintf("%s - Clear longjump path found\n", STRING(pEdict->v.netname));
            pBot.b_combat_longjump = TRUE;
            
            pEdict->v.ideal_yaw += 30 * mod;
            
            BotFixIdealYaw(pEdict);
            
            break;
         }
      }
      
      pBot.f_combat_longjump = gpGlobals->time + RANDOM_FLOAT2(0.3, 0.6); // don't try too often
      
      if(pBot.b_combat_longjump)
      {
         pBot.prev_random_type = 3;
         return;
      }
   }
}


static void BotRunPlayerMove(bot_t &pBot, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec)
{
   /*
      Calling sequence after calling g_engfuncs.pfnRunPlayerMove:
      
         1. CmdStart
         2. PlayerPreThink
         3. PM_Move
         4. PlayerPostThink
         5. CmdEnd
   */
   BotAimPre(pBot);
   g_engfuncs.pfnRunPlayerMove( pBot.pEdict, viewangles, forwardmove, sidemove, upmove, buttons, impulse, msec);
   BotAimPost(pBot);
}


void BotThink( bot_t &pBot )
{
   edict_t *pEdict = pBot.pEdict;
   
   Vector v_diff;             // vector from previous to current location
   float moved_distance;      // length of v_diff vector (distance bot moved)
   TraceResult tr;
   float f_strafe_speed;

   pEdict->v.flags |= FL_THIRDPARTYBOT | FL_FAKECLIENT;

   if(pBot.name[0] == 0)  // name filled in yet?
      safe_strcopy(pBot.name, sizeof(pBot.name), STRING(pEdict->v.netname));
   if(pBot.userid <= 0)  // user id filled in yet?
      pBot.userid = GETPLAYERUSERID(pEdict);

   // New code, BotThink is not run on every StartFrame anymore.
   pBot.f_frame_time = gpGlobals->time - pBot.f_last_think_time;
   pBot.f_last_think_time = gpGlobals->time;
  
   pBot.msecval = (int)(pBot.f_frame_time * 1000.0);
   
   // count up difference that integer conversion caused
   pBot.msecdel += pBot.f_frame_time * 1000.0 - pBot.msecval;
   
   // remove effect of integer conversion and lost msecs on previous frames
   if(pBot.msecdel > 1.625f)
   {
      float diff = 1.625f;
      
      if(pBot.msecdel > 60.0f)
         diff = 60.0f;
      else if(pBot.msecdel > 30.0f)
         diff = 30.0f;
      else if(pBot.msecdel > 15.0f)
         diff = 15.0f;
      else if(pBot.msecdel > 7.5f)
         diff = 7.5f;
      else if(pBot.msecdel > 3.25f)
         diff = 3.25f;
      
      pBot.msecval += diff - 0.5f;
      pBot.msecdel -= diff - 0.5f;
   }
   
   if (pBot.msecval < 1)    // don't allow msec to be less than 1...
   {      
      // adjust msecdel so we can correct lost msecs on following frames
      pBot.msecdel += pBot.msecval - 1;
      pBot.msecval = 1;
   }
   else if (pBot.msecval > 100)  // ...or greater than 100
   {
      // adjust msecdel so we can correct lost msecs on following frames
      pBot.msecdel += pBot.msecval - 100;
      pBot.msecval = 100;
   }
   
   pBot.total_msecval += pBot.msecval / 1000.0;
   pBot.total_frame_time += pBot.f_frame_time;

#if _DEBUG
   if(&pBot==&bots[0] && pBot.total_counter++ > 10)
   {
      UTIL_ConsolePrintf("total msecval count   : %9.4f", pBot.total_msecval);
      UTIL_ConsolePrintf("total frame time count: %9.4f", pBot.total_frame_time);
      UTIL_ConsolePrintf("total difference      : %9.4f", pBot.total_frame_time - pBot.total_msecval);
      pBot.total_counter=1;
   }
#endif
   
   pEdict->v.button = 0;
   pBot.f_move_speed = 0.0;

   // Get bot physics status
   pBot.b_on_ground = (pEdict->v.flags & FL_ONGROUND) == FL_ONGROUND;
   pBot.b_on_ladder = pEdict->v.movetype == MOVETYPE_FLY;
   pBot.b_in_water = pEdict->v.waterlevel == 2 || pEdict->v.waterlevel == 3;
   pBot.b_ducking = (pEdict->v.flags & FL_DUCKING) == FL_DUCKING;
   
   pBot.b_low_health = BotLowHealth(pBot);

   BotUpdateHearingSensitivity(pBot);

   // does bot need to say a message and time to say a message?
   if ((pBot.b_bot_say) && (pBot.f_bot_say < gpGlobals->time))
   {
      pBot.b_bot_say = FALSE;

      UTIL_HostSay(pEdict, 0, pBot.bot_say_msg);
   }
      
   // in intermission.. don't do anything, freeze bot
   if(g_in_intermission)
   {
      // endgame chat..
      if(!pBot.b_bot_endgame)
      {
         pBot.b_bot_endgame = TRUE;
         BotChatEndGame(pBot);
      }
      
      BotRunPlayerMove(pBot, pEdict->v.v_angle, 0, 0, 0, 0, 0, (byte)pBot.msecval);
      return;
   }

   // if the bot is dead, randomly press fire to respawn...
   if ((pEdict->v.health < 1) || (pEdict->v.deadflag != DEAD_NO))
   {
      if (pBot.need_to_initialize)
      {
         BotSpawnInit(pBot);

         pBot.need_to_initialize = FALSE;

         // whine if killed
         BotChatWhine(pBot);
      }

      if (RANDOM_LONG2(1, 100) <= 50)
         pEdict->v.button = IN_ATTACK;
            
      BotRunPlayerMove(pBot, pEdict->v.v_angle, pBot.f_move_speed, 0, 0, pEdict->v.button, 0, (byte)pBot.msecval);

      return;
   }
      
   // random chatting
   BotChatTalk(pBot);

   // set this for the next time the bot dies so it will initialize stuff
   if (pBot.need_to_initialize == FALSE)
   {
      pBot.need_to_initialize = TRUE;
      pBot.f_bot_spawn_time = gpGlobals->time;
   }

   // don't do anything while blinded
   if (pBot.blinded_time > gpGlobals->time)
   {
      if (pBot.idle_angle_time <= gpGlobals->time)
      {
         pBot.idle_angle_time = gpGlobals->time + RANDOM_FLOAT2(0.5, 2.0);

         pEdict->v.ideal_yaw = pBot.idle_angle + RANDOM_FLOAT2(0.0, 60.0) - 30.0;

         BotFixIdealYaw(pEdict);
      }

      // turn towards ideal_yaw by yaw_speed degrees (slower than normal)
      BotChangeYaw( pBot, pEdict->v.yaw_speed / 2 );
      
      BotRunPlayerMove(pBot, pEdict->v.v_angle, pBot.f_move_speed, 0, 0, pEdict->v.button, 0, (byte)pBot.msecval);

      return;
   }
   else
   {
      pBot.idle_angle = pEdict->v.v_angle.y;
   }

   // set to max speed
   pBot.f_move_speed = pBot.f_max_speed;
   pBot.b_not_maxspeed = FALSE;

   if (pBot.f_speed_check_time <= gpGlobals->time)
   {
      // see how far bot has moved since the previous position...
      v_diff = pBot.v_prev_origin - pEdict->v.origin;
      moved_distance = v_diff.Length();

      // save current position as previous
      pBot.v_prev_origin = pEdict->v.origin;
      pBot.f_speed_check_time = gpGlobals->time + 0.2;
   }
   else
   {
      moved_distance = 2.0;
   }

   // if the bot is not underwater AND not in the air (or on ladder),
   // check if the bot is about to fall off high ledge or into water...
   if (!pBot.b_in_water && pBot.b_on_ground && !pBot.b_on_ladder &&
       (pBot.f_drop_check_time < gpGlobals->time))
   {
      pBot.f_drop_check_time = gpGlobals->time + 0.05;

      BotLookForDrop( pBot );
   }

   // reset pause time if being attacked
   if((gpGlobals->time - pBot.f_last_time_attacked) < 1.0f)
      pBot.f_pause_time = 0;

   // turn towards ideal_pitch by pitch_speed degrees
   BotChangePitch( pBot, pEdict->v.pitch_speed );

   // turn towards ideal_yaw by yaw_speed degrees
   BotChangeYaw( pBot, pEdict->v.yaw_speed );
   
   // Only need to check ammo, since ammo check for weapons includes weapons ;)
   pBot.b_has_enough_ammo_for_good_weapon = !BotAllWeaponsRunningOutOfAmmo(pBot, TRUE);
   
   if(BotWeaponCanAttack(pBot, FALSE) && 
      ((pBot.b_has_enough_ammo_for_good_weapon && !pBot.b_low_health) || pBot.f_last_time_attacked < gpGlobals->time + 3.0f))
   {
      // get enemy
      BotFindEnemy( pBot );

      if(pBot.pBotEnemy == NULL)
      {      
         // if has zoom weapon and zooming, click off zoom
         if(pBot.current_weapon_index != -1 && 
            (weapon_select[pBot.current_weapon_index].type & WEAPON_FIRE_ZOOM) == WEAPON_FIRE_ZOOM && 
            pEdict->v.fov != 0 &&
            !(pEdict->v.button & (IN_ATTACK|IN_ATTACK2)))
         {
            pEdict->v.button |= IN_ATTACK2;
         }
         
         // if has aim spot weapon and have spot on, click spot off
         if(pBot.current_weapon_index != -1 && 
            weapon_select[pBot.current_weapon_index].iId == GEARBOX_WEAPON_EAGLE &&
            pBot.eagle_secondary_state != 0 &&
            !(pEdict->v.button & (IN_ATTACK|IN_ATTACK2)))
         {
            pEdict->v.button |= IN_ATTACK2;
            pBot.eagle_secondary_state = 0;
         }
      }
   }

   qboolean DidShootAtEnemy = FALSE;

   // does have an enemy?
   if (pBot.pBotEnemy != NULL)
   {
      if(BotWeaponCanAttack(pBot, FALSE) && (!pBot.b_low_health || pBot.f_last_time_attacked < gpGlobals->time + 3.0f))
      {
         BotShootAtEnemy( pBot );  // shoot at the enemy
         DidShootAtEnemy = (pBot.pBotEnemy != NULL);
         
         pBot.f_pause_time = 0;  // dont't pause if enemy exists
         
         // check if bot is on a ladder
         if (pBot.b_on_ladder)
         {
            // bot is stuck on a ladder... jump off ladder
            pEdict->v.button |= IN_JUMP;
         }
      }
      else
      {
         //
         pBot.f_pause_time = 0;
         
         // need waypoint away from enemy?
         edict_t *pAvoidEnemy = pBot.pBotEnemy;
         
         // don't have an enemy anymore so null out the pointer...
         BotRemoveEnemy(pBot, FALSE);
         
         int enemy_waypoint = WaypointFindNearest(pAvoidEnemy, 1024);
         int self_waypoint = WaypointFindNearest(pEdict, 1024);
      	 
         if(enemy_waypoint != -1 && self_waypoint != -1)
         {
            int runaway_waypoint = WaypointFindRunawayPath(self_waypoint, enemy_waypoint);
            
            //if(pBot.waypoint_goal != runaway_waypoint)
            //   UTIL_ConsolePrintf("[%s] Set runaway waypoint: %d -> %d", pBot.name, pBot.waypoint_goal, runaway_waypoint);
            
            pBot.wpt_goal_type = WPT_GOAL_LOCATION;
            pBot.curr_waypoint_index = self_waypoint;
            pBot.waypoint_goal = runaway_waypoint; 
            
            pBot.f_waypoint_goal_time = gpGlobals->time + 10.0f;
         }
      }
   }
   
   if(DidShootAtEnemy)
   {
      // do nothing
   }
   else if (pBot.f_pause_time > gpGlobals->time)  // is bot "paused"?
   {   	
      // you could make the bot look left then right, or look up
      // and down, to make it appear that the bot is hunting for
      // something (don't do anything right now)
      
      if(pBot.f_pause_look_time <= gpGlobals->time)
      {
         if(RANDOM_LONG2(1, 100) <= 50)
            pEdict->v.ideal_yaw += RANDOM_LONG2(30, 60);
         else
            pEdict->v.ideal_yaw -= RANDOM_LONG2(30, 60);
      
         if(pEdict->v.idealpitch > -30)
            pEdict->v.idealpitch -= RANDOM_LONG2(10, 30);
         else if(pEdict->v.idealpitch < 30)
            pEdict->v.idealpitch += RANDOM_LONG2(10, 30);
      
         BotFixIdealYaw(pEdict);
         BotFixIdealPitch(pEdict);
         
         pBot.f_pause_look_time = gpGlobals->time + RANDOM_FLOAT(0.5, 1.0);
      }
   }
   // next if lift-end and on moving platform
   else if(pBot.curr_waypoint_index != -1 && waypoints[pBot.curr_waypoint_index].flags & W_FL_LIFT_END &&
           !FNullEnt(pEdict->v.groundentity) && (!pEdict->v.groundentity->v.velocity.is_zero_vector()))
   {
      Vector v_to_wp = waypoints[pBot.curr_waypoint_index].origin - GetGunPosition( pEdict );
      
      Vector wp_angle = UTIL_VecToAngles(v_to_wp);
      
      pEdict->v.idealpitch = UTIL_WrapAngle(wp_angle.x);
      pEdict->v.ideal_yaw = UTIL_WrapAngle(wp_angle.y);
      
      pBot.f_move_speed = 50.0f;
      pBot.b_not_maxspeed = TRUE;
      
      pBot.f_strafe_direction = 0;
      pBot.f_strafe_time = gpGlobals->time + 0.2f;
   }
   else
   {
      BotJustWanderAround(pBot, moved_distance);
   }

   // does the bot have a waypoint?
   if (pBot.curr_waypoint_index != -1)
   {
      // check if the next waypoint is a door waypoint...
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_DOOR)
      {
         pBot.f_move_speed = pBot.f_max_speed / 3;  // slow down for doors
      }

      // check if the next waypoint is a ladder waypoint...
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_LADDER)
      {
         // check if the waypoint is at the top of a ladder AND
         // the bot isn't currenly on a ladder...
         if (pBot.waypoint_top_of_ladder && !pBot.b_on_ladder)
         {
            // is the bot on "ground" above the ladder?
            if (pBot.b_on_ground)
            {
               float waypoint_distance = (pEdict->v.origin - pBot.waypoint_origin).Length();

               if (waypoint_distance <= 20.0f)  // if VERY close...
                  pBot.f_move_speed = 20.0f;  // go VERY slow
               else if (waypoint_distance <= 100.0f)  // if fairly close...
                  pBot.f_move_speed = 50.0f;  // go fairly slow

               pBot.b_not_maxspeed = TRUE;

               pBot.ladder_dir = LADDER_DOWN;
            }
            else  // bot must be in mid-air, go BACKWARDS to touch ladder...
            {
               pBot.f_move_speed = -pBot.f_max_speed;
            }
         }
         else
         {
            // don't avoid walls for a while
            pBot.f_dont_avoid_wall_time = gpGlobals->time + 5.0;

            pBot.waypoint_top_of_ladder = FALSE;
         }
      }

      // check if the next waypoint is a crouch waypoint...
      if (waypoints[pBot.curr_waypoint_index].flags & W_FL_CROUCH)
      {
         // this might be duck-jump waypoint.. check if waypoint is at lower height than current bot origin
         if(waypoints[pBot.curr_waypoint_index].origin.z < pEdict->v.origin.z)
            pEdict->v.button |= IN_DUCK;  // duck down while moving forward
      }
   }
   
   if (pBot.f_grenade_search_time <= gpGlobals->time)
   {
      pBot.f_grenade_search_time = gpGlobals->time + 0.1;

      // does the bot see any grenades laying/flying about?
      if (BotLookForGrenades( pBot ))
         pBot.f_grenade_found_time = gpGlobals->time;
   }

   if (pBot.f_grenade_found_time + 1.0 > gpGlobals->time)
   {
      // move backwards for 1.0 second after seeing a grenade...
      pBot.f_move_speed = -pBot.f_move_speed;
      pBot.f_strafe_direction = RANDOM_FLOAT2(0, 1) * (pBot.f_strafe_direction>=0?1:-1);
      
      // disable normal strafe code until we have cleared grenade
      pBot.f_strafe_time = gpGlobals->time + 2.0f;
   }

   BotDoStrafe(pBot);

   if (pBot.f_duck_time > gpGlobals->time)
      pEdict->v.button |= IN_DUCK;  // need to duck (crowbar attack, and random combat ducking)

   // is the bot "paused"? or longjumping?
   if (pBot.f_pause_time > gpGlobals->time || pBot.f_longjump_time > gpGlobals->time) 
      pBot.f_move_speed = pBot.f_strafe_direction = 0;  // don't move while pausing
  
   BotDoRandomJumpingAndDuckingAndLongJumping(pBot, moved_distance);
  
   // save the previous speed (for checking if stuck)
   pBot.f_prev_speed = pBot.f_move_speed;
   
   // full strafe if bot isn't moving much else
   if(pBot.f_move_speed == 0.0f)
      f_strafe_speed = 0.0;
   else if(pBot.f_move_speed <= 20.0f)
      f_strafe_speed = pBot.f_strafe_direction * pBot.f_max_speed;
   else
      f_strafe_speed = pBot.f_strafe_direction * pBot.f_move_speed;
   
   // normalize strafe and move to keep speed at normal level
   if(f_strafe_speed != 0.0f || pBot.f_move_speed != 0.0f) 
   {
      Vector2D calc;
      
      calc.x = f_strafe_speed;
      calc.y = pBot.f_move_speed;
      
      calc = calc.Normalize() * (pBot.b_on_ladder || pBot.b_not_maxspeed ? pBot.f_move_speed : pBot.f_max_speed);
      
      f_strafe_speed = calc.x;
      pBot.f_move_speed = calc.y;
      
      if(0)
      {
         Vector v_angle = pEdict->v.v_angle;
         v_angle.x = 0;
         
         Vector v_right = UTIL_AnglesToRight(v_angle);
         Vector v_forward = UTIL_AnglesToForward(v_angle);
         
         Vector v_beam = v_forward * calc.y + v_right * calc.x;
         UTIL_DrawBeam(0, pEdict->v.origin, pEdict->v.origin + v_beam, 10, 2, 250, 50, 50, 200, 10);
      }
   }
   
   BotRunPlayerMove(pBot, pEdict->v.v_angle, pBot.f_move_speed,
                                f_strafe_speed, 0, pEdict->v.button, 0, (byte)pBot.msecval);
   
   return;
}

