//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_job_think.cpp
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
#include <meta_api.h>

#include "bot.h"
#include "waypoint.h"
#include "bot_func.h"

#include "bot_job_assessors.h"
#include "bot_job_functions.h"
#include "bot_job_think.h"
#include "bot_navigate.h"
#include "bot_weapons.h"

// team data /////////////////////////
extern int RoleStatus[];
extern int team_allies[4];
extern int max_team_players[4];
extern int team_class_limits[4];
extern int spawnAreaWP[4]; // used for tracking the areas where each team spawns
extern int max_teams;

extern bot_t bots[32];

extern bot_weapon_t weapon_defs[MAX_WEAPONS];

// bot settings //////////////////
extern bool bot_can_build_teleporter;
extern int bot_allow_humour;
extern bool offensive_chatter;
extern bool defensive_chatter;

extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints; // number of waypoints currently in use

// function prototypes //////////////
static void DropJobFromBuffer(bot_t *pBot, int buffer_index);
static void RefreshJobBuffer(bot_t *pBot);
static void BotEngineerThink(bot_t *pBot);

// structure for storing each job types function pointers
typedef struct {
   int (*assessFuncPtr)(const bot_t *pBot, const job_struct &r_Job);
   int (*jobFuncPtr)(bot_t *pBot);
} jobFunctions_struct;

// probably best to keep these function pointers private to this file
// these must be in the right order for each job to run properly
static jobFunctions_struct jf[job_type_total] = {
    {assess_JobSeekWaypoint, JobSeekWaypoint},
    {assess_JobGetUnstuck, JobGetUnstuck},
    {assess_JobRoam, JobRoam},
    {assess_JobChat, JobChat},
    {assess_JobReport, JobReport},
    {assess_JobPickUpItem, JobPickUpItem},
    {assess_JobPickUpFlag, JobPickUpFlag},
    {assess_JobPushButton, JobPushButton},
    {assess_JobUseTeleport, JobUseTeleport},
    {assess_JobMaintainObject, JobMaintainObject},
    {assess_JobBuildSentry, JobBuildSentry},
    {assess_JobBuildDispenser, JobBuildDispenser},
    {assess_JobBuildTeleport, JobBuildTeleport},
    {assess_JobBuffAlly, JobBuffAlly},
    {assess_JobEscortAlly, JobEscortAlly},
    {assess_JobCallMedic, JobCallMedic},
    {assess_JobGetHealth, JobGetHealth},
    {assess_JobGetArmor, JobGetArmor},
    {assess_JobGetAmmo, JobGetAmmo},
    {assess_JobDisguise, JobDisguise},
    {assess_JobFeignAmbush, JobFeignAmbush},
    {assess_JobSnipe, JobSnipe},
    {assess_JobGuardWaypoint, JobGuardWaypoint},
    {assess_JobDefendFlag, JobDefendFlag},
    {assess_JobGetFlag, JobGetFlag},
    {assess_JobCaptureFlag, JobCaptureFlag},
    {assess_JobHarrassDefense, JobHarrassDefense},
    {assess_JobRocketJump, JobRocketJump},
    {assess_JobConcussionJump, JobConcussionJump},
    {assess_JobDetpackWaypoint, JobDetpackWaypoint},
    {assess_JobPipetrap, JobPipetrap},
    {assess_JobInvestigateArea, JobInvestigateArea},
    {assess_JobPursueEnemy, JobPursueEnemy},
    {assess_JobPatrolHome, JobPatrolHome},
    {assess_JobSpotStimulus, JobSpotStimulus},
    {assess_JobAttackBreakable, JobAttackBreakable},
    {assess_JobAttackTeleport, JobAttackTeleport},
    {assess_JobSeekBackup, JobSeekBackup},
    {assess_JobAvoidEnemy, JobAvoidEnemy},
    {assess_JobAvoidAreaDamage, JobAvoidAreaDamage},
    {assess_JobInfectedAttack, JobInfectedAttack},
    {assess_JobBinGrenade, JobBinGrenade},
    {assess_JobDrownRecover, JobDrownRecover},
    {assess_JobMeleeWarrior, JobMeleeWarrior},
    {assess_JobGraffitiArtist, JobGraffitiArtist},
};

// list of essential data for all known job types
// these must be in the right order for each job to run properly
job_list_struct jl[job_type_total] = {   //Only handles 32 not 45? [APG]RoboCop[CL]
    {PRIORITY_MAXIMUM, {job_capture_flag}},
    {800, {job_get_unstuck}},
    {790, {job_maintain_object}},
    {780, {job_push_button}},
    {770, {job_build_sentry}},
    {750, {job_pickup_flag}},
    {720, {job_drown_recover}},
    {710, {job_bin_grenade}},
    {700, {job_avoid_area_damage}},
    {680, {job_get_flag}},
    {660, {job_concussion_jump}},
    {650, {job_spot_stimulus}},
    {640, {job_build_teleport}},
    {630, {job_use_teleport}},
    {620, {job_attack_breakable}},
    {610, {job_build_dispenser}},
    {600, {job_defend_flag}},
    {590, {job_snipe}},
    {580, {job_buff_ally}},
    {570, {job_disguise}},
    {560, {job_pursue_enemy}},
    {550, {job_escort_ally}},
    {540, {job_investigate_area}},
    {530, {job_infected_attack}},
    {520, {job_report}},
    {510, {job_get_ammo}},
    {500, {job_get_armor}},
    {490, {job_seek_backup}},
    {480, {job_call_medic}}, // this should be a higher priority than job_get_health
    {470, {job_get_health}},
    {460, {job_avoid_enemy}}, 
    {450, {job_pickup_item}}, // Bots appear to maybe not go beyond this line as the max is 32 tasks and could be out of range? [APG]RoboCop[CL]
    {440, {job_seek_waypoint}},
    {430, {job_attack_teleport}},
    {420, {job_pipetrap}},
    {410, {job_detpack_waypoint}},
    {400, {job_chat}},
    {350, {job_rocket_jump}}, // Reduced RJ until fix is provided [APG]RoboCop[CL]
    {300, {job_harrass_defense}},
    {250, {job_guard_waypoint}},
    {220, {job_melee_warrior}},
    {200, {job_patrol_home}},
    {150, {job_graffiti_artist}},
    {100, {job_feign_ambush}},
    {0, {job_roam}},
};

// This function clears the specified bots job buffer, and thus should
// probably be called when the bot is created.
void BotResetJobBuffer(bot_t *pBot) {
   int i;

   pBot->currentJob = 0;
   for (i = 0; i < JOB_BUFFER_MAX; i++) {
      DropJobFromBuffer(pBot, i);
   }

   // reset the job blacklist
   for (i = 0; i < JOB_BLACKLIST_MAX; i++) {
      pBot->jobBlacklist[i].type = job_none;
      pBot->jobBlacklist[i].f_timeOut = 0.0f;
   }
}

// This function eliminates whatever job is in the specified buffer
// index, and is the preferred method of terminating any job.
static void DropJobFromBuffer(bot_t *pBot, const int buffer_index) {
   pBot->jobType[buffer_index] = job_none;
   pBot->job[buffer_index].f_bufferedTime = 0.0f;
   pBot->job[buffer_index].priority = PRIORITY_NONE;
   pBot->job[buffer_index].phase = 0;
   pBot->job[buffer_index].phase_timer = 0.0f;
   pBot->job[buffer_index].waypoint = -1;
   pBot->job[buffer_index].waypointTwo = -1;
   pBot->job[buffer_index].player = nullptr;
   pBot->job[buffer_index].object = nullptr;
   pBot->job[buffer_index].message[0] = '\0';
}

// This function allows a job to be blacklisted(so that it cannot be added to the buffer)
// for a certain amount of time.  A job can be blacklisted because it keeps
// failing in a repetitive fashion.  e.g. Bot goes to repair a sentry gun but can't
// find a route to it from a certain position.
// timeOut should specify the number of seconds you want the job blacklisted for and
// should depend upon the type type of job that failed and the type of failure.
void BlacklistJob(bot_t *pBot, const int jobType, const float timeOut) {
   int shortestIndex = -1;
   float shortestTime = 9999999999999999.0f;

   for (int i = 0; i < JOB_BLACKLIST_MAX; i++) {
      if (pBot->jobBlacklist[i].f_timeOut < shortestTime) {
         shortestIndex = i;
         shortestTime = pBot->jobBlacklist[i].f_timeOut;
      }
   }

   if (shortestIndex != -1) {
      pBot->jobBlacklist[shortestIndex].type = jobType;
      pBot->jobBlacklist[shortestIndex].f_timeOut = pBot->f_think_time + timeOut;
   }
}

// This function returns true if the specified jobtype is already in the
// job buffer.
bool BufferContainsJobType(const bot_t *pBot, const int JobType) {
   for (int i = 0; i < JOB_BUFFER_MAX; i++) {
      if (pBot->jobType[i] == JobType)
         return true;
   }

   return false;
}

// If the specified job type is already in the job buffer this function will
// return the buffer index it occupies.
// Returns -1 if the job type is not in the buffer.
int BufferedJobIndex(const bot_t *pBot, const int JobType) {
   for (int i = 0; i < JOB_BUFFER_MAX; i++) {
      if (pBot->jobType[i] == JobType)
         return i;
   }

   return -1; // job isn't buffered
}

// This function serves two main purposes.
// 1. It returns NULL if the new job you would like to add to the job buffer
// is already in the buffer or has been blacklisted.
// 2. If the job isn't blacklisted or in the buffer already it will return a
// pointer to a common data storage area that you can use for the new job
// you wish to add.  Importantly it will clear all of the new job's data before
// giving you a pointer to it.
job_struct *InitialiseNewJob(const bot_t *pBot, const int newJobType) {
   int i;

   // abort if the job is in the job buffer already
   for (i = 0; i < JOB_BUFFER_MAX; i++) {
      if (pBot->jobType[i] == newJobType)
         return nullptr;
   }

   // abort if the job is currently blacklisted
   for (i = 0; i < JOB_BLACKLIST_MAX; i++) {
      if (newJobType == pBot->jobBlacklist[i].type && pBot->jobBlacklist[i].f_timeOut >= pBot->f_think_time)
         return nullptr;
   }

   static job_struct newJob;

   // reset the new job's data
   newJob.f_bufferedTime = pBot->f_think_time;
   newJob.priority = PRIORITY_NONE;
   newJob.phase = 0;
   newJob.phase_timer = 0.0f;
   newJob.waypoint = -1;
   newJob.waypointTwo = -1;
   newJob.player = nullptr;
   newJob.object = nullptr;
   newJob.message[0] = '\0';

   return &newJob;
}

// This function should be used when you wish to add a new job to the job buffer.
// It searches the job buffer for a free index or the lowest priority
// job that is less important than the one that you wish to add to the buffer.
// It returns true if the job was accepted.
bool SubmitNewJob(bot_t *pBot, const int newJobType, job_struct *newJob) {
   int i;

   if (pBot->current_wp == -1 && newJobType != job_seek_waypoint) // bit of a kludge but necessary
      return false;                                               // many job assessor functions need a valid current waypoint

   // if the job is currently blacklisted keep it out of the buffer
   for (i = 0; i < JOB_BLACKLIST_MAX; i++) {
      if (newJobType == pBot->jobBlacklist[i].type) {
         if (pBot->jobBlacklist[i].f_timeOut >= pBot->f_think_time)
            return false;
         else
            pBot->jobBlacklist[i].type = job_none; // job has timed out de-blacklist it
      }
   }

   // set some defaults for the new job
   newJob->f_bufferedTime = pBot->f_think_time;
   newJob->phase = 0;
   newJob->phase_timer = 0.0f;

   // assess the proposed new jobs validity and priority
   newJob->priority = jf[newJobType].assessFuncPtr(pBot, *newJob);
   if (newJob->priority == PRIORITY_NONE)
      return false;

   // look for the job with the lowest priority
   int worstJobFound = -1;
   int worstJobPriority = newJob->priority;
   for (i = 0; i < JOB_BUFFER_MAX; i++) {
      // found an unused job index?
      if (pBot->jobType[i] == job_none) {
         worstJobFound = i;
         worstJobPriority = PRIORITY_NONE;
         continue;
      }

      // currently, only one of each type of job is allowed in the buffer
      if (pBot->jobType[i] == newJobType)
         return false;

      // is this the lowest priority job found so far?
      if (pBot->job[i].priority < worstJobPriority) {
         worstJobFound = i;
         worstJobPriority = pBot->job[i].priority;
      }
   }

   // add the job to the buffer if it was important enough
   if (worstJobFound != -1) {
      /*	if(pBot->jobType[worstJobFound] != JOB_NONE)
                                      UTIL_BotLogPrintf("%s replacing job %s\n", pBot->name,
                                                      jl[pBot->jobType[worstJobFound]].jobNames);

                                      if(newJobType == JOB_ROCKET_JUMP)
                                                      UTIL_BotLogPrintf("%s adding JOB_ROCKET_JUMP\n", pBot->name); */

      pBot->jobType[worstJobFound] = newJobType;
      pBot->job[worstJobFound].priority = newJob->priority;
      pBot->job[worstJobFound].player = newJob->player;
      pBot->job[worstJobFound].object = newJob->object;
      pBot->job[worstJobFound].origin = newJob->origin;
      pBot->job[worstJobFound].waypoint = newJob->waypoint;
      pBot->job[worstJobFound].waypointTwo = newJob->waypointTwo;

      strncpy(pBot->job[worstJobFound].message, newJob->message, MAX_CHAT_LENGTH);
      pBot->job[worstJobFound].message[MAX_CHAT_LENGTH - 1] = '\0';

      pBot->job[worstJobFound].f_bufferedTime = pBot->f_think_time;
      pBot->job[worstJobFound].phase = 0;
      pBot->job[worstJobFound].phase_timer = 0.0f;
      return true;
   }

   return false;
}

// This function is reponsible for keeping the contents of the job buffer
// up to date.  For example, it can check the priority and validity of all
// jobs currently in the buffer.
static void RefreshJobBuffer(bot_t *pBot) {
   int i;

   int checkJob = random_long(0, JOB_BUFFER_MAX - 1);

   // make sure the job you're checking is not the current job
   if (checkJob == pBot->currentJob) {
      for (i = 0; i < JOB_BUFFER_MAX - 1; i++) {
         ++checkJob;
         if (checkJob >= JOB_BUFFER_MAX)
            checkJob = 0;

         if (pBot->jobType[checkJob] > job_none)
            break; // found a job to check
      }
   }

   // update the selected jobs priority,
   // and remove the job from the buffer if it is now invalid
   if (pBot->jobType[checkJob] > job_none) {
      pBot->job[checkJob].priority = jf[pBot->jobType[checkJob]].assessFuncPtr(pBot, pBot->job[checkJob]);
      if (pBot->job[checkJob].priority == PRIORITY_NONE)
         DropJobFromBuffer(pBot, checkJob);
   }

   // update the current jobs priority,
   // and remove it from the buffer if it is now invalid
   if (pBot->jobType[pBot->currentJob] > job_none) {
      pBot->job[pBot->currentJob].priority = jf[pBot->jobType[pBot->currentJob]].assessFuncPtr(pBot, pBot->job[pBot->currentJob]);
      if (pBot->job[pBot->currentJob].priority == PRIORITY_NONE)
         DropJobFromBuffer(pBot, pBot->currentJob);
   }

   // reset the phase of any jobs that are not currently running
   for (i = 0; i < JOB_BUFFER_MAX; i++) {
      if (i != pBot->currentJob) {
         pBot->job[i].phase = 0;
         pBot->job[i].phase_timer = 0.0f;
      }
   }
}

// This function is the hub of bot behaviour with regards to the job buffer.
// It is responsible for making bots carry out whatever their current job is
// via one of the job type functions.
void BotRunJobs(bot_t *pBot) {
   // keep the buffer up to date
   RefreshJobBuffer(pBot);

   // make the highest priority job in the buffer the current job
   for (int i = 0; i < JOB_BUFFER_MAX; i++) {
      /*	if(pBot->jobType[i] == JOB_PUSH_BUTTON)
                                      UTIL_BotLogPrintf("%s push priority %d, current %d\n",
                                                      pBot->name, pBot->job[i].priority, pBot->job[pBot->currentJob].priority);*/

      if (pBot->jobType[i] != job_none && (pBot->jobType[pBot->currentJob] == job_none || pBot->job[i].priority > pBot->job[pBot->currentJob].priority)) {
         pBot->currentJob = i;
         break;
      }
   }

   // abort if no jobs were found in the buffer
   if (pBot->jobType[pBot->currentJob] == job_none) {
      // if the buffer is completely empty, add JOB_ROAM as a backup
      if (pBot->f_killed_time + 3.0f < pBot->f_think_time) {
         job_struct *newJob = InitialiseNewJob(pBot, job_roam);
         if (newJob != nullptr)
            SubmitNewJob(pBot, job_roam, newJob);
      }

      return;
   }

   // call whichever function handles the bots current job
   // remove it from the buffer if it ended
   if (jf[pBot->jobType[pBot->currentJob]].jobFuncPtr != nullptr && jf[pBot->jobType[pBot->currentJob]].jobFuncPtr(pBot) == JOB_TERMINATED) {
      DropJobFromBuffer(pBot, pBot->currentJob);
   }
}

// This function allows bots to think about adding new jobs to the job buffer.
void BotJobThink(bot_t *pBot) {
   // don't think too often
   if (pBot->f_periodicAlert1 > pBot->f_think_time)
      return;

   job_struct *newJob;

   // if the bot has an invalid or no current waypoint try to find it one
   if (pBot->current_wp < 0 || pBot->current_wp >= num_waypoints) {
      if (BufferedJobIndex(pBot, job_seek_waypoint) == -1) {
         BotFindCurrentWaypoint(pBot);

         // still not got a current waypoint?  set up JOB_SEEK_WAYPOINT
         if (pBot->current_wp < 0) {
            newJob = InitialiseNewJob(pBot, job_seek_waypoint);
            SubmitNewJob(pBot, job_seek_waypoint, newJob);
         }
      }

      return; // to reduce processor consumption
   }

   if (pBot->bot_has_flag) {
      newJob = InitialiseNewJob(pBot, job_capture_flag);
      if (newJob != nullptr && SubmitNewJob(pBot, job_capture_flag, newJob) == true)
         return;
   }

   // infected?
   if (PlayerIsInfected(pBot->pEdict) == true) {
      newJob = InitialiseNewJob(pBot, job_infected_attack);
      if (newJob != nullptr && SubmitNewJob(pBot, job_infected_attack, newJob) == true)
         return;
   }

   // need health?
   if (PlayerHealthPercent(pBot->pEdict) < pBot->trait.health) {
      newJob = InitialiseNewJob(pBot, job_get_health);
      if (newJob != nullptr) {
         newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, pBot->current_team, 3000, W_FL_HEALTH);
         if (newJob->waypoint == -1)
            newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, -1, 3000, W_FL_HEALTH);

         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_get_health, newJob) == true)
            return;
      }
   }

   // need armor(e.g. just spawned)?
   if ((PlayerArmorPercent(pBot->pEdict) < pBot->trait.health || pBot->f_killed_time + 3.1f > pBot->f_think_time) && pBot->f_periodicAlert3 < pBot->f_think_time && random_float(1.0f, 1000.0f) > 600) {
      newJob = InitialiseNewJob(pBot, job_get_armor);
      if (newJob != nullptr) {
         newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, pBot->current_team, 3000, W_FL_ARMOR);
         if (newJob->waypoint == -1)
            newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, -1, 3000, W_FL_ARMOR);

         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_get_armor, newJob) == true)
            return;
      }
   }

   // need ammo?
   if (pBot->ammoStatus != AMMO_UNNEEDED) {
      newJob = InitialiseNewJob(pBot, job_get_ammo);
      if (newJob != nullptr) {
         newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, pBot->current_team, 5000, W_FL_AMMO);
         if (newJob->waypoint == -1)
            newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, -1, 5000, W_FL_AMMO);

         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_get_ammo, newJob) == true)
            return;
      }
   }

   // call for a medic?
   if (pBot->pEdict->v.playerclass != TFC_CLASS_MEDIC && PlayerHealthPercent(pBot->pEdict) < pBot->trait.health && pBot->enemy.f_lastSeen + 3.0f < pBot->f_think_time) {
      // just call if the bot isn't going anywhere
      if (pBot->current_wp == pBot->goto_wp) {
         if (pBot->currentJob > -1 && pBot->jobType[pBot->currentJob] != job_get_health && FriendlyClassTotal(pBot->pEdict, TFC_CLASS_MEDIC, false) > 0 && random_long(0l, 1000l) < 200)
            FakeClientCommand(pBot->pEdict, "saveme", nullptr, nullptr);
      }
      // if the bot saw a medic recently make it a job to call and wait for them
      else if (pBot->f_alliedMedicSeenTime + 5.0f > pBot->f_think_time && (newJob = InitialiseNewJob(pBot, job_call_medic)) != nullptr) {
         const int healthJobIndex = BufferedJobIndex(pBot, job_get_health);

         // if infected or not going for health already call the seen medic
         if (healthJobIndex == -1 || PlayerIsInfected(pBot->pEdict)) {
            if (SubmitNewJob(pBot, job_call_medic, newJob))
               return;
         }
         // if the bot is getting health from far away call the seen medic
         else if (healthJobIndex != -1) {
            const int routeDistance = WaypointDistanceFromTo(pBot->current_wp, pBot->job[healthJobIndex].waypoint, pBot->current_team);
            if (routeDistance > 1100 && SubmitNewJob(pBot, job_call_medic, newJob))
               return;
         }
      }
   }

   // time for the bot to let off some steam with a bit of messing about?
   if (bot_allow_humour && pBot->f_humour_time < pBot->f_think_time) {
      pBot->f_humour_time = pBot->f_think_time + random_float(60.0f, 180.0f);

      // no mucking about if enemies were recently seen or the bot spawned recently
      if (pBot->enemy.f_lastSeen + 40.0f < pBot->f_think_time && pBot->f_killed_time + 40.0f < pBot->f_think_time && BufferedJobIndex(pBot, job_melee_warrior) == -1 && BufferedJobIndex(pBot, job_graffiti_artist) == -1 &&
          random_long(1l, 200l) < pBot->trait.humour) {
         // take your pick of fun things to do
         if (random_long(1l, 1000l) > 400) {
            newJob = InitialiseNewJob(pBot, job_melee_warrior);
            if (newJob != nullptr)
               SubmitNewJob(pBot, job_melee_warrior, newJob);
         } else {
            newJob = InitialiseNewJob(pBot, job_graffiti_artist);
            if (newJob != nullptr)
               SubmitNewJob(pBot, job_graffiti_artist, newJob);
         }
      }
   }

   // let's pick a job suitable for the bots current class
   switch (pBot->bot_class) {
   case TFC_CLASS_CIVILIAN:
      break;
   case TFC_CLASS_SCOUT:
      //pBot->f_think_time = 0.5f;
      //FakeClientCommand(pBot->pEdict, "slot3", nullptr, nullptr);
      break;
   case TFC_CLASS_SNIPER:
      // go snipe
      newJob = InitialiseNewJob(pBot, job_snipe);
      if (newJob != nullptr) {
         newJob->waypoint = BotGoForSniperSpot(pBot);
         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_snipe, newJob) == true) {
            pBot->mission = ROLE_DEFENDER;
            return;
         }
      }
      break;
   case TFC_CLASS_SOLDIER:
      break;
   case TFC_CLASS_DEMOMAN:
      // go set a detpack?
      if (pBot->detpack == 2 && WaypointTypeExists(W_FL_TFC_DETPACK_CLEAR | W_FL_TFC_DETPACK_SEAL, pBot->current_team) && random_long(1l, 1000l) < 334) {
         newJob = InitialiseNewJob(pBot, job_detpack_waypoint);
         if (newJob != nullptr) {
            newJob->waypoint = WaypointFindDetpackGoal(pBot->current_wp, pBot->current_team);
            if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_detpack_waypoint, newJob) == true)
               return;
         }
      }
      break;
   case TFC_CLASS_HWGUY:
      break;
   case TFC_CLASS_PYRO:
      break;
   case TFC_CLASS_MEDIC:
      //pBot->f_think_time = 0.5f;
      //FakeClientCommand(pBot->pEdict, "slot3", nullptr, nullptr);
      break;
   case TFC_CLASS_SPY:
      //pBot->f_think_time = 0.5f;
      //FakeClientCommand(pBot->pEdict, "slot1", nullptr, nullptr);
      // time for a disguise?
      if (pBot->enemy.f_lastSeen + 2 < pBot->f_think_time) {
         if (pBot->current_team == UTIL_GetTeamColor(pBot->pEdict)) {
            newJob = InitialiseNewJob(pBot, job_disguise);
            if (newJob != nullptr && SubmitNewJob(pBot, job_disguise, newJob) == true)
               return;
         }
         // in case the bot forgot an earlier disguise job
         else
            pBot->disguise_state = DISGUISE_COMPLETE;
      }

      // if the ambush check timer has run down see if the bot could start an ambush
      if (pBot->f_spyFeignAmbushTime < pBot->f_think_time) {
         // reset the ambush timer
         pBot->f_spyFeignAmbushTime = pBot->f_think_time + random_float(12.0f, 24.0f);

         newJob = InitialiseNewJob(pBot, job_feign_ambush);
         if (newJob != nullptr && pBot->enemy.f_lastSeen + 3.0f < pBot->f_think_time && pBot->current_wp > -1 && spawnAreaWP[pBot->current_team] > -1 && spawnAreaWP[pBot->current_team] < num_waypoints) {
            const int homeDistance = WaypointDistanceFromTo(spawnAreaWP[pBot->current_team], pBot->current_wp, pBot->current_team);

            // if the bot has left respawn behind and the area is suitable start a
            // feign ambush job
            if (homeDistance > 2000 && SpyAmbushAreaCheck(pBot, newJob->origin)) {
               SubmitNewJob(pBot, job_feign_ambush, newJob);
            }
         }
      }
      break;
   case TFC_CLASS_ENGINEER:
      BotEngineerThink(pBot);
      //FakeClientCommand(pBot->pEdict, "slot1", nullptr, nullptr);
      break;
   default:
      break;
   }

   // time to choose a defense or offense related job
   // is the bot a defender on a known defensable map?
   if (pBot->mission == ROLE_DEFENDER && WaypointTypeExists(W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY, pBot->current_team)) {
      if (BufferedJobIndex(pBot, job_guard_waypoint) != -1 || BufferedJobIndex(pBot, job_patrol_home) != -1)
         return; // no need, guarding already

      // demomen can guard, patrol or set up pipetraps
      if (pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN) {
         if (BufferedJobIndex(pBot, job_pipetrap) != -1)
            return; // no need, guarding already

         // 50% chance the demoman will set up a pipetrap or just guard
         if (random_long(1l, 1000l) <= 500) {
            newJob = InitialiseNewJob(pBot, job_pipetrap);
            if (newJob != nullptr && WaypointTypeExists(W_FL_TFC_PIPETRAP, pBot->current_team)) {
               newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_PIPETRAP);

               if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_pipetrap, newJob))
                  return; // success
            }

            // failed to set up a pipetrap job, try basic guard duty
            newJob = InitialiseNewJob(pBot, job_guard_waypoint);
            if (newJob != nullptr && WaypointTypeExists(W_FL_TFC_PL_DEFEND, pBot->current_team)) {
               newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_PL_DEFEND);
               if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_guard_waypoint, newJob) == true)
                  return; // success
            }
         }
      }

      // these classes can guard or patrol
      if ((pBot->pEdict->v.playerclass == TFC_CLASS_SOLDIER || pBot->pEdict->v.playerclass == TFC_CLASS_HWGUY || pBot->pEdict->v.playerclass == TFC_CLASS_PYRO) && WaypointTypeExists(W_FL_TFC_PL_DEFEND, pBot->current_team)) {
         newJob = InitialiseNewJob(pBot, job_guard_waypoint);
         if (newJob != nullptr) {
            int guardChance = 750;

            // find out if any bot teammates are guarding already
            for (int i = 0; i < MAX_BOTS; i++) {
               if (bots[i].is_used && bots[i].current_team == pBot->current_team && (BufferedJobIndex(&bots[i], job_guard_waypoint) != -1 || BufferedJobIndex(&bots[i], job_pipetrap) != -1)) {
                  guardChance = 500; // only 50% chance the bot will guard
                  break;
               }
            }

            if (random_long(1l, 1000l) <= guardChance) {
               newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_PL_DEFEND);
               if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_guard_waypoint, newJob) == true)
                  return; // success
            }
         }
      }

      // patrol the base if any of the above doesn't come true
      newJob = InitialiseNewJob(pBot, job_patrol_home);
      if (newJob != nullptr && SubmitNewJob(pBot, job_patrol_home, newJob) == true)
         return;
   }
   // bot is attacking, give it an attack job if it hasn't one already
   else if (BufferedJobIndex(pBot, job_get_flag) == -1 && BufferedJobIndex(pBot, job_harrass_defense) == -1) {
      int getFlagChance = 500;

      if (pBot->pEdict->v.playerclass == TFC_CLASS_SCOUT || pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN)
         getFlagChance = 1000;
      else if (pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC)
         getFlagChance = 750;

      // does the bot "feel" like going straight for a flag
      if (random_long(1l, 1000l) <= getFlagChance) {
         newJob = InitialiseNewJob(pBot, job_get_flag);
         if (newJob != nullptr) {
            newJob->waypoint = BotFindFlagWaypoint(pBot);
            if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_get_flag, newJob))
               return;
         }
      }

      // not going for a flag, try attacking the enemy defences instead
      newJob = InitialiseNewJob(pBot, job_harrass_defense);
      if (newJob != nullptr)
         SubmitNewJob(pBot, job_harrass_defense, newJob);
   }
}

// This function allows Engineer bots to think about adding new jobs to the job buffer.
void BotEngineerThink(bot_t *pBot) {
   // is the bot's sentry gun now in an unavailable map area?
   if (pBot->has_sentry && !WaypointAvailable(pBot->sentryWaypoint, pBot->current_team)) {
      FakeClientCommand(pBot->pEdict, "detsentry", nullptr, nullptr);
      FakeClientCommand(pBot->pEdict, "detdispenser", nullptr, nullptr);
   }

   // is the bot's teleporter entrance now in an unavailable map area?
   if (pBot->tpEntranceWP != -1 && !FNullEnt(pBot->tpEntrance) && !WaypointAvailable(pBot->tpEntranceWP, pBot->current_team))
      FakeClientCommand(pBot->pEdict, "detentryteleporter", nullptr, nullptr);

   // is the bot's teleporter exit now in an unavailable map area?
   if (pBot->tpExitWP != -1 && !FNullEnt(pBot->tpExit) && !WaypointAvailable(pBot->tpExitWP, pBot->current_team))
      FakeClientCommand(pBot->pEdict, "detexitteleporter", nullptr, nullptr);

   /* // Sentry gun still alive? This script broken? [APG]RoboCop[CL]
                   if(pBot->has_sentry == true
                                   && (pBot->sentry_edict == nullptr
                                   || FNullEnt(pBot->sentry_edict)))
                   {
                                   pBot->has_sentry = false;
                                   pBot->sentry_edict = nullptr;
                   }
   */
   job_struct *newJob;

   // time to build a dispenser?
   if (pBot->has_dispenser == false && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 139 && random_long(1l, 1000l) < 400) {
      newJob = InitialiseNewJob(pBot, job_build_dispenser);
      if (newJob != nullptr) {
         newJob->waypoint = BotGetDispenserBuildWaypoint(pBot);

         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_build_dispenser, newJob) == true)
            return;
      }
   }

   // time to build a sentry?
   if (pBot->has_sentry == false && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 139) {
      newJob = InitialiseNewJob(pBot, job_build_sentry);
      if (newJob != nullptr) {
         newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_SENTRY);

         if (newJob->waypoint == -1)
            newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, -1, W_FL_TFC_SENTRY);

         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_build_sentry, newJob) == true)
            return;
      }
   }

   // repair/upgrade the bot's sentry?
   if (pBot->has_sentry == true && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 139 && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] > 10) {
      char modelName[30];
      strncpy(modelName, STRING(pBot->sentry_edict->v.model), 30);
      modelName[29] = '\0';

      if (pBot->sentry_ammo < 100 || pBot->sentry_edict->v.health < 100 || strcmp(modelName, "models/sentry3.mdl") != 0) {
         newJob = InitialiseNewJob(pBot, job_maintain_object);
         if (newJob != nullptr) {
            newJob->object = pBot->sentry_edict;

            if (SubmitNewJob(pBot, job_maintain_object, newJob) == true)
               return;
         }
      }
   }

   // detonate the bots teleporter exit if it doesn't have an teleporter entrance
   // i.e. build entrances and then build exits, not the other way round
   // this is because each map has a limited number of teleporter waypoints
   if (FNullEnt(pBot->tpEntrance) && !FNullEnt(pBot->tpExit)) {
      //	UTIL_BotLogPrintf("%s, detting my exit\n", pBot->name);
      FakeClientCommand(pBot->pEdict, "detexitteleporter", nullptr, nullptr);
   }

   // build a teleporter entrance?
   if (FNullEnt(pBot->tpEntrance) && bot_can_build_teleporter == true && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 190 && random_long(1l, 1000l) < 101) {
      newJob = InitialiseNewJob(pBot, job_build_teleport);
      if (newJob != nullptr) {
         newJob->waypoint = BotGetTeleporterBuildWaypoint(pBot, true);

         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_build_teleport, newJob) == true)
            return;
      }
   }

   // build a teleporter exit?
   // only do so if the bot owns a Teleporter entrance already
   if (!FNullEnt(pBot->tpEntrance) && FNullEnt(pBot->tpExit) && bot_can_build_teleporter == true && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 190 && random_long(1, 1000) < 101) {
      newJob = InitialiseNewJob(pBot, job_build_teleport);
      if (newJob != nullptr) {
         newJob->waypoint = BotGetTeleporterBuildWaypoint(pBot, false);

         if (newJob->waypoint != -1 && SubmitNewJob(pBot, job_build_teleport, newJob) == true)
            return;
      }
   }
}