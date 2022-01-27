//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_job_think.h
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

#ifndef BOT_JOB_THINK_H
#define BOT_JOB_THINK_H

// list of job types that the bots can try to accomplish
enum known_job_types {
   job_none = -1,
   job_seek_waypoint, // used when the bot can't find a current waypoint
   job_get_unstuck,
   job_roam,        // very low priority job, added to job buffer if it's completely empty
   job_chat,        // stop momentarily and say something personal
   job_report,      // stop momentarily and report something to the bots team
   job_pickup_item, // backpacks, medikits, armor that the bot has seen
   job_pickup_flag, // high priority item pickup
   job_push_button,
   job_use_teleport,    // go to and hop on a teleporter entrance to test it or use it
   job_maintain_object, // engineers repairing/reloading sentry guns and dispensors
   job_build_sentry,
   job_build_dispenser,
   job_build_teleport, // works for both teleporter entrances and exits
   job_buff_ally,      // medic/engineer healing or repairing an ally
   job_escort_ally,
   job_call_medic, // call a recently seen Medic and wait for medical assistance to arrive
   job_get_health,
   job_get_armor,
   job_get_ammo,
   job_disguise,     // be stealthy whilst disguising
   job_feign_ambush, // feign death and wait for a victim to come by
   job_snipe,        // from a sniper waypoint
   job_guard_waypoint,
   job_defend_flag,     // guard a dropped flag that the enemy team wants to pick up
   job_get_flag,        // get flag from a flag waypoint
   job_capture_flag,    // must have a flag first
   job_harrass_defense, // attack enemy defense locations
   job_rocket_jump,
   job_concussion_jump,
   job_detpack_waypoint,
   job_pipetrap,          // set up and monitor a pipebomb trap
   job_investigate_area,  // go to a waypoint to see what's happening there
   job_pursue_enemy,      // especially if enemy has a flag or is a spy
   job_patrol_home,       // wander the bots home base looking for enemies
   job_spot_stimulus,     // quick job when a bot's attacker is not in the bots viewcone
   job_attack_breakable,  // destroy breakable crates, walls etc.
   job_attack_teleport,   // go destroy an enemy teleport the bot saw
   job_seek_backup,       // go back to a place where you last saw an ally
   job_avoid_enemy,       // go somewhere the bots current enemy can't see it
   job_avoid_area_damage, // avoid temporary threat entities such as grenades
   job_infected_attack,   // useful when infected, go berserk on enemy defenders
   job_bin_grenade,       // throw a primed grenade away(it's no longer needed)
   job_drown_recover,     // swim to surface when drowning
   job_melee_warrior,     // daft job for attacking enemies with melee weapons only
   job_graffiti_artist,   // go spray a wall nearby for the heck of it

   // The constant below is not a job type, but is used to keep track of
   // the number of job types in the list.
   // When adding a new job type to this list, make sure it appears above this.
   job_type_total
};

// a job should be removed from the buffer if the job's function returns this signal
#define JOB_TERMINATED (-1)

// a job function will return this if the job hasn't finished yet
#define JOB_UNDERWAY 1

// list of essential data for all known job types
typedef struct {
   int base_priority;
   char job_names[32]; // useful in debugging
} job_list_struct;

extern job_list_struct jl[job_type_total];

// function prototypes below /////////////
void BotResetJobBuffer(bot_t *pBot);

void BlacklistJob(bot_t *pBot, int jobType, float timeOut);

bool BufferContainsJobType(const bot_t *pBot, int JobType);

int BufferedJobIndex(const bot_t *pBot, int JobType);

job_struct *InitialiseNewJob(const bot_t *pBot, int newJobType);

bool SubmitNewJob(bot_t *pBot, int newJobType, job_struct *newJob);

void BotRunJobs(bot_t *pBot);

void BotJobThink(bot_t *pBot);

#endif // BOT_JOB_THINK_H
