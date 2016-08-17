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

#include <float.h>

#include "extdll.h"
#include "bot.h"
#include <meta_api.h>
#include "cbase.h"
#include "tf_defs.h"

#include "list.h"

#include "bot_func.h"

#include "bot_job_think.h"
#include "bot_job_assessors.h"
#include "bot_job_functions.h"
#include "bot_weapons.h"
#include "waypoint.h"
#include "bot_navigate.h"

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
static void DropJobFromBuffer(bot_t* pBot, const int buffer_index);
static void RefreshJobBuffer(bot_t* pBot);
static void BotEngineerThink(bot_t* pBot);

// structure for storing each job types function pointers
typedef struct {
    int (*assessFuncPtr)(const bot_t* pBot, const job_struct& r_Job);
    int (*jobFuncPtr)(bot_t* pBot);
} jobFunctions_struct;

// probably best to keep these function pointers private to this file
// these must be in the right order for each job to run properly
static const jobFunctions_struct jf[JOB_TYPE_TOTAL] = {
    { assess_JobSeekWaypoint, JobSeekWaypoint }, { assess_JobGetUnstuck, JobGetUnstuck }, { assess_JobRoam, JobRoam },
    { assess_JobChat, JobChat }, { assess_JobReport, JobReport }, { assess_JobPickUpItem, JobPickUpItem },
    { assess_JobPickUpFlag, JobPickUpFlag }, { assess_JobPushButton, JobPushButton },
    { assess_JobUseTeleport, JobUseTeleport }, { assess_JobMaintainObject, JobMaintainObject },
    { assess_JobBuildSentry, JobBuildSentry }, { assess_JobBuildDispenser, JobBuildDispenser },
    { assess_JobBuildTeleport, JobBuildTeleport }, { assess_JobBuffAlly, JobBuffAlly },
    { assess_JobEscortAlly, JobEscortAlly }, { assess_JobCallMedic, JobCallMedic },
    { assess_JobGetHealth, JobGetHealth }, { assess_JobGetArmor, JobGetArmor }, { assess_JobGetAmmo, JobGetAmmo },
    { assess_JobDisguise, JobDisguise }, { assess_JobFeignAmbush, JobFeignAmbush }, { assess_JobSnipe, JobSnipe },
    { assess_JobGuardWaypoint, JobGuardWaypoint }, { assess_JobDefendFlag, JobDefendFlag },
    { assess_JobGetFlag, JobGetFlag }, { assess_JobCaptureFlag, JobCaptureFlag },
    { assess_JobHarrassDefense, JobHarrassDefense }, { assess_JobRocketJump, JobRocketJump },
    { assess_JobConcussionJump, JobConcussionJump }, { assess_JobDetpackWaypoint, JobDetpackWaypoint },
    { assess_JobPipetrap, JobPipetrap }, { assess_JobInvestigateArea, JobInvestigateArea },
    { assess_JobPursueEnemy, JobPursueEnemy }, { assess_JobPatrolHome, JobPatrolHome },
    { assess_JobSpotStimulus, JobSpotStimulus }, { assess_JobAttackBreakable, JobAttackBreakable },
    { assess_JobAttackTeleport, JobAttackTeleport }, { assess_JobSeekBackup, JobSeekBackup },
    { assess_JobAvoidEnemy, JobAvoidEnemy }, { assess_JobAvoidAreaDamage, JobAvoidAreaDamage },
    { assess_JobInfectedAttack, JobInfectedAttack }, { assess_JobBinGrenade, JobBinGrenade },
    { assess_JobDrownRecover, JobDrownRecover }, { assess_JobMeleeWarrior, JobMeleeWarrior },
    { assess_JobGraffitiArtist, JobGraffitiArtist },
};

// list of essential data for all known job types
// these must be in the right order for each job to run properly
const jobList_struct jl[JOB_TYPE_TOTAL] = {
    { 400, "JOB_SEEK_WAYPOINT" }, { PRIORITY_MAXIMUM, "JOB_GET_UNSTUCK" }, { 0, "JOB_ROAM" }, { 500, "JOB_CHAT" },
    { 500, "JOB_REPORT" }, { 520, "JOB_PICKUP_ITEM" }, { 450, "JOB_PICKUP_FLAG" },
    { PRIORITY_MAXIMUM - 1, "JOB_PUSH_BUTTON" }, { 460, "JOB_USE_TELEPORT" }, { 420, "JOB_MAINTAIN_OBJECT" },
    { 400, "JOB_BUILD_SENTRY" }, { 400, "JOB_BUILD_DISPENSER" }, { 400, "JOB_BUILD_TELEPORT" },
    { 410, "JOB_BUFF_ALLY" }, { 400, "JOB_ESCORT_ALLY" },
    { 520, "JOB_CALL_MEDIC" }, // this should be a higher priority than JOB_GET_HEALTH
    { 510, "JOB_GET_HEALTH" }, { 500, "JOB_GET_ARMOR" }, { 500, "JOB_GET_AMMO" }, { 400, "JOB_DISGUISE" },
    { 400, "JOB_FEIGN_AMBUSH" }, { 400, "JOB_SNIPE" }, { 300, "JOB_GUARD_WAYPOINT" }, { 410, "JOB_DEFEND_FLAG" },
    { 250, "JOB_GET_FLAG" }, { 410, "JOB_CAPTURE_FLAG" }, { 250, "JOB_HARRASS_DEFENSE" },
    { PRIORITY_MAXIMUM - 1, "JOB_ROCKET_JUMP" }, { PRIORITY_MAXIMUM, "JOB_CONCUSSION_JUMP" },
    { 400, "JOB_DETPACK_WAYPOINT" }, { 350, "JOB_PIPETRAP" }, { 400, "JOB_INVESTIGATE_AREA" },
    { 400, "JOB_PURSUE_ENEMY" }, { 300, "JOB_PATROL_HOME" }, { 710, "JOB_SPOT_STIMULUS" },
    { 600, "JOB_ATTACK_BREAKABLE" }, { 410, "JOB_ATTACK_TELEPORT" }, { 650, "JOB_SEEK_BACKUP" },
    { 700, "JOB_AVOID_ENEMY" }, { PRIORITY_MAXIMUM - 1, "JOB_AVOID_AREA_DAMAGE" },
    { PRIORITY_MAXIMUM - 2, "JOB_INFECTED_ATTACK" }, { PRIORITY_MAXIMUM, "JOB_BIN_GRENADE" },
    { PRIORITY_MAXIMUM, "JOB_DROWN_RECOVER" }, { 410, "JOB_MELEE_WARRIOR" }, { 410, "JOB_GRAFFITI_ARTIST" },
};

// This function clears the specified bots job buffer, and thus should
// probably be called when the bot is created.
void BotResetJobBuffer(bot_t* pBot)
{
    int i;

    pBot->currentJob = 0;
    for(i = 0; i < JOB_BUFFER_MAX; i++) {
        DropJobFromBuffer(pBot, i);
    }

    // reset the job blacklist
    for(i = 0; i < JOB_BLACKLIST_MAX; i++) {
        pBot->jobBlacklist[i].type = JOB_NONE;
        pBot->jobBlacklist[i].f_timeOut = 0.0;
    }
}

// This function eliminates whatever job is in the specified buffer
// index, and is the preferred method of terminating any job.
static void DropJobFromBuffer(bot_t* pBot, const int buffer_index)
{
    pBot->jobType[buffer_index] = JOB_NONE;
    pBot->job[buffer_index].f_bufferedTime = 0.0;
    pBot->job[buffer_index].priority = PRIORITY_NONE;
    pBot->job[buffer_index].phase = 0;
    pBot->job[buffer_index].phase_timer = 0.0;
    pBot->job[buffer_index].waypoint = -1;
    pBot->job[buffer_index].waypointTwo = -1;
    pBot->job[buffer_index].player = NULL;
    pBot->job[buffer_index].object = NULL;
    pBot->job[buffer_index].message[0] = '\0';
}

// This function allows a job to be blacklisted(so that it cannot be added to the buffer)
// for a certain amount of time.  A job can be blacklisted because it keeps
// failing in a repetitive fashion.  e.g. Bot goes to repair a sentry gun but can't
// find a route to it from a certain position.
// timeOut should specify the number of seconds you want the job blacklisted for and
// should depend upon the type type of job that failed and the type of failure.
void BlacklistJob(bot_t* pBot, const int jobType, const float timeOut)
{
    int shortestIndex = -1;
    float shortestTime = FLT_MAX;

    for(int i = 0; i < JOB_BLACKLIST_MAX; i++) {
        if(pBot->jobBlacklist[i].f_timeOut < shortestTime) {
            shortestIndex = i;
            shortestTime = pBot->jobBlacklist[i].f_timeOut;
        }
    }

    if(shortestIndex != -1) {
        pBot->jobBlacklist[shortestIndex].type = jobType;
        pBot->jobBlacklist[shortestIndex].f_timeOut = pBot->f_think_time + timeOut;
    }
}

// This function returns TRUE if the specified jobtype is already in the
// job buffer.
bool BufferContainsJobType(const bot_t* pBot, const int JobType)
{
    for(int i = 0; i < JOB_BUFFER_MAX; i++) {
        if(pBot->jobType[i] == JobType)
            return TRUE;
    }

    return FALSE;
}

// If the specified job type is already in the job buffer this function will
// return the buffer index it occupies.
// Returns -1 if the job type is not in the buffer.
int BufferedJobIndex(const bot_t* pBot, const int JobType)
{
    for(int i = 0; i < JOB_BUFFER_MAX; i++) {
        if(pBot->jobType[i] == JobType)
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
job_struct* InitialiseNewJob(bot_t* pBot, const int newJobType)
{
    int i;

    // abort if the job is in the job buffer already
    for(i = 0; i < JOB_BUFFER_MAX; i++) {
        if(pBot->jobType[i] == newJobType)
            return NULL;
    }

    // abort if the job is currently blacklisted
    for(i = 0; i < JOB_BLACKLIST_MAX; i++) {
        if(newJobType == pBot->jobBlacklist[i].type && pBot->jobBlacklist[i].f_timeOut >= pBot->f_think_time)
            return NULL;
    }

    static job_struct newJob;

    // reset the new job's data
    newJob.f_bufferedTime = pBot->f_think_time;
    newJob.priority = PRIORITY_NONE;
    newJob.phase = 0;
    newJob.phase_timer = 0.0;
    newJob.waypoint = -1;
    newJob.waypointTwo = -1;
    newJob.player = NULL;
    newJob.object = NULL;
    newJob.message[0] = '\0';

    return &newJob;
}

// This function should be used when you wish to add a new job to the job buffer.
// It searches the job buffer for a free index or the lowest priority
// job that is less important than the one that you wish to add to the buffer.
// It returns TRUE if the job was accepted.
bool SubmitNewJob(bot_t* pBot, const int newJobType, job_struct* newJob)
{
    int i;

    if(pBot->current_wp == -1 && newJobType != JOB_SEEK_WAYPOINT) // bit of a kludge but necessary
        return FALSE; // many job assessor functions need a valid current waypoint

    // if the job is currently blacklisted keep it out of the buffer
    for(i = 0; i < JOB_BLACKLIST_MAX; i++) {
        if(newJobType == pBot->jobBlacklist[i].type) {
            if(pBot->jobBlacklist[i].f_timeOut >= pBot->f_think_time)
                return FALSE;
            else
                pBot->jobBlacklist[i].type = JOB_NONE; // job has timed out de-blacklist it
        }
    }

    // set some defaults for the new job
    newJob->f_bufferedTime = pBot->f_think_time;
    newJob->phase = 0;
    newJob->phase_timer = 0.0;

    // assess the proposed new jobs validity and priority
    newJob->priority = jf[newJobType].assessFuncPtr(pBot, *newJob);
    if(newJob->priority == PRIORITY_NONE)
        return FALSE;

    // look for the job with the lowest priority
    int worstJobFound = -1;
    int worstJobPriority = newJob->priority;
    for(i = 0; i < JOB_BUFFER_MAX; i++) {
        // found an unused job index?
        if(pBot->jobType[i] == JOB_NONE) {
            worstJobFound = i;
            worstJobPriority = PRIORITY_NONE;
            continue;
        }

        // currently, only one of each type of job is allowed in the buffer
        if(pBot->jobType[i] == newJobType)
            return FALSE;

        // is this the lowest priority job found so far?
        if(pBot->job[i].priority < worstJobPriority) {
            worstJobFound = i;
            worstJobPriority = pBot->job[i].priority;
        }
    }

    // add the job to the buffer if it was important enough
    if(worstJobFound != -1) {
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
        pBot->job[worstJobFound].phase_timer = 0.0;
        return TRUE;
    }

    return FALSE;
}

// This function is reponsible for keeping the contents of the job buffer
// up to date.  For example, it can check the priority and validity of all
// jobs currently in the buffer.
static void RefreshJobBuffer(bot_t* pBot)
{
    int i;

    int checkJob = random_long(0, JOB_BUFFER_MAX - 1);

    // make sure the job you're checking is not the current job
    if(checkJob == pBot->currentJob) {
        for(i = 0; i < (JOB_BUFFER_MAX - 1); i++) {
            ++checkJob;
            if(checkJob >= JOB_BUFFER_MAX)
                checkJob = 0;

            if(pBot->jobType[checkJob] > JOB_NONE)
                break; // found a job to check
        }
    }

    // update the selected jobs priority,
    // and remove the job from the buffer if it is now invalid
    if(pBot->jobType[checkJob] > JOB_NONE) {
        pBot->job[checkJob].priority = jf[pBot->jobType[checkJob]].assessFuncPtr(pBot, pBot->job[checkJob]);
        if(pBot->job[checkJob].priority == PRIORITY_NONE)
            DropJobFromBuffer(pBot, checkJob);
    }

    // update the current jobs priority,
    // and remove it from the buffer if it is now invalid
    if(pBot->jobType[pBot->currentJob] > JOB_NONE) {
        pBot->job[pBot->currentJob].priority =
            jf[pBot->jobType[pBot->currentJob]].assessFuncPtr(pBot, pBot->job[pBot->currentJob]);
        if(pBot->job[pBot->currentJob].priority == PRIORITY_NONE)
            DropJobFromBuffer(pBot, pBot->currentJob);
    }

    // reset the phase of any jobs that are not currently running
    for(i = 0; i < JOB_BUFFER_MAX; i++) {
        if(i != pBot->currentJob) {
            pBot->job[i].phase = 0;
            pBot->job[i].phase_timer = 0.0;
        }
    }
}

// This function is the hub of bot behaviour with regards to the job buffer.
// It is responsible for making bots carry out whatever their current job is
// via one of the job type functions.
void BotRunJobs(bot_t* pBot)
{
    // keep the buffer up to date
    RefreshJobBuffer(pBot);

    // make the highest priority job in the buffer the current job
    for(int i = 0; i < JOB_BUFFER_MAX; i++) {
        /*	if(pBot->jobType[i] == JOB_PUSH_BUTTON)
                        UTIL_BotLogPrintf("%s push priority %d, current %d\n",
                                pBot->name, pBot->job[i].priority, pBot->job[pBot->currentJob].priority);*/

        if(pBot->jobType[i] != JOB_NONE && (pBot->jobType[pBot->currentJob] == JOB_NONE ||
                                               pBot->job[i].priority > pBot->job[pBot->currentJob].priority)) {
            pBot->currentJob = i;
            break;
        }
    }

    // abort if no jobs were found in the buffer
    if(pBot->jobType[pBot->currentJob] == JOB_NONE) {
        // if the buffer is completely empty, add JOB_ROAM as a backup
        if((pBot->f_killed_time + 3.0) < pBot->f_think_time) {
            job_struct* newJob = InitialiseNewJob(pBot, JOB_ROAM);
            if(newJob != NULL)
                SubmitNewJob(pBot, JOB_ROAM, newJob);
        }

        return;
    }

    // call whichever function handles the bots current job
    // remove it from the buffer if it ended
    if(jf[pBot->jobType[pBot->currentJob]].jobFuncPtr != NULL &&
        jf[pBot->jobType[pBot->currentJob]].jobFuncPtr(pBot) == JOB_TERMINATED) {
        DropJobFromBuffer(pBot, pBot->currentJob);
    }
}

// This function allows bots to think about adding new jobs to the job buffer.
void BotJobThink(bot_t* pBot)
{
    // don't think too often
    if(pBot->f_periodicAlert1 > pBot->f_think_time)
        return;

    job_struct* newJob = NULL;

    // if the bot has an invalid or no current waypoint try to find it one
    if(pBot->current_wp < 0 || pBot->current_wp >= num_waypoints) {
        if(BufferedJobIndex(pBot, JOB_SEEK_WAYPOINT) == -1) {
            BotFindCurrentWaypoint(pBot);

            // still not got a current waypoint?  set up JOB_SEEK_WAYPOINT
            if(pBot->current_wp < 0) {
                newJob = InitialiseNewJob(pBot, JOB_SEEK_WAYPOINT);
                SubmitNewJob(pBot, JOB_SEEK_WAYPOINT, newJob);
            }
        }

        return; // to reduce processor consumption
    }

    if(pBot->bot_has_flag) {
        newJob = InitialiseNewJob(pBot, JOB_CAPTURE_FLAG);
        if(newJob != NULL && SubmitNewJob(pBot, JOB_CAPTURE_FLAG, newJob) == TRUE)
            return;
    }

    // infected?
    if(PlayerIsInfected(pBot->pEdict) == TRUE) {
        newJob = InitialiseNewJob(pBot, JOB_INFECTED_ATTACK);
        if(newJob != NULL && SubmitNewJob(pBot, JOB_INFECTED_ATTACK, newJob) == TRUE)
            return;
    }

    // need health?
    if(PlayerHealthPercent(pBot->pEdict) < pBot->trait.health) {
        newJob = InitialiseNewJob(pBot, JOB_GET_HEALTH);
        if(newJob != NULL) {
            newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, pBot->current_team, 3000, W_FL_HEALTH);
            if(newJob->waypoint == -1)
                newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, -1, 3000, W_FL_HEALTH);

            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_GET_HEALTH, newJob) == TRUE)
                return;
        }
    }

    // need armor(e.g. just spawned)?
    if(PlayerArmorPercent(pBot->pEdict) < pBot->trait.health ||
        ((pBot->f_killed_time + 3.1) > pBot->f_think_time && pBot->f_periodicAlert3 < pBot->f_think_time &&
            random_float(1, 1000) > 600)) {
        newJob = InitialiseNewJob(pBot, JOB_GET_ARMOR);
        if(newJob != NULL) {
            newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, pBot->current_team, 3000, W_FL_ARMOR);
            if(newJob->waypoint == -1)
                newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, -1, 3000, W_FL_ARMOR);

            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_GET_ARMOR, newJob) == TRUE)
                return;
        }
    }

    // need ammo?
    if(pBot->ammoStatus != AMMO_UNNEEDED) {
        newJob = InitialiseNewJob(pBot, JOB_GET_AMMO);
        if(newJob != NULL) {
            newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, pBot->current_team, 5000, W_FL_AMMO);
            if(newJob->waypoint == -1)
                newJob->waypoint = WaypointFindNearestGoal(pBot->current_wp, -1, 5000, W_FL_AMMO);

            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_GET_AMMO, newJob) == TRUE)
                return;
        }
    }

    // call for a medic?
    if(pBot->pEdict->v.playerclass != TFC_CLASS_MEDIC && PlayerHealthPercent(pBot->pEdict) < pBot->trait.health &&
        (pBot->enemy.f_lastSeen + 3.0) < pBot->f_think_time) {
        // just call if the bot isn't going anywhere
        if(pBot->current_wp == pBot->goto_wp) {
            if(pBot->currentJob > -1 && pBot->jobType[pBot->currentJob] != JOB_GET_HEALTH &&
                FriendlyClassTotal(pBot->pEdict, TFC_CLASS_MEDIC, FALSE) > 0 && random_long(0, 1000) < 200)
                FakeClientCommand(pBot->pEdict, "saveme", NULL, NULL);
        }
        // if the bot saw a medic recently make it a job to call and wait for them
        else if((pBot->f_alliedMedicSeenTime + 5.0) > pBot->f_think_time &&
            (newJob = InitialiseNewJob(pBot, JOB_CALL_MEDIC)) != NULL) {
            const int healthJobIndex = BufferedJobIndex(pBot, JOB_GET_HEALTH);

            // if infected or not going for health already call the seen medic
            if(healthJobIndex == -1 || PlayerIsInfected(pBot->pEdict)) {
                if(SubmitNewJob(pBot, JOB_CALL_MEDIC, newJob))
                    return;
            }
            // if the bot is getting health from far away call the seen medic
            else if(healthJobIndex != -1) {
                const int routeDistance =
                    WaypointDistanceFromTo(pBot->current_wp, pBot->job[healthJobIndex].waypoint, pBot->current_team);
                if(routeDistance > 1100 && SubmitNewJob(pBot, JOB_CALL_MEDIC, newJob))
                    return;
            }
        }
    }

    // time for the bot to let off some steam with a bit of messing about?
    if(bot_allow_humour && pBot->f_humour_time < pBot->f_think_time) {
        pBot->f_humour_time = pBot->f_think_time + random_float(60.0, 180.0);

        // no mucking about if enemies were recently seen or the bot spawned recently
        if((pBot->enemy.f_lastSeen + 40.0) < pBot->f_think_time && (pBot->f_killed_time + 40.0) < pBot->f_think_time &&
            BufferedJobIndex(pBot, JOB_MELEE_WARRIOR) == -1 && BufferedJobIndex(pBot, JOB_GRAFFITI_ARTIST) == -1 &&
            random_long(1, 200) < pBot->trait.humour) {
            // take your pick of fun things to do
            if(random_long(1, 1000) > 400) {
                newJob = InitialiseNewJob(pBot, JOB_MELEE_WARRIOR);
                if(newJob != NULL)
                    SubmitNewJob(pBot, JOB_MELEE_WARRIOR, newJob);
            } else {
                newJob = InitialiseNewJob(pBot, JOB_GRAFFITI_ARTIST);
                if(newJob != NULL)
                    SubmitNewJob(pBot, JOB_GRAFFITI_ARTIST, newJob);
            }
        }
    }

    // let's pick a job suitable for the bots current class
    switch(pBot->bot_class) {
    case TFC_CLASS_CIVILIAN:
        break;
    case TFC_CLASS_SCOUT:
        break;
    case TFC_CLASS_SNIPER:
        // go snipe
        newJob = InitialiseNewJob(pBot, JOB_SNIPE);
        if(newJob != NULL) {
            newJob->waypoint = BotGoForSniperSpot(pBot);
            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_SNIPE, newJob) == TRUE) {
                pBot->mission = ROLE_DEFENDER;
                return;
            }
        }
        break;
    case TFC_CLASS_SOLDIER:
        break;
    case TFC_CLASS_DEMOMAN:
        // go set a detpack?
        if(pBot->detpack == 2 &&
            WaypointTypeExists(W_FL_TFC_DETPACK_CLEAR | W_FL_TFC_DETPACK_SEAL, pBot->current_team) &&
            random_long(1, 1000) < 334) {
            newJob = InitialiseNewJob(pBot, JOB_DETPACK_WAYPOINT);
            if(newJob != NULL) {
                newJob->waypoint = WaypointFindDetpackGoal(pBot->current_wp, pBot->current_team);
                if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_DETPACK_WAYPOINT, newJob) == TRUE)
                    return;
            }
        }
        break;
    case TFC_CLASS_HWGUY:
        break;
    case TFC_CLASS_PYRO:
        break;
    case TFC_CLASS_MEDIC:
        break;
    case TFC_CLASS_SPY:
        // time for a disguise?
        if((pBot->enemy.f_lastSeen + 2.0) < pBot->f_think_time) {
            if(pBot->current_team == UTIL_GetTeamColor(pBot->pEdict)) {
                newJob = InitialiseNewJob(pBot, JOB_DISGUISE);
                if(newJob != NULL && SubmitNewJob(pBot, JOB_DISGUISE, newJob) == TRUE)
                    return;
            }
            // in case the bot forgot an earlier disguise job
            else
                pBot->disguise_state = DISGUISE_COMPLETE;
        }

        // if the ambush check timer has run down see if the bot could start an ambush
        if(pBot->f_spyFeignAmbushTime < pBot->f_think_time) {
            // reset the ambush timer
            pBot->f_spyFeignAmbushTime = pBot->f_think_time + random_float(12.0, 24.0);

            newJob = InitialiseNewJob(pBot, JOB_FEIGN_AMBUSH);
            if(newJob != NULL && (pBot->enemy.f_lastSeen + 3.0) < pBot->f_think_time && pBot->current_wp > -1 &&
                spawnAreaWP[pBot->current_team] > -1 && spawnAreaWP[pBot->current_team] < num_waypoints) {
                const int homeDistance =
                    WaypointDistanceFromTo(spawnAreaWP[pBot->current_team], pBot->current_wp, pBot->current_team);

                // if the bot has left respawn behind and the area is suitable start a
                // feign ambush job
                if(homeDistance > 2000 && SpyAmbushAreaCheck(pBot, newJob->origin)) {
                    SubmitNewJob(pBot, JOB_FEIGN_AMBUSH, newJob);
                }
            }
        }
        break;
    case TFC_CLASS_ENGINEER:
        BotEngineerThink(pBot);
        break;
    default:
        break;
    }

    // time to choose a defense or offense related job
    // is the bot a defender on a known defensable map?
    if(pBot->mission == ROLE_DEFENDER &&
        WaypointTypeExists(W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY, pBot->current_team)) {
        if(BufferedJobIndex(pBot, JOB_GUARD_WAYPOINT) != -1 || BufferedJobIndex(pBot, JOB_PATROL_HOME) != -1)
            return; // no need, guarding already

        // demomen can guard, patrol or set up pipetraps
        if(pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN) {
            if(BufferedJobIndex(pBot, JOB_PIPETRAP) != -1)
                return; // no need, guarding already

            // 50% chance the demoman will set up a pipetrap or just guard
            if(random_long(1, 1000) <= 500) {
                newJob = InitialiseNewJob(pBot, JOB_PIPETRAP);
                if(newJob != NULL && WaypointTypeExists(W_FL_TFC_PIPETRAP, pBot->current_team)) {
                    newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_PIPETRAP);

                    if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_PIPETRAP, newJob))
                        return; // success
                }

                // failed to set up a pipetrap job, try basic guard duty
                newJob = InitialiseNewJob(pBot, JOB_GUARD_WAYPOINT);
                if(newJob != NULL && WaypointTypeExists(W_FL_TFC_PL_DEFEND, pBot->current_team)) {
                    newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_PL_DEFEND);
                    if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_GUARD_WAYPOINT, newJob) == TRUE)
                        return; // success
                }
            }
        }

        // these classes can guard or patrol
        if((pBot->pEdict->v.playerclass == TFC_CLASS_SOLDIER || pBot->pEdict->v.playerclass == TFC_CLASS_HWGUY ||
               pBot->pEdict->v.playerclass == TFC_CLASS_PYRO) &&
            WaypointTypeExists(W_FL_TFC_PL_DEFEND, pBot->current_team)) {
            newJob = InitialiseNewJob(pBot, JOB_GUARD_WAYPOINT);
            if(newJob != NULL) {
                int guardChance = 750;

                // find out if any bot teammates are guarding already
                for(int i = 0; i < MAX_BOTS; i++) {
                    if(bots[i].is_used && bots[i].current_team == pBot->current_team &&
                        (BufferedJobIndex(&bots[i], JOB_GUARD_WAYPOINT) != -1 ||
                            BufferedJobIndex(&bots[i], JOB_PIPETRAP) != -1)) {
                        guardChance = 500; // only 50% chance the bot will guard
                        break;
                    }
                }

                if(random_long(1, 1000) <= guardChance) {
                    newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_PL_DEFEND);
                    if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_GUARD_WAYPOINT, newJob) == TRUE)
                        return; // success
                }
            }
        }

        // patrol the base if any of the above doesn't come true
        newJob = InitialiseNewJob(pBot, JOB_PATROL_HOME);
        if(newJob != NULL && SubmitNewJob(pBot, JOB_PATROL_HOME, newJob) == TRUE)
            return;
    }
    // bot is attacking, give it an attack job if it hasn't one already
    else if(BufferedJobIndex(pBot, JOB_GET_FLAG) == -1 && BufferedJobIndex(pBot, JOB_HARRASS_DEFENSE) == -1) {
        int getFlagChance = 500;

        if(pBot->pEdict->v.playerclass == TFC_CLASS_SCOUT || pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN)
            getFlagChance = 1000;
        else if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC)
            getFlagChance = 750;

        // does the bot "feel" like going straight for a flag
        if(random_long(1, 1000) <= getFlagChance) {
            newJob = InitialiseNewJob(pBot, JOB_GET_FLAG);
            if(newJob != NULL) {
                newJob->waypoint = BotFindFlagWaypoint(pBot);
                if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_GET_FLAG, newJob))
                    return;
            }
        }

        // not going for a flag, try attacking the enemy defences instead
        newJob = InitialiseNewJob(pBot, JOB_HARRASS_DEFENSE);
        if(newJob != NULL)
            SubmitNewJob(pBot, JOB_HARRASS_DEFENSE, newJob);
    }
}

// This function allows Engineer bots to think about adding new jobs to the job buffer.
void BotEngineerThink(bot_t* pBot)
{
    // is the bot's sentry gun now in an unavailable map area?
    if(pBot->has_sentry && !WaypointAvailable(pBot->sentryWaypoint, pBot->current_team)) {
        FakeClientCommand(pBot->pEdict, "detsentry", NULL, NULL);
        FakeClientCommand(pBot->pEdict, "detdispenser", NULL, NULL);
    }

    // is the bot's teleporter entrance now in an unavailable map area?
    if(pBot->tpEntranceWP != -1 && !FNullEnt(pBot->tpEntrance) &&
        !WaypointAvailable(pBot->tpEntranceWP, pBot->current_team))
        FakeClientCommand(pBot->pEdict, "detentryteleporter", NULL, NULL);

    // is the bot's teleporter exit now in an unavailable map area?
    if(pBot->tpExitWP != -1 && !FNullEnt(pBot->tpExit) && !WaypointAvailable(pBot->tpExitWP, pBot->current_team))
        FakeClientCommand(pBot->pEdict, "detexitteleporter", NULL, NULL);

    /*	// Sentry gun still alive?
            if(pBot->has_sentry == TRUE
                    && (pBot->sentry_edict == NULL
                    || FNullEnt(pBot->sentry_edict)))
            {
                    pBot->has_sentry = FALSE;
                    pBot->sentry_edict = NULL;
            }*/

    job_struct* newJob;

    // time to build a dispenser?
    if(pBot->has_dispenser == FALSE && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 139 &&
        random_long(1, 1000) < 400) {
        newJob = InitialiseNewJob(pBot, JOB_BUILD_DISPENSER);
        if(newJob != NULL) {
            newJob->waypoint = BotGetDispenserBuildWaypoint(pBot);

            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_BUILD_DISPENSER, newJob) == TRUE)
                return;
        }
    }

    // time to build a sentry?
    if(pBot->has_sentry == FALSE && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 139) {
        newJob = InitialiseNewJob(pBot, JOB_BUILD_SENTRY);
        if(newJob != NULL) {
            newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_SENTRY);

            if(newJob->waypoint == -1)
                newJob->waypoint = WaypointFindRandomGoal(pBot->current_wp, -1, W_FL_TFC_SENTRY);

            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_BUILD_SENTRY, newJob) == TRUE)
                return;
        }
    }

    // repair/upgrade the bot's sentry?
    if(pBot->has_sentry == TRUE && pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 139 &&
        pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] > 10) {
        char modelName[30];
        strncpy(modelName, STRING(pBot->sentry_edict->v.model), 30);
        modelName[29] = '\0';

        if(pBot->sentry_ammo < 100 || pBot->sentry_edict->v.health < 100 ||
            strcmp(modelName, "models/sentry3.mdl") != 0) {
            newJob = InitialiseNewJob(pBot, JOB_MAINTAIN_OBJECT);
            if(newJob != NULL) {
                newJob->object = pBot->sentry_edict;

                if(SubmitNewJob(pBot, JOB_MAINTAIN_OBJECT, newJob) == TRUE)
                    return;
            }
        }
    }

    // detonate the bots teleporter exit if it doesn't have an teleporter entrance
    // i.e. build entrances and then build exits, not the other way round
    // this is because each map has a limited number of teleporter waypoints
    if(FNullEnt(pBot->tpEntrance) && !FNullEnt(pBot->tpExit)) {
        //	UTIL_BotLogPrintf("%s, detting my exit\n", pBot->name);
        FakeClientCommand(pBot->pEdict, "detexitteleporter", NULL, NULL);
    }

    // build a teleporter entrance?
    if(FNullEnt(pBot->tpEntrance) && bot_can_build_teleporter == TRUE &&
        pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 190 && random_long(1, 1000) < 101) {
        newJob = InitialiseNewJob(pBot, JOB_BUILD_TELEPORT);
        if(newJob != NULL) {
            newJob->waypoint = BotGetTeleporterBuildWaypoint(pBot, TRUE);

            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_BUILD_TELEPORT, newJob) == TRUE)
                return;
        }
    }

    // build a teleporter exit?
    // only do so if the bot owns a Teleporter entrance already
    if(!FNullEnt(pBot->tpEntrance) && FNullEnt(pBot->tpExit) && bot_can_build_teleporter == TRUE &&
        pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 190 && random_long(1, 1000) < 101) {
        newJob = InitialiseNewJob(pBot, JOB_BUILD_TELEPORT);
        if(newJob != NULL) {
            newJob->waypoint = BotGetTeleporterBuildWaypoint(pBot, FALSE);

            if(newJob->waypoint != -1 && SubmitNewJob(pBot, JOB_BUILD_TELEPORT, newJob) == TRUE)
                return;
        }
    }
}
