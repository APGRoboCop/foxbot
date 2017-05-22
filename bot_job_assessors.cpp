//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_job_assessors.h
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

// This file should contain only job assessor functions, one for each job type.
// Assessor functions are useful because many types of jobs can vary in terms
// of their priority level.  For example, the job of picking up a discarded
// back pack will be less important if it is now quite far away.

#include "extdll.h"
#include "bot.h"
#include <meta_api.h>
#include "cbase.h"
#include "tf_defs.h"

#include "list.h"

#include "bot_func.h"

#include "bot_job_think.h"
#include "bot_job_assessors.h"
#include "bot_weapons.h"
#include "waypoint.h"
#include "bot_navigate.h"

extern bot_t bots[32];
extern bot_weapon_t weapon_defs[MAX_WEAPONS];

// bot settings //////////////////
extern bool bot_can_build_teleporter;

extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints; // number of waypoints currently in use

// assessment function for the priority of a JOB_SEEK_WAYPOINT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobSeekWaypoint(const bot_t* pBot, const job_struct& r_job)
{
    // check the jobs validity
    if(r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // this actually denotes success if it happens!
    if(pBot->current_wp > -1 && pBot->current_wp < num_waypoints)
        return PRIORITY_NONE;

    return jl[JOB_SEEK_WAYPOINT].basePriority;
}

// assessment function for the priority of a JOB_GET_UNSTUCK job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobGetUnstuck(const bot_t* pBot, const job_struct& r_job)
{
    // check the jobs validity
    if(r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    return jl[JOB_GET_UNSTUCK].basePriority;
}

// assessment function for the priority of a JOB_ROAM job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobRoam(const bot_t* pBot, const job_struct& r_job)
{
    // check the jobs validity
    if(r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // don't check the waypoints validity here
    // this job kicks in if no other job is working(e.g. because of route failure)
    // let the job itself try to cope with route failure

    return jl[JOB_ROAM].basePriority;
}

// assessment function for the priority of a JOB_CHAT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobChat(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.phase == 0 && (r_job.f_bufferedTime + 12.0) < pBot->f_think_time) {
        return PRIORITY_NONE;
    }

    return jl[JOB_CHAT].basePriority;
}

// assessment function for the priority of a JOB_REPORT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobReport(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.phase == 0 && (r_job.f_bufferedTime + 10.0) < pBot->f_think_time) {
        return PRIORITY_NONE;
    }

    return jl[JOB_REPORT].basePriority;
}

// assessment function for the priority of a JOB_PICKUP_ITEM job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobPickUpItem(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(FNullEnt(r_job.object) || r_job.object->v.flags & FL_KILLME || r_job.object->v.effects & EF_NODRAW ||
        r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // drop the job if the item is unneeded
    if(pBot->ammoStatus == AMMO_UNNEEDED && PlayerHealthPercent(pBot->pEdict) > 99 &&
        PlayerArmorPercent(pBot->pEdict) > 99) {
        //	UTIL_HostSay(pBot->pEdict, 0, "forgetting item"); //DebugMessageOfDoom!
        return PRIORITY_NONE;
    }

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team))
        return PRIORITY_NONE;

    // make sure the bot can reach the item and that it isn't too far away
    const int routeDistance = WaypointDistanceFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team);
    if(routeDistance == -1 || routeDistance > 1400)
        return PRIORITY_NONE;

    // has the object moved?
    // for some reason some objects continue to exist, but become impossible to locate
    // also some objects can fly through the air(e.g. backpacks)
    if(!VectorsNearerThan(r_job.object->v.origin, r_job.origin, 100.0))
        return PRIORITY_NONE;

    return jl[JOB_PICKUP_ITEM].basePriority;
}

// assessment function for the priority of a JOB_PICKUP_FLAG job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobPickUpFlag(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->bot_has_flag == TRUE || FNullEnt(r_job.object) ||
        r_job.object->v.owner != NULL // i.e. someone is carrying it
        || (r_job.f_bufferedTime + 300.0) < pBot->f_think_time)
        return PRIORITY_NONE;

    // if the bot is guarding this dropped flag already don't pick it up
    // this check allows the bot to pick up other flags it might see whilst defending
    // a dropped flag
    const int defendJobIndex = BufferedJobIndex(pBot, JOB_DEFEND_FLAG);
    if(defendJobIndex != -1 && pBot->job[defendJobIndex].object == r_job.object)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    // abort if the flag has moved significantly
    if(!VectorsNearerThan(r_job.object->v.origin, r_job.origin, 200.0))
        return PRIORITY_NONE;

    return jl[JOB_PICKUP_FLAG].basePriority;
}

// assessment function for the priority of a JOB_PUSH_BUTTON job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobPushButton(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(FNullEnt(r_job.object) || (pBot->f_use_button_time + 3.0) >= pBot->f_think_time ||
        r_job.f_bufferedTime < pBot->f_killed_time || (r_job.f_bufferedTime + 15.0) < pBot->f_think_time) {
        return PRIORITY_NONE;
    }

    // BModels have 0,0,0 for origin so we must use VecBModelOrigin()
    const Vector buttonOrigin = VecBModelOrigin(r_job.object);

    // not near enough?
    if(!VectorsNearerThan(pBot->pEdict->v.origin, buttonOrigin, 250.0))
        return PRIORITY_NONE;

    const Vector vecStart = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
    TraceResult tr;
    UTIL_TraceLine(vecStart, buttonOrigin, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

    // make sure the button is visible
    if(!(tr.flFraction >= 1.0 || tr.pHit == r_job.object))
        return PRIORITY_NONE;

    return jl[JOB_PUSH_BUTTON].basePriority;
}

// assessment function for the priority of a JOB_USE_TELEPORT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobUseTeleport(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->bot_has_flag || PlayerIsInfected(pBot->pEdict) || r_job.f_bufferedTime < pBot->f_killed_time ||
        FNullEnt(r_job.object) || !IsAlive(r_job.object) // cursed undead zombie-limbo teleports
        || r_job.object->v.flags & FL_KILLME) {
        return PRIORITY_NONE;
    }

    // make sure the teleporter is still a teleporter, just to be reeeaally sure
    char classname[24];
    strncpy(classname, STRING(r_job.object->v.classname), 24);
    classname[23] = '\0';
    if(strcmp(classname, "building_teleporter") != 0)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    // we have to check if this job is still relevant
    // first we need to know if this job is being used to find out where a Teleport goes to
    const int teleportGoal = r_job.waypointTwo; // ultimate teleport destination
    const int teleIndex = BotRecallTeleportEntranceIndex(pBot, r_job.object);
    if(teleIndex != -1 && teleportGoal > -1 && pBot->telePair[teleIndex].exitWP != -1) {
        // the bot knows where this teleport goes, so we need to know if the most
        // important job in the buffer goes where the teleport will send the bot
        int bestIndex = -1;
        int bestPriority = PRIORITY_NONE;
        for(int i = 0; i < JOB_BUFFER_MAX; i++) {
            if(pBot->jobType[i] > JOB_NONE && pBot->jobType[i] != JOB_USE_TELEPORT // ignore this job
                && pBot->job[i].priority > bestPriority) {
                bestIndex = i;
                bestPriority = pBot->job[i].priority;
            }
        }

        // does the most important job in the buffer not go where the teleport job goes?
        if(bestIndex != -1 &&
            WaypointDistanceFromTo(teleportGoal, pBot->job[bestIndex].waypoint, pBot->current_team) > 500) {
            return PRIORITY_NONE; // this teleport job is no longer relevant enough
        }
    }

    return jl[JOB_USE_TELEPORT].basePriority;
}

// assessment function for the priority of a JOB_MAINTAIN_OBJECT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobMaintainObject(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->pEdict->v.playerclass != TFC_CLASS_ENGINEER || pBot->bot_has_flag || FNullEnt(r_job.object) ||
        !IsAlive(r_job.object) // NO... it can't be - it's dead!
        || r_job.object->v.flags & FL_KILLME) {
        //	UTIL_HostSay(pBot->pEdict, 0, "JOB_MAINTAIN_OBJECT no object");  //DebugMessageOfDoom!
        return PRIORITY_NONE;
    }

    // not enough ammo for repairs?
    if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SHOTGUN].iAmmo1] < 2 &&
        pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 140) {
        //	UTIL_HostSay(pBot->pEdict, 0, "JOB_MAINTAIN_OBJECT no ammo");  //DebugMessageOfDoom!
        return PRIORITY_NONE;
    }

    // check the waypoints validity
    if(r_job.phase > 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                              WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)) {
        //	UTIL_HostSay(pBot->pEdict, 0, "JOB_MAINTAIN_OBJECT no route");  //DebugMessageOfDoom!
        return PRIORITY_NONE;
    }

    return jl[JOB_MAINTAIN_OBJECT].basePriority;
}

// assessment function for the priority of a JOB_BUILD_SENTRY job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobBuildSentry(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->pEdict->v.playerclass != TFC_CLASS_ENGINEER)
        return PRIORITY_NONE;

    // check the conditions at job start
    if(r_job.phase == 0) {
        if(pBot->has_sentry == TRUE || pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 140)
            return PRIORITY_NONE;
    }

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    // if the job is in the final phase(waiting for building to finish)
    // make the job uninterruptable
    if(r_job.phase == 2)
        return PRIORITY_MAXIMUM;

    return jl[JOB_BUILD_SENTRY].basePriority;
}

// assessment function for the priority of a JOB_BUILD_DISPENSER job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobBuildDispenser(const bot_t* pBot, const job_struct& r_job)
{
    // check the conditions at job start
    if(r_job.phase == 0) {
        if(pBot->has_dispenser == TRUE || pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 140)
            return PRIORITY_NONE;
    }

    // recommend the job be removed if it is invalid
    if(pBot->pEdict->v.playerclass != TFC_CLASS_ENGINEER || r_job.f_bufferedTime <= pBot->f_killed_time)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    // if the job is in the final phase(waiting for building to finish)
    // make the job uninterruptable
    if(r_job.phase == 2)
        return PRIORITY_MAXIMUM;

    return jl[JOB_BUILD_DISPENSER].basePriority;
}

// assessment function for the priority of a JOB_BUILD_TELEPORT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobBuildTeleport(const bot_t* pBot, const job_struct& r_job)
{
    // check the conditions at job start
    if(r_job.phase == 0) {
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 140)
            return PRIORITY_NONE;
    }

    // recommend the job be removed if it is invalid
    if(bot_can_build_teleporter == FALSE || pBot->pEdict->v.playerclass != TFC_CLASS_ENGINEER ||
        r_job.f_bufferedTime <= pBot->f_killed_time)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    // if the job is in the final phase(waiting for building to finish)
    // make the job uninterruptable
    if(r_job.phase == 2)
        return PRIORITY_MAXIMUM;

    return jl[JOB_BUILD_TELEPORT].basePriority;
}

// assessment function for the priority of a JOB_BUFF_ALLY job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobBuffAlly(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(FNullEnt(r_job.player) || !IsAlive(r_job.player) || r_job.f_bufferedTime < pBot->f_killed_time ||
        (r_job.f_bufferedTime + 30.0) < pBot->f_think_time)
        return PRIORITY_NONE;

    // a metal wrench doth not cureth the contagion
    if(pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER) {
        if(pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] < 20 // need ammo too
            || PlayerIsInfected(r_job.player) || PlayerArmorPercent(r_job.player) > 99)
            return PRIORITY_NONE;
    }

    // check the waypoints validity
    // and to see if the patient is too far away
    if(r_job.phase > 0) {
        const int routeDistance = WaypointDistanceFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team);

        if(!WaypointAvailable(r_job.waypoint, pBot->current_team) || routeDistance == -1 || routeDistance > 1500)
            return PRIORITY_NONE;
    }

    return jl[JOB_BUFF_ALLY].basePriority;
}

// assessment function for the priority of a JOB_ESCORT_ALLY job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobEscortAlly(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(FNullEnt(r_job.player) || !IsAlive(r_job.player))
        return PRIORITY_NONE;

    // make sure the escorted doesn't have too many bot escorts
    int escortCount = 0;
    int escortIndex;
    for(int i = 0; i < 32; i++) {
        if(bots[i].is_used && bots[i].current_team == pBot->current_team) {
            // only count escorts who are near enough to the escorted player
            escortIndex = BufferedJobIndex(&bots[i], JOB_ESCORT_ALLY);
            if(escortIndex != -1 && bots[i].job[escortIndex].player == r_job.player &&
                VectorsNearerThan(bots[i].pEdict->v.origin, r_job.player->v.origin, 600.0)) {
                ++escortCount;

                if(escortCount > 2)
                    return PRIORITY_NONE;
            }
        }
    }

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1))
        return PRIORITY_NONE;

    return jl[JOB_ESCORT_ALLY].basePriority;
}

// assessment function for the priority of a JOB_CALL_MEDIC job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobCallMedic(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(!FNullEnt(pBot->enemy.ptr) || r_job.f_bufferedTime < pBot->f_killed_time ||
        pBot->pEdict->v.waterlevel > WL_FEET_IN_WATER || PlayerHealthPercent(pBot->pEdict) > 99)
        return PRIORITY_NONE;

    // don't wait for a medic somewhere inappropriate
    if(pBot->current_wp > -1 &&
        (waypoints[pBot->current_wp].flags & W_FL_LIFT || waypoints[pBot->current_wp].flags & W_FL_LADDER ||
            waypoints[pBot->current_wp].flags & W_FL_JUMP))
        return PRIORITY_NONE;

    // try to limit this job to two bot teammates per general area
    // i.e. avoid overloading medics with medic calls
    if(r_job.phase == 0) {
        int cryBabies = 0;
        for(int i = 0; i < MAX_BOTS; i++) {
            if(bots[i].is_used && bots[i].currentJob > -1 && bots[i].jobType[bots[i].currentJob] == JOB_CALL_MEDIC &&
                bots[i].current_team == pBot->current_team && &bots[i] != pBot // make sure the player isn't THIS bot
                && VectorsNearerThan(bots[i].pEdict->v.origin, pBot->pEdict->v.origin, 900.0)) {
                ++cryBabies;

                if(cryBabies > 1)
                    return PRIORITY_NONE;
            }
        }
    }

    // boost the priority if infected(just above that of JOB_INFECTED_ATTACK)
    if(PlayerIsInfected(pBot->pEdict))
        return (jl[JOB_INFECTED_ATTACK].basePriority + 1);

    return jl[JOB_CALL_MEDIC].basePriority;
}

// assessment function for the priority of a JOB_GET_HEALTH job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobGetHealth(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(PlayerHealthPercent(pBot->pEdict) > 99)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team))
        return PRIORITY_NONE;

    // make sure the journey isn't too long(job may have been asleep awhile)
    const int routeDistance = WaypointDistanceFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team);
    if(routeDistance == -1 || routeDistance > 4000)
        return PRIORITY_NONE;

    return jl[JOB_GET_HEALTH].basePriority;
}

// assessment function for the priority of a JOB_GET_ARMOR job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobGetArmor(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(PlayerArmorPercent(pBot->pEdict) > 99)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team))
        return PRIORITY_NONE;

    // make sure the journey isn't too long(job may have been asleep awhile)
    const int routeDistance = WaypointDistanceFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team);
    if(routeDistance == -1 || routeDistance > 4000)
        return PRIORITY_NONE;

    return jl[JOB_GET_ARMOR].basePriority;
}

// assessment function for the priority of a JOB_GET_AMMO job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobGetAmmo(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->ammoStatus == AMMO_UNNEEDED)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team))
        return PRIORITY_NONE;

    // make sure the journey isn't too long(job may have been asleep awhile)
    const int routeDistance = WaypointDistanceFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team);
    if(routeDistance == -1 || routeDistance > 5000)
        return PRIORITY_NONE;

    return jl[JOB_GET_AMMO].basePriority;
}

// assessment function for the priority of a JOB_DISGUISE job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobDisguise(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->visEnemyCount > 0 || pBot->bot_has_flag || r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(r_job.phase > 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                              WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1))
        return PRIORITY_NONE;

    return jl[JOB_DISGUISE].basePriority;
}

// assessment function for the priority of a JOB_FEIGN_AMBUSH job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobFeignAmbush(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->bot_has_flag || r_job.f_bufferedTime < pBot->f_killed_time ||
        (pBot->f_injured_time + 1.0) > pBot->f_think_time || pBot->nadePrimed == TRUE || PlayerIsInfected(pBot->pEdict))
        return PRIORITY_NONE;

    // the job can't be left to sleep, it's best done where it was triggered
    if(r_job.phase == 0 && (r_job.f_bufferedTime + 1.0) < pBot->f_think_time)
        return PRIORITY_NONE;

    // check if the bot is in an unsuitable location
    if(pBot->pEdict->v.waterlevel != WL_NOT_IN_WATER ||
        (pBot->current_wp > -1 && waypoints[pBot->current_wp].flags & W_FL_LIFT))
        return PRIORITY_NONE;

    return jl[JOB_FEIGN_AMBUSH].basePriority;
}

// assessment function for the priority of a JOB_SNIPE job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobSnipe(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->pEdict->v.playerclass != TFC_CLASS_SNIPER || r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    return jl[JOB_SNIPE].basePriority;
}

// assessment function for the priority of a JOB_GUARD_WAYPOINT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobGuardWaypoint(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->mission != ROLE_DEFENDER ||
        (pBot->pEdict->v.playerclass != TFC_CLASS_SOLDIER && pBot->pEdict->v.playerclass != TFC_CLASS_HWGUY &&
            pBot->pEdict->v.playerclass != TFC_CLASS_PYRO && pBot->pEdict->v.playerclass != TFC_CLASS_DEMOMAN))
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    return jl[JOB_GUARD_WAYPOINT].basePriority;
}

// assessment function for the priority of a JOB_DEFEND_FLAG job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobDefendFlag(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(FNullEnt(r_job.object) || pBot->bot_has_flag)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1))
        return PRIORITY_NONE;

    // abort if the flag has moved significantly
    if(!VectorsNearerThan(r_job.object->v.origin, r_job.origin, 200.0))
        return PRIORITY_NONE;

    // see if this flag will have too many defenders
    if(r_job.phase == 0) {
        int DefendJobIndex;
        int defenderTotal = 1; // 1 = this bot
        for(int i = 0; i < MAX_BOTS; i++) {
            if(bots[i].is_used && bots[i].current_team == pBot->current_team &&
                &bots[i] != pBot) // make sure the player isn't THIS bot
            {
                DefendJobIndex = BufferedJobIndex(&bots[i], JOB_DEFEND_FLAG);
                if(DefendJobIndex != -1 && bots[i].job[DefendJobIndex].object == r_job.object) {
                    ++defenderTotal;

                    if(defenderTotal > 1)
                        return PRIORITY_NONE;
                }
            }
        }
    }

    return jl[JOB_DEFEND_FLAG].basePriority;
}

// assessment function for the priority of a JOB_GET_FLAG job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobGetFlag(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->mission == ROLE_DEFENDER || pBot->bot_has_flag)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    return jl[JOB_GET_FLAG].basePriority;
}

// assessment function for the priority of a JOB_CAPTURE_FLAG job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobCaptureFlag(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->bot_has_flag == FALSE)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1))
        return PRIORITY_NONE;

    return jl[JOB_CAPTURE_FLAG].basePriority;
}

// assessment function for the priority of a JOB_HARRASS_DEFENSE job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobHarrassDefense(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->mission == ROLE_DEFENDER || pBot->bot_has_flag || pBot->pEdict->v.playerclass == TFC_CLASS_SCOUT ||
        pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(WaypointAvailable(r_job.waypoint, pBot->current_team) &&
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    return jl[JOB_HARRASS_DEFENSE].basePriority;
}

// assessment function for the priority of a JOB_ROCKET_JUMP job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobRocketJump(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.f_bufferedTime < pBot->f_killed_time ||
        pBot->m_rgAmmo[weapon_defs[TF_WEAPON_RPG].iAmmo1] < 4 // got enough rockets?
        || (r_job.phase == 0 && (r_job.f_bufferedTime + 0.5) < pBot->f_think_time) ||
        pBot->pEdict->v.waterlevel > WL_FEET_IN_WATER || r_job.waypoint < 0) {
        //	UTIL_HostSay(pBot->pEdict,0,"JOB_ROCKET_JUMP invalid"); //DebugMessageOfDoom!
        return PRIORITY_NONE;
    }

    // once initiated this job should not be interrupted
    return PRIORITY_MAXIMUM;
}

// assessment function for the priority of a JOB_CONCUSSION_JUMP job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobConcussionJump(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.f_bufferedTime < pBot->f_killed_time || (r_job.f_bufferedTime + 10.0) < pBot->f_think_time // took too long
        || (r_job.phase == 0 && (r_job.f_bufferedTime + 0.5) < pBot->f_think_time)) {
        return PRIORITY_NONE;
    }

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team))
        return PRIORITY_NONE;

    // once initiated this job should not be interrupted
    return PRIORITY_MAXIMUM;
}

// assessment function for the priority of a JOB_DETPACK_WAYPOINT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobDetpackWaypoint(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->pEdict->v.playerclass != TFC_CLASS_DEMOMAN || (r_job.phase == 0 && pBot->detpack != 2))
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    // if the bot is running away from the detpack increase the jobs priority
    if(r_job.phase == 2)
        return 700;

    return jl[JOB_DETPACK_WAYPOINT].basePriority;
}

// assessment function for the priority of a JOB_PIPETRAP job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobPipetrap(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->pEdict->v.playerclass != TFC_CLASS_DEMOMAN || pBot->mission != ROLE_DEFENDER || pBot->enemy.ptr != NULL)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    return jl[JOB_PIPETRAP].basePriority;
}

// assessment function for the priority of a JOB_INVESTIGATE_AREA job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobInvestigateArea(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) || r_job.f_bufferedTime < pBot->f_killed_time ||
        (r_job.f_bufferedTime + 15.0) < pBot->f_think_time)
        return PRIORITY_NONE;

    // recommend the job be removed if the waypoint can't be reached or is too far away
    const int routeDistance = WaypointDistanceFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team);
    if(routeDistance == -1 || routeDistance > 1700) {
        return PRIORITY_NONE;
    }

    return jl[JOB_INVESTIGATE_AREA].basePriority;
}

// assessment function for the priority of a JOB_PURSUE_ENEMY job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobPursueEnemy(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(FNullEnt(r_job.player) || !IsAlive(r_job.player))
        return PRIORITY_NONE;

    // still pursue the enemy after being killed? (check once after dying)
    if(r_job.f_bufferedTime < pBot->f_killed_time && (pBot->f_think_time + 3.1) > pBot->f_killed_time &&
        pBot->f_periodicAlert3 < pBot->f_think_time) {
        // know where to go?
        if(!WaypointAvailable(r_job.waypoint, pBot->current_team))
            return PRIORITY_NONE;

        int routeDistance = WaypointDistanceFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team);

        if(PlayerHasFlag(r_job.player)) {
            // forget flag carriers if they are now far away
            if(routeDistance > 3000)
                return PRIORITY_NONE;
        } else {
            if(pBot->mission == ROLE_DEFENDER) {
                // stop defenders being drawn too far out
                if(routeDistance > 2000)
                    return PRIORITY_NONE;
            }
            // ignore non flag carriers if they are far away or just randomly
            else if(routeDistance > 3000 || random_long(1, 1000) < 501)
                return PRIORITY_NONE;
        }
    }

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1))
        return PRIORITY_NONE;

    return jl[JOB_PURSUE_ENEMY].basePriority;
}

// assessment function for the priority of a JOB_PATROL_HOME job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobPatrolHome(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->mission != ROLE_DEFENDER)
        return PRIORITY_NONE;

    // allow the bot to get bored of patrolling, so that it can rethink
    // what it wants to do(maybe even start a new career in patrolling again!)
    if((r_job.f_bufferedTime + 60.0) < pBot->f_think_time && pBot->f_periodicAlert3 < pBot->f_think_time &&
        random_long(1, 1000) < 200)
        return PRIORITY_NONE;

    return jl[JOB_PATROL_HOME].basePriority;
}

// assessment function for the priority of a JOB_SPOT_STIMULUS job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobSpotStimulus(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->visEnemyCount > 0 || (r_job.f_bufferedTime + 5.0) < pBot->f_think_time ||
        r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // don't interrupt the bot if it's climbing a ladder,
    // unless it's already stuck on the ladder
    if(pBot->pEdict->v.movetype == MOVETYPE_FLY &&
        (pBot->pEdict->v.velocity.z > 5.0 || pBot->pEdict->v.velocity.z < -5.0))
        return PRIORITY_NONE;

    return jl[JOB_SPOT_STIMULUS].basePriority;
}

// assessment function for the priority of a JOB_ATTACK_BREAKABLE job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobAttackBreakable(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(FNullEnt(r_job.object) || r_job.object->v.flags & FL_KILLME || r_job.object->v.health < 0 ||
        pBot->enemy.ptr != NULL || r_job.f_bufferedTime < pBot->f_killed_time || pBot->ammoStatus <= AMMO_LOW)
        return PRIORITY_NONE;

    // too far away to be concerned with?
    const Vector entity_origin = VecBModelOrigin(r_job.object);
    if(!VectorsNearerThan(pBot->pEdict->v.origin, entity_origin, 500.0))
        return PRIORITY_NONE;

    return jl[JOB_ATTACK_BREAKABLE].basePriority;
}

// assessment function for the priority of a JOB_ATTACK_TELEPORT job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobAttackTeleport(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->enemy.ptr != NULL || pBot->ammoStatus < AMMO_UNNEEDED || pBot->bot_has_flag || FNullEnt(r_job.object) ||
        !IsAlive(r_job.object) // the dead somehow live on...
        || r_job.object->v.flags & FL_KILLME || pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN ||
        pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER || r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1))
        return PRIORITY_NONE;

    return jl[JOB_ATTACK_TELEPORT].basePriority;
}

// assessment function for the priority of a JOB_SEEK_BACKUP job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobSeekBackup(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.f_bufferedTime < pBot->f_killed_time ||
        (r_job.phase == 0 // abort if the job has been asleep in the buffer for too long
            && (r_job.f_bufferedTime + 4.0) < pBot->f_think_time))
        return PRIORITY_NONE;

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)) {
        return PRIORITY_NONE;
    }

    return jl[JOB_SEEK_BACKUP].basePriority;
}

// assessment function for the priority of a JOB_AVOID_ENEMY job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobAvoidEnemy(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.f_bufferedTime < pBot->f_killed_time ||
        (r_job.phase == 0 // abort if the job has been asleep in the buffer for too long
            && (r_job.f_bufferedTime + 4.0) < pBot->f_think_time))
        return PRIORITY_NONE;

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)) {
        //	UTIL_HostSay(pBot->pEdict, 0, "JOB_AVOID_ENEMY - bad path"); //DebugMessageOfDoom!
        return PRIORITY_NONE;
    }

    return jl[JOB_AVOID_ENEMY].basePriority;
}

// assessment function for the priority of a JOB_AVOID_AREA_DAMAGE job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobAvoidAreaDamage(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.f_bufferedTime < pBot->f_killed_time ||
        (r_job.phase == 0 // abort if the job has been asleep in the buffer for too long
            && (r_job.f_bufferedTime + 2.0) < pBot->f_think_time))
        return PRIORITY_NONE;

    // has the threatening object ceased to exist?
    if(FNullEnt(r_job.object) || r_job.object->v.flags & FL_KILLME ||
        !VectorsNearerThan(r_job.object->v.origin, r_job.origin, 200.0))
        return PRIORITY_NONE;

    return jl[JOB_AVOID_AREA_DAMAGE].basePriority;
}

// assessment function for the priority of a JOB_INFECTED_ATTACK job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobInfectedAttack(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.f_bufferedTime < pBot->f_killed_time || pBot->bot_has_flag || PlayerIsInfected(pBot->pEdict) == FALSE)
        return PRIORITY_NONE;

    return jl[JOB_INFECTED_ATTACK].basePriority;
}

// assessment function for the priority of a JOB_BIN_GRENADE job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobBinGrenade(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if((r_job.phase == 0 && pBot->nadePrimed == FALSE) || pBot->enemy.ptr != NULL ||
        r_job.f_bufferedTime < pBot->f_killed_time || (r_job.f_bufferedTime + 5.0) < pBot->f_think_time) {
        return PRIORITY_NONE;
    }

    return jl[JOB_BIN_GRENADE].basePriority;
}

// assessment function for the priority of a JOB_DROWN_RECOVER job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobDrownRecover(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->pEdict->v.waterlevel < WL_HEAD_IN_WATER || r_job.f_bufferedTime < pBot->f_killed_time)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
        WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1)
        return PRIORITY_NONE;

    return jl[JOB_DROWN_RECOVER].basePriority;
}

// assessment function for the priority of a JOB_MELEE_WARRIOR job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobMeleeWarrior(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(r_job.f_bufferedTime < pBot->f_killed_time || pBot->enemy.seenWithFlag || pBot->bot_has_flag)
        return PRIORITY_NONE;

    // check the waypoints validity
    if(r_job.phase != 0 && (!WaypointAvailable(r_job.waypoint, pBot->current_team) ||
                               WaypointRouteFromTo(pBot->current_wp, r_job.waypoint, pBot->current_team) == -1))
        return PRIORITY_NONE;

    return jl[JOB_MELEE_WARRIOR].basePriority;
}

// assessment function for the priority of a JOB_GRAFFITI_ARTIST job.
// r_job can be a job you wish to add to the buffer or an existing job.
int assess_JobGraffitiArtist(const bot_t* pBot, const job_struct& r_job)
{
    // recommend the job be removed if it is invalid
    if(pBot->bot_has_flag || pBot->visEnemyCount > 0 || r_job.f_bufferedTime < pBot->f_killed_time ||
        (r_job.f_bufferedTime + 25.0) < pBot->f_think_time || PlayerIsInfected(pBot->pEdict)) {
        return PRIORITY_NONE;
    }

    return jl[JOB_GRAFFITI_ARTIST].basePriority;
}
