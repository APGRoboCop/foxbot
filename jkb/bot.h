//
// JK_Botti - be more human!
//
// bot.h
//

#ifndef BOT_H
#define BOT_H

#include <ctype.h>
#include "safe_snprintf.h"

// Manual branch optimization for GCC 3.0.0 and newer
#if !defined(__GNUC__) || __GNUC__ < 3
	#define likely(x) (x)
	#define unlikely(x) (x)
#else
	#define likely(x) __builtin_expect((long int)!!(x), true)
	#define unlikely(x) __builtin_expect((long int)!!(x), false)
#endif


//
#define JKASSERT(_x_) \
    do { if (unlikely(!!(_x_))) UTIL_AssertConsolePrintf(#_x_, __FILE__, __LINE__); } while(0)


// stuff for Win32 vs. Linux builds

#ifdef __linux__
#define Sleep sleep
#define stricmp strcasecmp
#define strcmpi strcasecmp 
#define strnicmp strncasecmp
typedef int BOOL;
#endif


// for detecting submods
#define SUBMOD_HLDM 0
#define SUBMOD_SEVS 1
#define SUBMOD_BUBBLEMOD 2
#define SUBMOD_XDM 3
#define SUBMOD_OP4 4


// global defines
#define PLAYER_SEARCH_RADIUS (64 - 4.0)


// define a new bit flag for bot identification
#define FL_THIRDPARTYBOT (1 << 27)


// teamplay_gamerules.h
#define MAX_TEAMNAME_LENGTH     16
#define MAX_TEAMS               32
#define TEAMPLAY_TEAMLISTLENGTH (MAX_TEAMS*MAX_TEAMNAME_LENGTH)


#define LADDER_UNKNOWN  0
#define LADDER_UP       1
#define LADDER_DOWN     2

#define WANDER_LEFT  1
#define WANDER_RIGHT 2

#define RESPAWN_IDLE             1
#define RESPAWN_NEED_TO_RESPAWN  2
#define RESPAWN_IS_RESPAWNING    3

// waypoint goal types
#define WPT_GOAL_NONE		0
#define WPT_GOAL_HEALTH		1
#define WPT_GOAL_ARMOR		2
#define WPT_GOAL_WEAPON		3
#define WPT_GOAL_AMMO		4
#define WPT_GOAL_ITEM		5
#define WPT_GOAL_LOCATION	8
#define WPT_GOAL_TRACK_SOUND	9
#define WPT_GOAL_ENEMY		10

// instant damage (from cbase.h)
#define DMG_CRUSH			(1 << 0)	// crushed by falling or moving object
#define DMG_BURN			(1 << 3)	// heat burned
#define DMG_FREEZE			(1 << 4)	// frozen
#define DMG_FALL			(1 << 5)	// fell too far
#define DMG_SHOCK			(1 << 8)	// electric shock
#define DMG_DROWN			(1 << 14)	// Drowning
#define DMG_NERVEGAS		(1 << 16)	// nerve toxins, very bad
#define DMG_RADIATION		(1 << 18)	// radiation exposure
#define DMG_DROWNRECOVER	(1 << 19)	// drowning recovery
#define DMG_ACID			(1 << 20)	// toxic chemicals or acid burns
#define DMG_SLOWBURN		(1 << 21)	// in an oven
#define DMG_SLOWFREEZE		(1 << 22)	// in a subzero freezer


#define BOT_SKIN_LEN MAX_TEAMNAME_LENGTH 
#define BOT_NAME_LEN 32

#define MAX_BOT_CHAT 100


typedef struct
{
   int    index;
   char * skin;
   char * name;
   int    skill;
   int    top_color;
   int    bottom_color;
} cfg_bot_record_t;

typedef struct breakable_list_s 
{
   qboolean inuse;
   
   struct breakable_list_s * next;
   qboolean material_breakable;
   edict_t *pEdict;
} breakable_list_t;

typedef struct
{
   int  iId;     // weapon ID
   int  iClip;   // amount of ammo in the clip
   int  iAmmo1;  // amount of ammo in primary reserve
   int  iAmmo2;  // amount of ammo in secondary reserve
} bot_current_weapon_t;

typedef struct
{
   qboolean is_used;
   int userid;
   
   int cfg_bot_index;
   
   edict_t *pEdict;
   qboolean need_to_initialize;
   
   char name[BOT_NAME_LEN];
   char skin[BOT_SKIN_LEN];
   int bot_skill;
   int weapon_skill;
   
   float f_recoil;
   
   float f_kick_time;
   float f_create_time;
   float f_frame_time;
   float f_bot_spawn_time;
   
   int chat_percent;
   int taunt_percent;
   int whine_percent;
   int endgame_percent;
   int chat_tag_percent;
   int chat_drop_percent;
   int chat_swap_percent;
   int chat_lower_percent;

   double connect_time;
   double stay_time;
   
   qboolean b_on_ground;
   qboolean b_on_ladder;
   qboolean b_in_water;
   qboolean b_ducking;
   qboolean b_has_enough_ammo_for_good_weapon;
   qboolean b_low_health;
   
   int eagle_secondary_state;

   // things from pev in CBasePlayer...
   float idle_angle;
   float idle_angle_time;
   float blinded_time;

   float bot_think_time;
   float f_last_think_time;
   
   float msecdel;
   float msecval;

   float f_max_speed;
   float f_prev_speed;
   float f_speed_check_time;
   Vector v_prev_origin;

   float f_find_item;
   edict_t *pBotPickupItem;

   int ladder_dir;
   float f_start_use_ladder_time;
   float f_end_use_ladder_time;
   qboolean  waypoint_top_of_ladder;

   float f_wall_check_time;
   float f_wall_on_right;
   float f_wall_on_left;
   float f_dont_avoid_wall_time;
   float f_look_for_waypoint_time;
   float f_jump_time;
   float f_drop_check_time;

   int wander_dir;
   qboolean b_not_maxspeed;
   float f_move_direction;
   float f_strafe_direction;  // 0 = none, negative = left, positive = right
   float f_strafe_time;
   float f_exit_water_time;

   Vector waypoint_origin;
   float f_waypoint_time;
   int curr_waypoint_index;
   int prev_waypoint_index[5];
   float f_random_waypoint_time;
   int waypoint_goal;
   float f_waypoint_goal_time;
   float prev_waypoint_distance;
   int wpt_goal_type;
   edict_t *pTrackSoundEdict;        // used when wpt_goal_type == WPT_GOAL_TRACK_SOUND
   float f_track_sound_time;         // how long we track sound?

#define EXCLUDE_POINTS_COUNT 10
   int exclude_points[EXCLUDE_POINTS_COUNT+1];  // ten item locations + 1 null

   float f_last_item_found;

   edict_t *pBotEnemy;
   float f_bot_see_enemy_time;
   Vector v_bot_see_enemy_origin;
   float f_next_find_visible_sound_enemy_time;
 
   float f_last_time_attacked;

   edict_t *killer_edict;
   qboolean  b_bot_say;
   float f_bot_say;
   char  bot_say_msg[256];
   float f_bot_chat_time;
   qboolean b_bot_endgame;

   float f_duck_time;
   
   float f_random_jump_time;
   float f_random_jump_duck_time;
   float f_random_jump_duck_end;
   float f_random_duck_time;
   int prev_random_type;
   
   qboolean b_longjump;
   float f_combat_longjump;
   qboolean b_combat_longjump;
   float f_longjump_time;
   qboolean b_longjump_do_jump;
   
   float f_sniper_aim_time;
   
   float f_shoot_time;
   float f_primary_charging;
   float f_secondary_charging;
   int   charging_weapon_id;
   float f_grenade_search_time;
   float f_grenade_found_time;
   int current_weapon_index;
   float current_opt_distance;
   
   float f_move_speed;
   float f_pause_time;
   float f_pause_look_time;
   float f_sound_update_time;

   qboolean  b_see_tripmine;
   qboolean  b_shoot_tripmine;
   Vector v_tripmine;

   qboolean  b_use_health_station;
   float f_use_health_time;
   qboolean  b_use_HEV_station;
   float f_use_HEV_time;

   qboolean  b_use_button;
   float f_use_button_time;
   qboolean  b_lift_moving;
   
   Vector v_use_target;

   int   logo_percent;
   qboolean  b_spray_logo;
   float f_spray_logo_time;
   char  logo_name[16];
   int   top_color;
   int   bottom_color;

   float f_reaction_target_time;  // time when enemy targeting starts
   
   float f_weaponchange_time;

   float f_current_hearing_sensitivity;
   
   qboolean b_set_special_shoot_angle;
   float f_special_shoot_angle;

   bot_current_weapon_t current_weapon;  // one current weapon for each bot
   int m_rgAmmo[MAX_AMMO_SLOTS];  // total ammo amounts (1 array for each bot)

// for counting msec effency
   float total_frame_time;
   float total_msecval;
   int total_counter;
} bot_t;

#define MAX_TEAMS 32
#define MAX_TEAMNAME_LENGTH 16

#define MAX_FLAGS  5

typedef struct {
   edict_t *edict;
   int  team_no;
} FLAG_S;

#define MAX_BACKPACKS 100

typedef struct {
   edict_t *edict;
   int  armor;   // 0=none
   int  health;  // 0=none
   int  ammo;    // 0=none
   int  team;    // 0=all, 1=Blue, 2=Red, 3=Yellow, 4=Green
} BACKPACK_S;


#define MAX_SKINS 200

typedef struct
{
   qboolean skin_used;
   char model_name[32];
   char bot_name[32];
} skin_t;

typedef struct
{
   qboolean can_modify;
   char text[81];
} bot_chat_t;

typedef struct
{
   char identification[4];		// should be WAD2 (or 2DAW) or WAD3 (or 3DAW)
   int  numlumps;
   int  infotableofs;
} wadinfo_t;

typedef struct
{
   int  filepos;
   int  disksize;
   int  size;					// uncompressed
   char type;
   char compression;
   char pad1, pad2;
   char name[16];				// must be null terminated
} lumpinfo_t;

extern bot_t bots[32];
extern int submod_id;
extern int m_spriteTexture;

//void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);

#include "util.h"
#include "bot_func.h"
#include "bot_inline_funcs.h"

#endif // BOT_H
