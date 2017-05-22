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
enum knownJobTypes {
	JOB_NONE = -1,
	JOB_SEEK_WAYPOINT, // used when the bot can't find a current waypoint
	JOB_GET_UNSTUCK,
	JOB_ROAM,  // very low priority job, added to job buffer if it's completely empty
	JOB_CHAT,    // stop momentarily and say something personal
	JOB_REPORT,  // stop momentarily and report something to the bots team
	JOB_PICKUP_ITEM,  // backpacks, medikits, armor that the bot has seen
	JOB_PICKUP_FLAG,  // high priority item pickup
	JOB_PUSH_BUTTON,
	JOB_USE_TELEPORT,  // go to and hop on a teleporter entrance to test it or use it
	JOB_MAINTAIN_OBJECT,  // engineers repairing/reloading sentry guns and dispensors
	JOB_BUILD_SENTRY,
	JOB_BUILD_DISPENSER,
	JOB_BUILD_TELEPORT,  // works for both teleporter entrances and exits
	JOB_BUFF_ALLY,   // medic/engineer healing or repairing an ally
	JOB_ESCORT_ALLY,
	JOB_CALL_MEDIC, // call a recently seen Medic and wait for medical assistance to arrive
	JOB_GET_HEALTH,
	JOB_GET_ARMOR,
	JOB_GET_AMMO,
	JOB_DISGUISE, // be stealthy whilst disguising
	JOB_FEIGN_AMBUSH,  // feign death and wait for a victim to come by
	JOB_SNIPE,    // from a sniper waypoint
	JOB_GUARD_WAYPOINT,
	JOB_DEFEND_FLAG,  // guard a dropped flag that the enemy team wants to pick up
	JOB_GET_FLAG,       // get flag from a flag waypoint
	JOB_CAPTURE_FLAG,   // must have a flag first
	JOB_HARRASS_DEFENSE, // attack enemy defense locations
	JOB_ROCKET_JUMP,
	JOB_CONCUSSION_JUMP,
	JOB_DETPACK_WAYPOINT,
	JOB_PIPETRAP,  // set up and monitor a pipebomb trap
	JOB_INVESTIGATE_AREA,  // go to a waypoint to see what's happening there
	JOB_PURSUE_ENEMY,  // especially if enemy has a flag or is a spy
	JOB_PATROL_HOME,   // wander the bots home base looking for enemies
	JOB_SPOT_STIMULUS,  // quick job when a bot's attacker is not in the bots viewcone
	JOB_ATTACK_BREAKABLE,  // destroy breakable crates, walls etc.
	JOB_ATTACK_TELEPORT,  // go destroy an enemy teleport the bot saw
	JOB_SEEK_BACKUP,  // go back to a place where you last saw an ally
	JOB_AVOID_ENEMY,  // go somewhere the bots current enemy can't see it
	JOB_AVOID_AREA_DAMAGE,  // avoid temporary threat entities such as grenades
	JOB_INFECTED_ATTACK,  // useful when infected, go berserk on enemy defenders
	JOB_BIN_GRENADE,  // throw a primed grenade away(it's no longer needed)
	JOB_DROWN_RECOVER,  // swim to surface when drowning
	JOB_MELEE_WARRIOR,  // daft job for attacking enemies with melee weapons only
	JOB_GRAFFITI_ARTIST,  // go spray a wall nearby for the heck of it

	// The constant below is not a job type, but is used to keep track of
	// the number of job types in the list.
	// When adding a new job type to this list, make sure it appears above this.
	JOB_TYPE_TOTAL
};

// a job should be removed from the buffer if the job's function returns this signal
#define JOB_TERMINATED -1

// a job function will return this if the job hasn't finished yet
#define JOB_UNDERWAY 1


// list of essential data for all known job types
typedef struct {
	int basePriority;
	char jobNames[32];  // useful in debugging
	} jobList_struct;

extern const jobList_struct jl[JOB_TYPE_TOTAL];


// function prototypes below /////////////
void BotResetJobBuffer(bot_t *pBot);

void BlacklistJob(bot_t *pBot, const int jobType, const float timeOut);

bool BufferContainsJobType(const bot_t *pBot, const int JobType);

int BufferedJobIndex(const bot_t *pBot, const int JobType);

job_struct *InitialiseNewJob(bot_t *pBot, const int newJobType);

bool SubmitNewJob(bot_t *pBot, const int newJobType, job_struct *newJob);

void BotRunJobs(bot_t *pBot);

void BotJobThink(bot_t *pBot);

#endif // BOT_JOB_THINK_H
