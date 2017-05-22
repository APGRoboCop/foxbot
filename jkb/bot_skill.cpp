//
// JK_Botti - be more human!
//
// bot_skill.cpp
//

#define BOTSKILL

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"
#include "bot_weapons.h"

#include "bot_skill.h"

#if 0
COPY+PASTE from bot_skill.h, for explaning values in following table:
typedef struct
{

0:
   int pause_frequency; // how often (out of 1000 times) the bot will pause, based on bot skill
   float pause_time_min; // how long bot pauses (min, max)
   float pause_time_max; 
   
   float normal_strafe; // how much bot strafes when walking around
   float battle_strafe; // how much bot strafes when attacking enemy
  
   int keep_optimal_dist; // how often bot (out of 100 times) the bot try to keep at optimum distance of weapon when attacking

6: 
   float respawn_react_delay; // delay on players after respawn
   float react_delay_min; // reaction delay settings (first is for bot_reaction 1, second for 2, etc)
   float react_delay_max;

9:
   float weaponchange_rate_min; // how fast changing weapons (min, max)
   float weaponchange_rate_max; 

11: 
   float shootcone_diameter; // bot tries to fire when aim line is less than [diameter / 2] apart from target 
   float shootcone_minangle; // OR angle between bot aim line and line to target is less than angle set here

13:   
   float turn_skill; // BotAim turn_skill, how good bot is at aiming on enemy origin.
   float turn_slowness; // Is bot's aim in slow motion?
   float updown_turn_ration; // how much slower bots aims up and down than side ways?

16:
   // Bot doesn't use real origin of target player but instead use ping emulation based on recorded old position data of player. 
   // These settings specify ammount of latency and randomness used at different skill levels.
   float ping_emu_latency; // ping emulation in seconds
   float ping_emu_speed_varitation; // percent
   float ping_emu_position_varitation; // units from target center

19:
   qboolean can_longjump; // and can longjump.
   
   int random_jump_frequency; // how often (out of 100 times) the bot will do random jump
   int random_jump_duck_frequency; // how often (out of 100 times) the bot will do random duck when random jumping
   int random_duck_frequency; // how often (out of 100 times) the bot will do random duck jumping in combat mode
   int random_longjump_frequency; // how often (out of 100 times) the bot will do random longjump instead of random jump

#if 0
24:
   qboolean can_taujump; // can tau jump? (waypoint taujump, attack/flee taujump)
   
   int attack_taujump_frequency; // how often (out of 100 times) the bot will do tau jump at far away enemy
   int flee_taujump_frequency; // how often (out of 100 times) the bot will taujump away from enemy
   
   float attack_taujump_distance; // how far enemy have to be to bot to use tau jump
   float flee_taujump_distance; // max distance to flee enemy from
   float flee_taujump_health; // how much bot has health left when tries to escape
   float flee_taujump_escape_distance; // how long way bot tries to move away

   qboolean can_shoot_through_walls; // can shoot through walls by sound
   int wallshoot_frequency; // how often (out of 100 times) the bot will try attack enemy behind wall
#endif

33:
   float hearing_sensitivity; // how well bot hears sounds
   float track_sound_time_min; // how long bot tracks one sound
   float track_sound_time_max;

} bot_skill_settings_t;
#endif

bot_skill_settings_t default_skill_settings[5] = {
   // best skill (lvl1)
   {
//0:
     1, 0.05, 0.30, 10.0, 50.0, 80, 
//6:
     0.6, 0.08, 0.12,
//9:
     0.1, 0.3,
//11:
     150.0, 12.5, 
//13:
     4.0, 1.0, 2.0,
//16:
     0.060, 0.03, 2.0, 
//19:
     TRUE, 50, 75, 50, 100,
//24:
     //TRUE, 100, 100, 1000.0, 400.0, 20.0, 1000.0,
     //TRUE, 99,
//33:
     1.5, 20.0, 40.0 },

   // lvl2
   {
//0:
     2, 0.10, 0.60, 9.5, 30.0, 60,
//6:
     0.8, 0.12, 0.18,
//9:
     0.2, 0.5,
//11:
     175.0, 20.0, 
//13:
     3.0, 1.25, 2.25,
//16:
     0.120, 0.04, 3.0, 
//19:
     TRUE, 35, 60, 35, 90,
//24:
     //TRUE, 50, 50, 1000.0, 400.0, 10.0, 1000.0,
     //TRUE, 66,
//33:
     1.25, 15.0, 30.0 },

   // lvl3
   {
//0:
     3, 0.15, 0.80, 8.0, 15.0, 40,
//6:
     1.0, 0.16, 0.24,
//9:
     0.3, 0.7,
//11:
     200.0, 25.0, 
//13:
     2.0, 1.5, 2.50,
//16:
     0.180, 0.05, 4.0, 
//19:
     TRUE, 20, 40, 20, 70,
//24:
     //TRUE, 20, 20, 1000.0, 400.0, 10.0, 1000.0,
     //TRUE, 33,
//33:
     1.0, 10.0, 20.0 },

   // lvl4
   {
//0:
     4, 0.20, 0.90, 7.5, 7.5, 25,
//6:
     1.20, 0.2, 0.3,
//9:
     0.6, 1.4,
//11:
     250.0, 30.0, 
//13:
     1.25, 1.75, 2.75,
//16:
     0.240, 0.075, 6.0, 
//19:
     TRUE, 10, 25, 10, 40,
//24:
     //TRUE, 0, 0, 0.0, 0.0, 0.0, 0.0,
     //FALSE, 0,
//33:
     0.75, 7.5, 15.0 },

   // worst skill (lvl5)
   {
//0:
     5, 0.25, 1.00, 7.0, 1.0, 15,
//6:
     1.4, 0.24, 0.36,
//9:
     1.2, 2.8,
//11:
     300.0, 35.0, 
//13:
     0.75, 2.0, 3.0,
//16:
     0.300, 0.10, 8.0, 
//19:
     FALSE, 5, 15, 5, 0,
//24:
     //FALSE, 0, 0, 0.0, 0.0, 0.0, 0.0,
     //FALSE, 0,
//33:
     0.5, 5.0, 10.0 },
};

bot_skill_settings_t skill_settings[5];

void ResetSkillsToDefault(void)
{
   memcpy(skill_settings, default_skill_settings, sizeof(skill_settings));
}

