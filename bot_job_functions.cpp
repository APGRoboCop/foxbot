//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_job_functions.cpp
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

extern chatClass chat; // bot chat stuff

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
extern int bot_allow_humour;
extern bool offensive_chatter;
extern bool defensive_chatter;

extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints; // number of waypoints currently in use

// area defs...
extern AREA areas[MAX_WAYPOINTS];
extern int num_areas;

// This function handles bot behaviour for the JOB_SEEK_WAYPOINT job.
// i.e. the bot can't find a waypoint to move towards, so try to move
// about until the bot can find one again.
int JobSeekWaypoint(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // never ignore enemies even though the bot is lost somewhere
    if(pBot->enemy.ptr != NULL) {
        BotSetFacing(pBot, pBot->enemy.ptr->v.origin);
        BotNavigateWaypointless(pBot);
        return JOB_UNDERWAY;
    }

    // if on a ladder generally climb upwards(think like a spider!)
    if(pBot->pEdict->v.movetype == MOVETYPE_FLY) {
        // randomly jump off occasionally
        if(pBot->f_periodicAlert3 < pBot->f_think_time && random_long(1, 1000) < 101) {
            pBot->pEdict->v.button = 0; // in case IN_FORWARD is active
            pBot->pEdict->v.button |= IN_JUMP;
            pBot->f_pause_time = pBot->f_think_time + 1.0; // so the jump will work
            return JOB_UNDERWAY;
        }

        TraceResult tr;
        UTIL_TraceLine(pBot->pEdict->v.origin, pBot->pEdict->v.origin + Vector(0, 0, 40.0), dont_ignore_monsters,
            pBot->pEdict->v.pContainingEntity, &tr);

        // if there is space above the bots head climb upwards
        if(tr.flFraction >= 1.0) {
            pBot->pEdict->v.idealpitch = 90;
            BotChangePitch(pBot->pEdict, 99999);
        }
    }

    // phase zero - try checking for a current waypoint first
    if(job_ptr->phase == 0) {
        BotFindCurrentWaypoint(pBot);

        job_ptr->phase = 1;
        return JOB_UNDERWAY;
    }

    // phase 1 - pick a direction for the bot to wander in
    if(job_ptr->phase == 1) {
        const Vector newAngle = Vector(0.0, random_float(-180.0, 180.0), 0.0);
        UTIL_MakeVectors(newAngle);

        const Vector v_forwards = pBot->pEdict->v.origin + (gpGlobals->v_forward * 1000.0);

        // not the same direction the bot is facing already
        if(!FInViewCone(v_forwards, pBot->pEdict)) {
            job_ptr->origin = v_forwards;
            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + random_float(1.5, 4.0);
        } else
            pBot->f_move_speed = pBot->f_max_speed / 2; // slow down silver!
    }

    // phase 2 - wander in a straight line for a few seconds
    if(job_ptr->phase == 2) {
        BotSetFacing(pBot, job_ptr->origin);
        BotNavigateWaypointless(pBot);
        pBot->pEdict->v.button |= IN_FORWARD; // in case bot is on a ladder

        if(pBot->pEdict->v.waterlevel == WL_HEAD_IN_WATER) {
            // always swim up if injured to avoid drowning
            if(PlayerHealthPercent(pBot->pEdict) < 100)
                pBot->f_vertical_speed = pBot->f_max_speed;
            else
                pBot->f_vertical_speed = 0.0; // neutral, see what happens
        }

        // wandered long enough or gotten stuck?
        if(job_ptr->phase_timer < pBot->f_think_time ||
            (pBot->f_move_speed > 5.0 && pBot->pEdict->v.velocity.Length() < 0.1)) {
            job_ptr->phase = 0;
            return JOB_UNDERWAY;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_GET_UNSTUCK job.
// i.e. the bot has unintentionally stopped moving so try moving in a random
// direction so that it may get unstuck.
int JobGetUnstuck(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase 0 - look for an open area to move towards
    if(job_ptr->phase == 0) {
        // if on a ladder jump off
        if(pBot->pEdict->v.movetype == MOVETYPE_FLY) {
            pBot->pEdict->v.button = 0; // in case IN_FORWARD is active
            pBot->pEdict->v.button |= IN_JUMP;
            pBot->f_pause_time = pBot->f_think_time + 1.0; // so the jump will work
            job_ptr->phase = 0;                            // back to square one
        }

        // try a few random directions, looking for an open area
        for(int i = 0; i < 8; i++) {
            Vector newAngle = Vector(0.0, random_float(-180.0, 180.0), 0.0);
            UTIL_MakeVectors(newAngle);

            // do a trace 300 units ahead of the new view angle to check for sight barriers
            const Vector v_forwards = pBot->pEdict->v.origin + (gpGlobals->v_forward * 300.0);

            TraceResult tr;
            UTIL_TraceLine(
                pBot->pEdict->v.origin, v_forwards, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

            // if the new view angle is clear or a blockage is not too near
            if(tr.flFraction >= 1.0 || !VectorsNearerThan(pBot->pEdict->v.origin, tr.vecEndPos, 80.0)) {
                job_ptr->origin = tr.vecEndPos; // remember where to go
                job_ptr->phase = 1;
                job_ptr->phase_timer = pBot->f_think_time + random_float(1.0, 3.0);

                //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                //		job_ptr->origin, 10, 2, 250, 250, 250, 200, 10);

                return JOB_UNDERWAY;
            }
        }

        return JOB_TERMINATED; // couldn't find a clear area nearby
    }

    // phase 1 - wander in a straight line for a second or two
    if(job_ptr->phase == 1) {
        BotSetFacing(pBot, job_ptr->origin);
        pBot->f_move_speed = pBot->f_max_speed;
        pBot->f_side_speed = 0.0;
        pBot->f_pause_time = 0.0;

        // in case bot connects with a ladder or movable crate
        pBot->pEdict->v.button |= IN_FORWARD;

        // jump occasionally, it might help(e.g. over hard to detect obstacles)
        if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) < 334)
            pBot->pEdict->v.button |= IN_JUMP;

        // wandered long enough?
        if(job_ptr->phase_timer < pBot->f_think_time || (pBot->pEdict->v.origin - job_ptr->origin).Length2D() < 20.0) {
            BlacklistJob(pBot, JOB_GET_UNSTUCK, 1.0); // don't get caught in a loop
            return JOB_TERMINATED;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_ROAM job.
// i.e. the bot will roam the map randomly(looking for trouble!).
// This low priority job is meant as a fallback if the bot has no jobs in it's buffer.
// Because it is a failsafe job it should never fail to work properly.
int JobRoam(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // make sure the bot has a waypoint destination
    if(job_ptr->waypoint == -1) {
        // pick a random plain waypoint
        job_ptr->waypoint = WaypointFindRandomGoal_R(pBot->pEdict->v.origin, FALSE, 8000.0, -1, 0);
        if(job_ptr->waypoint == -1) {
            // this really shouldn't happen(unless the map has no waypoints)
            return JOB_TERMINATED;
        }
    }

    // check if the bot has arrived at the waypoint
    if(pBot->current_wp == job_ptr->waypoint &&
        VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 50.0)) {
        // wait for a bit then move on
        if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) < 400)
            job_ptr->waypoint = -1;
        else {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            BotLookAbout(pBot); // keep the bots eyes open
        }
    } else {
        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            // failure is not an option here, so force the selection of a new goal waypoint
            job_ptr->waypoint = -1;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_CHAT job.
// i.e. the bot has something personal/humourous to say.
int JobChat(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // make the bot pause whilst "typing"
    pBot->f_move_speed = 0.0;
    pBot->f_side_speed = 0.0;

    // give the bot time to return to it's waypoint afterwards
    pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

    // phase zero - job initialisation
    if(job_ptr->phase == 0) {
        if(pBot->enemy.ptr == NULL) {
            job_ptr->phase = 1;

            // create a delay so the bot can "type"
            job_ptr->phase_timer = pBot->f_think_time + random_float(2.0, 5.0);
        }
    }

    // end phase - say the message
    if(job_ptr->phase == 1 && job_ptr->phase_timer < pBot->f_think_time) {
        // just in case!
        job_ptr->message[MAX_CHAT_LENGTH - 1] = '\0';

        UTIL_HostSay(pBot->pEdict, 0, job_ptr->message);
        return JOB_TERMINATED; // job done
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_REPORT job.
// i.e. the bot has something important to say to it's team.
int JobReport(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // make the bot pause whilst "typing"
    pBot->f_move_speed = 0.0;
    pBot->f_side_speed = 0.0;

    // give the bot time to return to it's waypoint afterwards
    pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

    // phase zero - initialisation
    if(job_ptr->phase == 0) {
        if(pBot->enemy.ptr == NULL) {
            job_ptr->phase = 1;

            // create a delay so the bot can "type"
            job_ptr->phase_timer = pBot->f_think_time + random_float(2.0, 5.0);
        }
    }

    // end phase - say the message
    if(job_ptr->phase == 1 && job_ptr->phase_timer < pBot->f_think_time) {
        // just in case!
        job_ptr->message[MAX_CHAT_LENGTH - 1] = '\0';

        UTIL_HostSay(pBot->pEdict, 1, job_ptr->message);
        return JOB_TERMINATED; // job done
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_PICKUP_ITEM job.
// This will send a bot after a backpack, medikit, or armor that the bot has seen.
int JobPickUpItem(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    /*	if(pBot->f_periodicAlert3 > pBot->f_think_time)
            {
                    char msg[96] = "";
                    snprintf(msg, 96, "picking up %s", item_name);
                    UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!
            }*/

    // stop the bot from taking large route variations during this job
    pBot->f_side_route_time = pBot->f_think_time + 5.0;
    pBot->sideRouteTolerance = 400; // very short route changes

    // phase zero - go to the waypoint nearest the item
    if(job_ptr->phase == 0) {
        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_PICKUP_ITEM, random_float(5.0, 15.0));
            return JOB_TERMINATED;
        }

        // if the bot has arrived at the waypoint nearest to the seen item
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 40.0)) {
            // if you can see the item go pick it up, else dump the job
            if(BotCanSeeOrigin(pBot, job_ptr->object->v.origin)) {
                job_ptr->phase = 1;
                job_ptr->phase_timer = pBot->f_think_time + 8.0;
            } else
                return JOB_TERMINATED;
        }
    }

    // phase 1 - try to pick the item up
    if(job_ptr->phase == 1) {
        // took too long reaching it from the nearest waypoint?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            BlacklistJob(pBot, JOB_PICKUP_FLAG, random_float(1.0, 4.0));
            return JOB_TERMINATED;
        }

        // make sure the item is visible
        if(!BotCanSeeOrigin(pBot, job_ptr->object->v.origin))
            return JOB_TERMINATED;

        BotSetFacing(pBot, job_ptr->object->v.origin);
        BotNavigateWaypointless(pBot);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // if the bot is right on the item (treat the Z axis seperately for best accuracy)
        if(pBot->pEdict->v.origin.z > (job_ptr->object->v.origin.z - 37.0) &&
            pBot->pEdict->v.origin.z < (job_ptr->object->v.origin.z + 40.0)) {
            if((job_ptr->object->v.origin - pBot->pEdict->v.origin).Length2D() < 25.0) {
                // blacklist the job very briefly in case the item can't be picked up
                // don't want the bot obssessing about it constantly
                BlacklistJob(pBot, JOB_PICKUP_ITEM, 1.0);

                return JOB_TERMINATED; // success
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_PICKUP_FLAG job.
// i.e. navigate to waypoint nearest to a previously seen flag and then
// try to pick it up.
int JobPickUpFlag(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - go to the waypoint nearest the flag
    if(job_ptr->phase == 0) {
        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_PICKUP_FLAG, random_float(5.0, 10.0));
            return JOB_TERMINATED;
        }

        // if the bot has arrived at the waypoint nearest to the seen flag
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 40.0)) {
            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + 8.0;
        }
    }

    // phase 1 - try to pick the flag up
    if(job_ptr->phase == 1) {
        // took too long reaching it from the nearest waypoint?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            BlacklistJob(pBot, JOB_PICKUP_FLAG, random_float(3.0, 10.0));
            return JOB_TERMINATED;
        }

        // make sure the flag is visible
        if(!BotCanSeeOrigin(pBot, job_ptr->object->v.origin))
            return JOB_TERMINATED;

        BotSetFacing(pBot, job_ptr->object->v.origin);
        BotNavigateWaypointless(pBot);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // if the bot is right on the flag (treat the Z axis seperately for best accuracy)
        if(pBot->pEdict->v.origin.z > (job_ptr->object->v.origin.z - 37.0) &&
            pBot->pEdict->v.origin.z < (job_ptr->object->v.origin.z + 60.0)) {
            if((job_ptr->object->v.origin - pBot->pEdict->v.origin).Length2D() < 25.0)
                job_ptr->phase = 2;
        }

        return JOB_UNDERWAY;
    }

    // phase 2 - can't pick it up, so defend and report the dropped flags location
    if(job_ptr->phase == 2) {
        // set up a job for defending the dropped flag
        job_struct* newJob = InitialiseNewJob(pBot, JOB_DEFEND_FLAG);
        if(newJob != NULL) {
            newJob->object = job_ptr->object;
            newJob->origin = job_ptr->object->v.origin; // remember where it was seen
            SubmitNewJob(pBot, JOB_DEFEND_FLAG, newJob);
        }

        // used to stop bots on each team over-reporting enemy dropped flags
        static float f_flagReportTimeOut[4] = { 0.0, 0.0, 0.0, 0.0 };

        // set up a job for reporting the dropped flag's location
        if(offensive_chatter && (f_flagReportTimeOut[pBot->current_team] < pBot->f_think_time ||
                                    f_flagReportTimeOut[pBot->current_team] > (pBot->f_think_time + 60.2))) {
            // reset the timeout
            f_flagReportTimeOut[pBot->current_team] = pBot->f_think_time + random_float(40.0, 60.0);

            //	UTIL_HostSay(pBot->pEdict, 0, "Found dropped flag"); //DebugMessageOfDoom!

            newJob = InitialiseNewJob(pBot, JOB_REPORT);
            if(newJob != NULL) {
                const int area = AreaInsideClosest(pBot->pEdict);
                if(area != -1) {
                    switch(pBot->current_team) {
                    case 0:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy has dropped flag %s", areas[area].namea);
                        break;
                    case 1:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy has dropped flag %s", areas[area].nameb);
                        break;
                    case 2:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy has dropped flag %s", areas[area].namec);
                        break;
                    case 3:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Enemy has dropped flag %s", areas[area].named);
                        break;
                    default:
                        break;
                    }

                    SubmitNewJob(pBot, JOB_REPORT, newJob);
                }
            }
        }

        BlacklistJob(pBot, JOB_PICKUP_FLAG, 6.0);
        return JOB_TERMINATED; // job done
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_PUSH_BUTTON job.
int JobPushButton(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // remember the center of the button
    const Vector v_button = VecBModelOrigin(job_ptr->object);

    // debugging stuff
    //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
    //		v_button, 10, 2, 250, 250, 250, 200, 10);

    // phase zero - figure out which button type it is and how to deal with it
    if(job_ptr->phase == 0) {
        // does the button activate only when something(e.g. a player) bumps into it?
        if(job_ptr->object->v.spawnflags & SFLAG_PROXIMITY_BUTTON)
            job_ptr->phase = 2;
        else
            job_ptr->phase = 1; // no, it's just a standard button that you use to activate

        job_ptr->phase_timer = pBot->f_think_time + 8.0;
    }

    // phase 1 - deal with normal use-to-activate buttons here
    if(job_ptr->phase == 1) {
        // taken too long getting to the button?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            BlacklistJob(pBot, JOB_PUSH_BUTTON, 3.0);
            return JOB_TERMINATED;
        }

        BotSetFacing(pBot, v_button);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // approach if not near enough to the button
        if(!VectorsNearerThan(pBot->pEdict->v.origin, v_button, 50.0)) {
            BotNavigateWaypointless(pBot);
        } else {
            pBot->f_pause_time = pBot->f_think_time + 0.2;

            const Vector vecStart = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;

            if(BotInFieldOfView(pBot, v_button - vecStart) < 15) {
                pBot->pEdict->v.button = IN_USE;
                pBot->f_use_button_time = pBot->f_think_time;

                pBot->current_wp = WaypointFindNearest_E(pBot->pEdict, REACHABLE_RANGE, pBot->current_team);

                return JOB_TERMINATED; // success
            }
        }
    }

    // phase 2 - get fairly close to what must be a proximity button
    if(job_ptr->phase == 2) {
        // taken too long getting to the button?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            BlacklistJob(pBot, JOB_PUSH_BUTTON, 3.0);
            return JOB_TERMINATED;
        }

        BotSetFacing(pBot, v_button);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // approach if not near enough to the button
        if(!VectorsNearerThan(pBot->pEdict->v.origin, v_button, 60.0))
            BotNavigateWaypointless(pBot);
        else {
            job_ptr->phase = 3;
            job_ptr->phase_timer = pBot->f_think_time + random_float(0.3, 0.6);
        }
    }

    // phase 3 - keep walking into the proximity button for a fraction of a second
    // to make sure the bot has pushed up against it
    if(job_ptr->phase == 3) {
        BotSetFacing(pBot, v_button);
        BotNavigateWaypointless(pBot);

        // walked into the button long enough?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            pBot->f_use_button_time = pBot->f_think_time; // assume it got activated

            pBot->current_wp = WaypointFindNearest_E(pBot->pEdict, REACHABLE_RANGE, pBot->current_team);

            return JOB_TERMINATED; // success
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_USE_TELEPORT job.
// i.e. go to a waypoint near the teleport, hop onboard and use the teleport.
// If it doesn't work, forget about it.
int JobUseTeleport(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - go to the waypoint near the Teleporter
    if(job_ptr->phase == 0) {
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 30.0)) {
            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + 6.0;
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_USE_TELEPORT, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 1 - go stand on the Teleporter
    if(job_ptr->phase == 1) {
        // figure out the Teleporters center for maximum accuracy
        const Vector TeleportCenter = job_ptr->object->v.absmin + job_ptr->object->v.size / 2;

        // is there some reason to abort?
        // e.g. took too long, teleport is occupied, or bot is fighting
        if(pBot->enemy.ptr != NULL || job_ptr->phase_timer < pBot->f_think_time ||
            BotAllyAtVector(pBot, TeleportCenter, 70.0, FALSE) != NULL) {
            BlacklistJob(pBot, JOB_USE_TELEPORT, 6.0);
            return JOB_TERMINATED;
        }

        BotSetFacing(pBot, TeleportCenter);
        BotNavigateWaypointless(pBot);

        const float distance2D = (TeleportCenter - pBot->pEdict->v.origin).Length2D();

        // slow down on approach
        if(distance2D < 100.0)
            pBot->f_move_speed = pBot->f_max_speed / 3;

        // is the bot standing on it? (treat the Z axis seperately for best accuracy)
        if(pBot->pEdict->v.origin.z > (TeleportCenter.z - 10.0) &&
            pBot->pEdict->v.origin.z < (TeleportCenter.z + 90.0) && distance2D < 20.0) {
            //	UTIL_BotLogPrintf("%s, waiting at %d, distance %f(2D %f), speed %f, time %f\n",
            //		pBot->name,
            //		job_ptr->waypoint,
            //		(pBot->pEdict->v.origin - job_ptr->object->v.origin).Length(),
            //		(pBot->pEdict->v.origin - TeleportCenter).Length2D(),
            //		(pBot->pEdict->v.velocity).Length(),
            //		pBot->f_think_time);

            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + random_float(1.8, 3.5);
            return JOB_UNDERWAY;
        }
    }

    // phase 2 - wait a few seconds, if nothing happens blacklist the job
    // for a short time because the teleporter is not working
    if(job_ptr->phase == 2) {
        /*	if(pBot->f_periodicAlert1 < pBot->f_think_time)
                        UTIL_BotLogPrintf("%s, waiting at %d, distance %f\n",
                                pBot->name, job_ptr->waypoint,
                                (pBot->pEdict->v.origin - job_ptr->object->v.origin).Length());*/

        pBot->f_move_speed = 0.0;
        pBot->f_side_speed = 0.0;
        pBot->f_vertical_speed = 0.0;
        pBot->f_pause_time = pBot->f_think_time + 0.3;
        BotLookAbout(pBot);

        // has the Teleporter worked?
        if(!VectorsNearerThan(pBot->pEdict->v.origin, job_ptr->object->v.origin, 500.0)) {
            //	UTIL_BotLogPrintf("%s, beamed over via %d, distance %f, time %f\n",
            //		pBot->name, job_ptr->waypoint,
            //		(pBot->pEdict->v.origin - job_ptr->object->v.origin).Length(),
            //		pBot->f_think_time);

            job_ptr->phase = 3;
            return JOB_UNDERWAY;
        }

        // is the teleporter non-functional?  if so forget about it for a while
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // recall the memory index of the entrance the bot just tested so the bot
            // can then remember it as being non-functional
            const int teleIndex = BotRecallTeleportEntranceIndex(pBot, job_ptr->object);
            if(teleIndex != -1)
                BotForgetTeleportPair(pBot, teleIndex);

            //	UTIL_BotLogPrintf("%s, teleporter at %d is idle, time %f\n",
            //		pBot->name, job_ptr->waypoint, pBot->f_think_time);

            BlacklistJob(pBot, JOB_USE_TELEPORT, random_float(25.0, 40.0));
            return JOB_TERMINATED;
        }

        // no jumping around foo
        if(pBot->pEdict->v.button & IN_JUMP)
            pBot->pEdict->v.button &= ~IN_JUMP;
    }

    // phase 3 - find and remember the exit(beneath the bot)
    if(job_ptr->phase == 3) {
        // recall the index of the entrance the bot just used
        int teleIndex = BotRecallTeleportEntranceIndex(pBot, job_ptr->object);

        if(teleIndex == -1) {
            teleIndex = BotGetFreeTeleportIndex(pBot);

            // hopefully this shouldn't happen, but just in case
            if(teleIndex == -1) {
                //	UTIL_BotLogPrintf("%s, doh! -1\n", pBot->name);
                return JOB_TERMINATED;
            }
        }

        // find the exit(hopefully!)
        const edict_t* teleExit = BotEntityAtPoint("building_teleporter", pBot->pEdict->v.origin, 90.0);

        // look for a waypoint near the exit
        if(teleExit != NULL) {
            pBot->current_wp = WaypointFindNearest_E(pBot->pEdict, REACHABLE_RANGE, pBot->current_team);

            // welcome space cadet!
            if(pBot->current_wp != -1) {
                //	UTIL_BotLogPrintf("%s, teleported to %d\n", pBot->name, pBot->current_wp);
                pBot->telePair[teleIndex].entrance = job_ptr->object;
                pBot->telePair[teleIndex].entranceWP = job_ptr->waypoint;
                pBot->telePair[teleIndex].exitWP = pBot->current_wp;
            } else
                BotForgetTeleportPair(pBot, teleIndex);
        }

        return JOB_TERMINATED; // regardless of success or failure
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_MAINTAIN_OBJECT job.
// Navigate to a known damaged sentry/dispensor or un-upgraded sentry and give it
// a few whacks with a wrench.
int JobMaintainObject(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - set a destination waypoint near the object
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = WaypointFindNearest_E(job_ptr->object, 500.0, pBot->current_team);
        job_ptr->phase = 1;
        return JOB_UNDERWAY;
    }

    // phase 1 - go to the destination waypoint near the object
    if(job_ptr->phase == 1) {
        // check if the bot has stumbled into the object before it's reached the waypoint
        if(VectorsNearerThan(pBot->pEdict->v.origin, job_ptr->object->v.origin, 85.0) &&
            BotCanSeeOrigin(pBot, job_ptr->object->v.origin)) {
            job_ptr->phase = 2;
            return JOB_UNDERWAY;
        }

        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 40.0))
            job_ptr->phase = 2;
        else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_MAINTAIN_OBJECT, random_float(5.0, 15.0));
                return JOB_TERMINATED;
            }
        }
        return JOB_UNDERWAY;
    }

    // phase 2 - check if the object still needs maintenance
    if(job_ptr->phase == 2) {
        char className[24];
        strncpy(className, STRING(job_ptr->object->v.classname), 24);
        className[23] = '\0';

        if(strcmp("building_sentrygun", className) == 0) {
            bool maintenanceNeeded = FALSE;

            if(job_ptr->object->v.health < 100)
                maintenanceNeeded = TRUE;
            else if(job_ptr->object == pBot->sentry_edict && pBot->sentry_ammo < 100)
                maintenanceNeeded = TRUE;
            else { // sentry needs upgrading?
                char modelName[24];
                strncpy(modelName, STRING(job_ptr->object->v.model), 24);
                modelName[23] = '\0';

                if(strcmp(modelName, "models/sentry3.mdl") != 0)
                    maintenanceNeeded = TRUE;
            }

            if(maintenanceNeeded == FALSE)
                return JOB_TERMINATED; // maintenance is uneeded
        } else if(strcmp("building_dispenser", className) == 0) {
            if(job_ptr->object->v.health > 99)
                return JOB_TERMINATED; // maintenance is uneeded
        }

        job_ptr->phase = 3;
        job_ptr->phase_timer = pBot->f_think_time + 6.0;
        return JOB_UNDERWAY;
    }

    // phase 3 - get close to the object
    if(job_ptr->phase == 3) {
        // having problems getting to the object?
        if(job_ptr->phase_timer < pBot->f_think_time || !BotCanSeeOrigin(pBot, job_ptr->object->v.origin)) {
            BlacklistJob(pBot, JOB_MAINTAIN_OBJECT, random_float(12.0, 24.0));
            return JOB_TERMINATED;
        } else {
            BotSetFacing(pBot, job_ptr->object->v.origin);
            BotNavigateWaypointless(pBot);
            pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

            if(pBot->enemy.ptr == NULL && pBot->current_weapon.iId != TF_WEAPON_SPANNER)
                UTIL_SelectItem(pBot->pEdict, "tf_weapon_spanner");

            if(VectorsNearerThan(pBot->pEdict->v.origin, job_ptr->object->v.origin, 60.0)) {
                job_ptr->phase = 4;
                job_ptr->phase_timer = pBot->f_think_time + random_float(2.0, 3.0);
            }
        }
    }

    // final phase - keep whacking the object for a few seconds
    if(job_ptr->phase == 4) {
        // abort if the object can't be seen
        if(!BotCanSeeOrigin(pBot, job_ptr->object->v.origin))
            return JOB_TERMINATED;

        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED; // enough time spent here - job done

        BotSetFacing(pBot, job_ptr->object->v.origin);
        BotNavigateWaypointless(pBot);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        if(pBot->current_weapon.iId != TF_WEAPON_SPANNER)
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_spanner");
        else
            pBot->pEdict->v.button |= IN_ATTACK; // have at thee ya varlet!
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_BUILD_SENTRY job.
// i.e. go to a sentry waypoint and build a sentry there.
int JobBuildSentry(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase 0, getting into position
    if(job_ptr->phase == 0) {
        if(pBot->current_wp == job_ptr->waypoint) {
            if(!VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 20.0)) {
                // abort the job if there's a sentry gun here already
                if(BotEntityAtPoint("building_sentrygun", waypoints[pBot->current_wp].origin, 300)) {
                    //	UTIL_HostSay(pBot->pEdict, 0, "sentry gun here already"); //DebugMessageOfDoom!
                    BlacklistJob(pBot, JOB_BUILD_SENTRY, random_float(30.0, 60.0));
                    return JOB_TERMINATED;
                }
            } else // the bot has arrived at the waypoint
            {
                pBot->f_move_speed = 0.0;
                pBot->f_side_speed = 0.0;

                // make sure the bot is facing the right way before continuing
                const int aim_index = WaypointFindNearestAiming(waypoints[job_ptr->waypoint].origin);
                if(aim_index != -1) {
                    //	WaypointDrawBeam(INDEXENT(1), waypoints[job_ptr->waypoint].origin + Vector(0, 0, 60),
                    //		waypoints[aim_index].origin, 10, 2, 250, 250, 250, 200, 10);

                    BotSetFacing(pBot, waypoints[aim_index].origin);

                    Vector v_aim = waypoints[aim_index].origin - pBot->pEdict->v.origin;
                    if(BotInFieldOfView(pBot, v_aim) == 0) {
                        job_ptr->phase = 1;
                        job_ptr->phase_timer = pBot->f_think_time + 0.5;
                        FakeClientCommand(pBot->pEdict, "build", "2", NULL);
                    }

                    return JOB_UNDERWAY;
                } else // no waypoint aim indicator found
                {
                    BlacklistJob(pBot, JOB_BUILD_SENTRY, random_float(20.0, 30.0));
                    return JOB_TERMINATED;
                }
            }
        }

        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_BUILD_SENTRY, random_float(10.0, 20.0));
            return JOB_TERMINATED;
        }
    }

    // phase 1 - walk backwards for a second whilst trying to start the build
    if(job_ptr->phase == 1) {
        if(pBot->f_periodicAlertFifth < pBot->f_think_time)
            FakeClientCommand(pBot->pEdict, "build", "2", NULL);

        pBot->f_move_speed = -(pBot->f_max_speed / 2);

        // find the sentry gun being built(if it is) and remember it's location
        edict_t* pent = NULL;
        while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_sentrygun_base")) != NULL && !FNullEnt(pent)) {
            //	UTIL_BotLogPrintf("%s: sentry gun base distance %f\n",
            //		pBot->name, (pBot->pEdict->v.origin - pent->v.origin).Length());

            // check that this sentry gun is near to where the bot was
            // when it started building
            if(!(pent->v.flags & FL_KILLME) && VectorsNearerThan(pBot->pEdict->v.origin, pent->v.origin, 85.0)) {
                job_ptr->origin = pent->v.origin;
                pBot->f_move_speed = 0.0;
                job_ptr->phase = 2;
                job_ptr->phase_timer = pBot->f_think_time + 7.0;
                return JOB_UNDERWAY;
            }
        }

        // waited too long and nothing happened?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // don't repeat the job for a while
            BlacklistJob(pBot, JOB_BUILD_SENTRY, random_float(20.0, 40.0));

            // in case bot didn't know it already had a sentry(rare but possible)
            FakeClientCommand(pBot->pEdict, "detsentry", NULL, NULL);
            return JOB_TERMINATED;
        }
    }

    // phase 2 - keep an eye out for the sentry gun once it's built
    if(job_ptr->phase == 2) {
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // find and remember the sentry gun the bot just built
            bool success = FALSE;
            edict_t* pent = NULL;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_sentrygun")) != NULL && !FNullEnt(pent)) {
                //	UTIL_BotLogPrintf("%s: sentry gun distance %f\n",
                //		pBot->name, (job_ptr->origin - pent->v.origin).Length());

                // check that this sentry gun is near to where the bot was
                // when it started building
                if(VectorsNearerThan(job_ptr->origin, pent->v.origin, 50.0)) {
                    //	UTIL_HostSay(pBot->pEdict, 0, "sentry gun built"); //DebugMessageOfDoom!
                    success = TRUE;
                    pBot->has_sentry = TRUE;
                    pBot->sentry_edict = pent;
                    pBot->sentryWaypoint = job_ptr->waypoint;

                    // rotate the sentry if necessary
                    if(waypoints[job_ptr->waypoint].flags & W_FL_TFC_SENTRY_180) {
                        //	UTIL_HostSay(pBot->pEdict, 0, "rotating SG"); //DebugMessageOfDoom!
                        FakeClientCommand(pBot->pEdict, "rotatesentry180", NULL, NULL);
                        pBot->SGRotated = TRUE;
                    }

                    job_ptr->phase = 3;
                    return JOB_UNDERWAY;
                }
            }

            // abort if the bot has been waiting for the sentry gun and it isn't there
            if(success == FALSE) { // don't repeat the job for a while
                BlacklistJob(pBot, JOB_BUILD_SENTRY, random_float(20.0, 40.0));
                return JOB_TERMINATED;
            }
        }
    }

    // final phase - report the sentry guns location
    if(job_ptr->phase == 3) {
        if(defensive_chatter) {
            const int bot_area = AreaInsideClosest(pBot->pEdict);
            if(bot_area != -1) {
                job_struct* newJob = InitialiseNewJob(pBot, JOB_REPORT);
                if(newJob != NULL) {
                    switch(pBot->current_team) {
                    case 0:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry gun built: %s", areas[bot_area].namea);
                        break;
                    case 1:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry gun built: %s", areas[bot_area].nameb);
                        break;
                    case 2:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry gun built: %s", areas[bot_area].namec);
                        break;
                    case 3:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry gun built: %s", areas[bot_area].named);
                        break;
                    default:
                        return JOB_TERMINATED;
                    }

                    newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
                    SubmitNewJob(pBot, JOB_REPORT, newJob);
                }
            }
        }

        return JOB_TERMINATED; // job done!
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_BUILD_DISPENSER job.
int JobBuildDispenser(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase 0, getting into position
    if(job_ptr->phase == 0) {
        if(pBot->current_wp == job_ptr->waypoint) {
            const float distance = (pBot->pEdict->v.origin - waypoints[job_ptr->waypoint].origin).Length();

            if(distance > 30.0) {
                // abort the job if there's a dispenser here already
                if(BotEntityAtPoint("building_dispenser", waypoints[pBot->current_wp].origin, 300)) {
                    //	UTIL_HostSay(pBot->pEdict, 0, "Dispenser here already"); //DebugMessageOfDoom!
                    BlacklistJob(pBot, JOB_BUILD_DISPENSER, random_float(30.0, 60.0));
                    return JOB_TERMINATED;
                }
            } else {
                job_ptr->phase = 1;
                job_ptr->phase_timer = pBot->f_think_time + 0.5;
                FakeClientCommand(pBot->pEdict, "build", "1", NULL);
                return JOB_UNDERWAY;
            }
        }

        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_BUILD_DISPENSER, random_float(5.0, 20.0));
            return JOB_TERMINATED;
        }
    }

    // phase 1 - walk backwards for a second whilst trying to start the build
    if(job_ptr->phase == 1) {
        if(pBot->f_periodicAlertFifth < pBot->f_think_time)
            FakeClientCommand(pBot->pEdict, "build", "1", NULL);

        pBot->f_move_speed = -(pBot->f_max_speed / 2);

        // find the dispenser being built(if it is) and remember it's location
        edict_t* pent = NULL;
        while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_dispenser")) != NULL && !FNullEnt(pent)) {
            //	UTIL_BotLogPrintf("%s: dispenser distance %f\n",
            //		pBot->name, (pBot->pEdict->v.origin - pent->v.origin).Length());

            // check that this dispenser is near to where the bot was
            // when it started building
            if(!(pent->v.flags & FL_KILLME) && VectorsNearerThan(pBot->pEdict->v.origin, pent->v.origin, 85.0)) {
                job_ptr->origin = pent->v.origin;
                pBot->f_move_speed = 0.0;
                job_ptr->phase = 2;
                job_ptr->phase_timer = pBot->f_think_time + 4.0;
                return JOB_UNDERWAY;
            }
        }

        // waited too long and nothing happened?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // don't repeat the job for a while
            BlacklistJob(pBot, JOB_BUILD_DISPENSER, random_float(20.0, 40.0));

            // in case bot didn't know it already had a dispenser(rare but possible)
            FakeClientCommand(pBot->pEdict, "detdispenser", NULL, NULL);
            return JOB_TERMINATED;
        }
    }

    // phase 2 - keep an eye out for the dispenser once it's built
    if(job_ptr->phase == 2) {
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // find and remember the dispenser the bot just built
            edict_t* pent = NULL;
            bool success = FALSE;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_dispenser")) != NULL && !FNullEnt(pent)) {
                //	UTIL_BotLogPrintf("%s: dispenser distance %f\n",
                //		pBot->name, (job_ptr->origin - pent->v.origin).Length());

                // check that this dispenser is near to where the bot was
                // when it started building
                if(VectorsNearerThan(job_ptr->origin, pent->v.origin, 50.0)) {
                    //	UTIL_HostSay(pBot->pEdict, 0, "dispenser built"); //DebugMessageOfDoom!
                    success = TRUE;
                    pBot->has_dispenser = TRUE;
                    pBot->dispenser_edict = pent;
                    pBot->f_dispenserDetTime = 0.0; // don't blow it up yet!
                    return JOB_TERMINATED;          // job done!
                }
            }

            // abort if the bot has been waiting for the dispenser and it isn't there
            if(!success) { // don't repeat the job for a while
                BlacklistJob(pBot, JOB_BUILD_DISPENSER, random_float(30.0, 60.0));
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_BUILD_TELEPORT job.
// This job can handle the building of both teleport entrances and exits
// based upon the type of waypoint the bot is sent to.
int JobBuildTeleport(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // find out if the bot is to build a teleport entrance or an exit
    bool buildExit = TRUE;
    if(waypoints[job_ptr->waypoint].flags & W_FL_TFC_TELEPORTER_ENTRANCE)
        buildExit = FALSE;

    // phase 0, getting into position
    if(job_ptr->phase == 0) {
        if(pBot->current_wp == job_ptr->waypoint) {
            const float distance = (pBot->pEdict->v.origin - waypoints[job_ptr->waypoint].origin).Length();

            if(distance > 20.0) {
                // abort the job if there's a teleport here already
                if(BotEntityAtPoint("building_teleporter", waypoints[pBot->current_wp].origin, 120.0)) {
                    //	UTIL_HostSay(pBot->pEdict, 0, "Teleport here already"); //DebugMessageOfDoom!
                    BlacklistJob(pBot, JOB_BUILD_TELEPORT, random_float(30.0, 60.0));
                    return JOB_TERMINATED;
                }
            } else {
                pBot->f_move_speed = 0.0;
                pBot->f_side_speed = 0.0;

                // make sure the bot is facing the right way before continuing
                const int aim_index = WaypointFindNearestAiming(waypoints[job_ptr->waypoint].origin);
                if(aim_index != -1) {
                    //	WaypointDrawBeam(INDEXENT(1), waypoints[job_ptr->waypoint].origin + Vector(0, 0, 60),
                    //		waypoints[aim_index].origin, 10, 2, 250, 250, 250, 200, 10);

                    BotSetFacing(pBot, waypoints[aim_index].origin);

                    const Vector v_aim = waypoints[aim_index].origin - pBot->pEdict->v.origin;

                    if(BotInFieldOfView(pBot, v_aim) == 0) {
                        job_ptr->phase = 1;
                        job_ptr->phase_timer = pBot->f_think_time + 0.5;

                        if(buildExit)
                            FakeClientCommand(pBot->pEdict, "build", "5", NULL);
                        else
                            FakeClientCommand(pBot->pEdict, "build", "4", NULL);
                    }

                    return JOB_UNDERWAY;
                } else // don't know which way to face
                {
                    //	UTIL_HostSay(pBot->pEdict, 0, "don't know which way for teleporter"); //DebugMessageOfDoom!
                    BlacklistJob(pBot, JOB_BUILD_TELEPORT, random_float(10.0, 30.0));
                    return JOB_TERMINATED;
                }
            }
        }

        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_BUILD_TELEPORT, random_float(10.0, 30.0));
            return JOB_TERMINATED;
        }
    }

    // phase 1 - walk backwards for a second whilst trying to start the build
    if(job_ptr->phase == 1) {
        if(pBot->f_periodicAlertFifth < pBot->f_think_time) {
            if(buildExit)
                FakeClientCommand(pBot->pEdict, "build", "5", NULL);
            else
                FakeClientCommand(pBot->pEdict, "build", "4", NULL);
        }

        pBot->f_move_speed = -(pBot->f_max_speed / 2);

        // find the teleport being built(if it is) and remember it's location
        edict_t* pent = NULL;
        while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_teleporter")) != NULL && !FNullEnt(pent)) {
            //	UTIL_BotLogPrintf("%s: Teleport distance %f\n",
            //		pBot->name, (pBot->pEdict->v.origin - pent->v.origin).Length());

            // check that this teleport is near to where the bot was
            // when it started building
            if(!(pent->v.flags & FL_KILLME) && VectorsNearerThan(pBot->pEdict->v.origin, pent->v.origin, 85.0)) {
                job_ptr->origin = pent->v.origin;
                pBot->f_move_speed = 0.0;
                job_ptr->phase = 2;
                job_ptr->phase_timer = pBot->f_think_time + 5.0;
                return JOB_UNDERWAY;
            }
        }

        // waited too long and nothing happened?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // don't repeat the job for a while
            BlacklistJob(pBot, JOB_BUILD_TELEPORT, random_float(20.0, 40.0));

            // in case bot didn't know it already had a teleporter(rare but possible)
            if(buildExit)
                FakeClientCommand(pBot->pEdict, "detexitteleporter", NULL, NULL);
            else
                FakeClientCommand(pBot->pEdict, "detentryteleporter", NULL, NULL);
            return JOB_TERMINATED;
        }
    }

    // phase 2 - keep an eye out for the teleport once it's built
    if(job_ptr->phase == 2) {
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // find and remember the teleport the bot just built
            edict_t* pent = NULL;
            bool success = FALSE;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_teleporter")) != NULL && !FNullEnt(pent)) {
                //	UTIL_BotLogPrintf("%s: Teleport distance %f\n",
                //		pBot->name, (job_ptr->origin - pent->v.origin).Length());

                // check that this teleport is near to where the bot was
                // when it started building
                if(VectorsNearerThan(job_ptr->origin, pent->v.origin, 50.0)) {
                    //	UTIL_HostSay(pBot->pEdict, 0, "teleporter built"); //DebugMessageOfDoom!
                    success = TRUE;
                    if(buildExit) {
                        pBot->tpExit = pent;
                        pBot->tpExitWP = job_ptr->waypoint;

                        // Set the owner on the teleport
                        pent->v.euser1 = pBot->pEdict;
                        pent->v.iuser1 = W_FL_TFC_TELEPORTER_EXIT;

                        //	UTIL_BotLogPrintf("%s, teleExit built at %d\n", pBot->name, job_ptr->waypoint);
                    } else {
                        pBot->tpEntrance = pent;
                        pBot->tpEntranceWP = job_ptr->waypoint;

                        // Set the owner on the teleport
                        pent->v.euser1 = pBot->pEdict;
                        pent->v.iuser1 = W_FL_TFC_TELEPORTER_ENTRANCE;

                        //	UTIL_BotLogPrintf("%s, teleEntry built at %d\n", pBot->name, job_ptr->waypoint);
                    }

                    job_ptr->phase = 3;
                    return JOB_UNDERWAY;
                }
            }

            // abort if the bot has been waiting for the teleport and it isn't there
            if(!success) { // don't repeat the job for a while
                BlacklistJob(pBot, JOB_BUILD_TELEPORT, random_float(30.0, 60.0));
                return JOB_TERMINATED;
            }
        }
    }

    // final phase - report the teleporters location
    if(job_ptr->phase == 3) {
        if(defensive_chatter) {
            const int bot_area = AreaInsideClosest(pBot->pEdict);
            if(bot_area != -1) {
                job_struct* newJob = InitialiseNewJob(pBot, JOB_REPORT);
                if(newJob != NULL) {
                    char tpType[24] = "Teleporter Entrance";
                    if(waypoints[job_ptr->waypoint].flags & W_FL_TFC_TELEPORTER_EXIT)
                        strcpy(tpType, "Teleporter Exit");

                    switch(pBot->current_team) {
                    case 0:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "%s built: %s", tpType, areas[bot_area].namea);
                        break;
                    case 1:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "%s built: %s", tpType, areas[bot_area].nameb);
                        break;
                    case 2:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "%s built: %s", tpType, areas[bot_area].namec);
                        break;
                    case 3:
                        snprintf(newJob->message, MAX_CHAT_LENGTH, "%s built: %s", tpType, areas[bot_area].named);
                        break;
                    default:
                        return JOB_TERMINATED;
                    }

                    newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
                    SubmitNewJob(pBot, JOB_REPORT, newJob);
                }
            }
        }

        return JOB_TERMINATED; // job done!
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_BUFF_ALLY job.
// This job involves a Medic or Engineer navigating to a known ally and
// healing them or repairing their armor.
int JobBuffAlly(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // if the medics patient is healthy and buffed enough target them no more
    if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC) {
        if(job_ptr->player->v.health > job_ptr->player->v.max_health + 20)
            return JOB_TERMINATED;

        // if the patient is moving about then heal them to full health only
        const float patientVelocity = job_ptr->player->v.velocity.Length();
        if(patientVelocity > 50.0 && job_ptr->player->v.health >= job_ptr->player->v.max_health)
            return JOB_TERMINATED;
    }

    // phase zero - set a waypoint near where the patient was last seen
    // useful when the patient is far away or not visible
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = WaypointFindNearest_S(job_ptr->origin, NULL, 500.0, pBot->current_team, W_FL_DELETED);

        job_ptr->phase = 1;
        return JOB_UNDERWAY;
    }

    // phase 1 - go to the waypoint near where the patient was last seen
    if(job_ptr->phase == 1) {
        // go for the ally if they are near and visible
        const float allyDistance = (pBot->pEdict->v.origin - job_ptr->player->v.origin).Length();
        if(allyDistance < 500.1 && FVisible(job_ptr->player->v.origin + job_ptr->player->v.view_ofs, pBot->pEdict)) {
            job_ptr->phase = 2;
            return JOB_UNDERWAY;
        }

        // give up if the bot arrived at the waypoint(and the patient wasn't seen)
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 70.0))
            return JOB_TERMINATED;
        else {
            // stop the bot from taking large route variations during this job
            pBot->f_side_route_time = pBot->f_think_time + 5.0;
            pBot->sideRouteTolerance = 200; // very short route changes

            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_BUFF_ALLY, random_float(5.0, 15.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 2 - decide how long the bot will try to heal the visible patient
    if(job_ptr->phase == 2) {
        job_ptr->phase = 3;
        job_ptr->phase_timer = pBot->f_think_time + random_float(8.0, 12.0);
    }

    // phase 3 - heal/repair the visible, near patient
    if(job_ptr->phase == 3) {
        // spent too long trying to get to the patient(e.g. blocked by scenery)
        if(job_ptr->phase_timer < pBot->f_think_time) {
            BlacklistJob(pBot, JOB_BUFF_ALLY, 5.0);
            return JOB_TERMINATED;
        }

        // go back to looking for the patient if they disappear from view
        const float allyDistance = (pBot->pEdict->v.origin - job_ptr->player->v.origin).Length();
        if(allyDistance >= 500.1 || !FVisible(job_ptr->player->v.origin + job_ptr->player->v.view_ofs, pBot->pEdict)) {
            job_ptr->phase = 0;
            return JOB_UNDERWAY;
        }

        // remember where the patient was seen in case they disappear from view
        job_ptr->origin = job_ptr->player->v.origin;

        BotSetFacing(pBot, job_ptr->player->v.origin);
        BotNavigateWaypointless(pBot);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        pBot->strafe_mod = STRAFE_MOD_HEAL;

        // make sure the right weapon is selected
        if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC && pBot->current_weapon.iId != TF_WEAPON_MEDIKIT)
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_medikit");
        else if(pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER && pBot->current_weapon.iId != TF_WEAPON_SPANNER)
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_spanner");

        if(allyDistance < 90.0)
            pBot->pEdict->v.button |= IN_ATTACK;
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_ESCORT_ALLY job.
// i.e. stay close to an allied player.
int JobEscortAlly(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // make sure the escortee has a flag
    if(!PlayerHasFlag(job_ptr->player))
        return JOB_TERMINATED;

    // added for readability
    enum phaseNames {
        SET_WAYPOINT_VISIBLE_ALLY = 0, // must be 0 for the job to work
        FOLLOW_VISIBLE_ALLY,
        SET_WAYPOINT_UNSEEN_ALLY,
        FOLLOW_UNSEEN_ALLY
    };

    // stop the bot from taking large route variations during this job
    pBot->f_side_route_time = pBot->f_think_time + 5.0;
    pBot->sideRouteTolerance = 200; // very short route changes

    // don't want the bot to trail further than this from it's escortee
    const float maxEscortRange = 500.0;

    // phase zero - we assume the ally is visible and must find a waypoint near them
    if(job_ptr->phase == SET_WAYPOINT_VISIBLE_ALLY) {
        job_ptr->waypoint = WaypointFindInRange(job_ptr->origin, 50.0, maxEscortRange, pBot->current_team, TRUE);

        job_ptr->phase = FOLLOW_VISIBLE_ALLY;
        return JOB_UNDERWAY;
    }

    // phase 1 - go for the waypoint nearest to the ally(who we assume is visible)
    if(job_ptr->phase == FOLLOW_VISIBLE_ALLY) {
        // if the ally is no longer visible - switch to the phase for handling that
        if(pBot->f_periodicAlert1 < pBot->f_think_time && !FVisible(job_ptr->player->v.origin, pBot->pEdict)) {
            job_ptr->phase = SET_WAYPOINT_UNSEEN_ALLY;
            return JOB_UNDERWAY;
        } else
            job_ptr->origin = job_ptr->player->v.origin; // remember where we saw them

        // make sure the waypoint the bot is using is near enough to the escortee
        if(pBot->f_periodicAlertFifth < pBot->f_think_time &&
            !VectorsNearerThan(waypoints[job_ptr->waypoint].origin, job_ptr->player->v.origin, maxEscortRange))
            job_ptr->phase = SET_WAYPOINT_VISIBLE_ALLY;

        // pause if the bot arrived at the chosen waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 70.0)) {
            // periodically change guard position(keeps the bot alert)
            if(pBot->f_periodicAlert3 < pBot->f_think_time && random_long(1, 1000) < 501) {
                job_ptr->phase = SET_WAYPOINT_VISIBLE_ALLY;
            } else {
                pBot->f_move_speed = 0.0;
                pBot->f_side_speed = 0.0;

                BotLookAbout(pBot); // keep the bots eyes open
            }
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_ESCORT_ALLY, random_float(5.0, 15.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 2 - assuming the ally is not visible, make a guess at where the
    // bot can find them
    if(job_ptr->phase == SET_WAYPOINT_UNSEEN_ALLY) {
        job_ptr->waypoint = BotGuessPlayerPosition(pBot, job_ptr->origin);

        job_ptr->phase = FOLLOW_UNSEEN_ALLY;
        return JOB_UNDERWAY;
    }

    // phase 3 - assuming the ally is not visible, go for the waypoint where
    // the bot may find them
    if(job_ptr->phase == FOLLOW_UNSEEN_ALLY) {
        // if the ally is visible again - switch to the phase for handling that
        if(pBot->f_periodicAlert1 < pBot->f_think_time && FVisible(job_ptr->player->v.origin, pBot->pEdict)) {
            job_ptr->phase = SET_WAYPOINT_VISIBLE_ALLY;
            return JOB_UNDERWAY;
        }

        // terminate the job if the bot arrived at the chosen waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 50.0))
            return JOB_TERMINATED;
        else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_ESCORT_ALLY, random_float(5.0, 15.0));
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_CALL_MEDIC job.
// i.e. Call a recently seen Medic and wait for medical assistance to arrive.
int JobCallMedic(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    //	if(pBot->f_periodicAlert1 < pBot->f_think_time)
    //		UTIL_HostSay(pBot->pEdict, 0, "MEDIC!"); //DebugMessageOfDoom!

    // phase zero - call and decide how long to wait initially
    if(job_ptr->phase == 0) {
        FakeClientCommand(pBot->pEdict, "saveme", NULL, NULL);
        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + random_float(5.0, 7.0);
    }

    // phase 1 - wait for a medic to become visible
    if(job_ptr->phase == 1) {
        // give up if the bots waited too long without seeing a medic
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // seen a medic very recently?  wait a bit longer, let them heal you
        if((pBot->f_alliedMedicSeenTime + 0.5) > pBot->f_think_time) {
            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + random_float(8.0, 10.0);
            return JOB_UNDERWAY;
        }

        pBot->f_pause_time = pBot->f_think_time + 0.2;
        BotLookAbout(pBot);

        // call again?
        if(pBot->f_periodicAlert3 < pBot->f_think_time && random_long(0, 1000) < 500)
            FakeClientCommand(pBot->pEdict, "saveme", NULL, NULL);
    }

    // phase 2 - wait for a visible medic to come over and do their magic
    if(job_ptr->phase == 2) {
        // give up if the bots waited too long without being healed
        // also give up if the medic the bot saw has disappeared again
        if(job_ptr->phase_timer < pBot->f_think_time || (pBot->f_alliedMedicSeenTime + 2.0) < pBot->f_think_time) {
            // give up on Medics for a while so the bot can get on with it's life
            BlacklistJob(pBot, JOB_CALL_MEDIC, random_float(20.0, 30.0));
            return JOB_TERMINATED;
        }

        pBot->f_pause_time = pBot->f_think_time + 0.2;
        BotLookAbout(pBot);
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_GET_HEALTH job.
// i.e. go to a health waypoint.
int JobGetHealth(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - go to the waypoint
    if(job_ptr->phase == 0) {
        // check if the bot has arrived
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0)) {
            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + random_float(7.0, 15.0);
            return JOB_UNDERWAY;
        } else // not there yet
        {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE) &&
                !BotSetAlternativeGoalWaypoint(pBot, job_ptr->waypoint, W_FL_HEALTH)) {
                BlacklistJob(pBot, JOB_GET_HEALTH, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
            // is the waypoint already occupied by someone more needy? try elsewhere
            else if(pBot->f_periodicAlert1 < pBot->f_think_time &&
                WaypointDistanceFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team) < 800) {
                const edict_t* allyPtr = BotAllyAtVector(pBot, waypoints[job_ptr->waypoint].origin, 80.0, TRUE);
                if(!FNullEnt(allyPtr) && PlayerHealthPercent(pBot->pEdict) > PlayerHealthPercent(allyPtr)) {
                    job_ptr->waypoint =
                        WaypointFindRandomGoal_D(job_ptr->waypoint, pBot->current_team, 4000, W_FL_AMMO);

                    if(job_ptr->waypoint == -1) {
                        BlacklistJob(pBot, JOB_GET_AMMO, random_float(10.0, 20.0));
                        return JOB_TERMINATED;
                    }
                }
            }
        }
    }

    // phase 1 - waiting at the waypoint in question
    if(job_ptr->phase == 1) {
        // left the waypoint for any reason?
        if(!VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0))
            job_ptr->phase = 0;

        // if disturbed by an enemy, try another waypoint or give up for a few seconds
        if(pBot->visEnemyCount > 0 || (pBot->f_injured_time + 0.5) > pBot->f_think_time) {
            job_ptr->waypoint = WaypointFindRandomGoal_D(pBot->current_wp, pBot->current_team, 3000, W_FL_HEALTH);

            if(job_ptr->waypoint == -1) {
                BlacklistJob(pBot, JOB_GET_HEALTH, random_float(2.0, 4.0));
                return JOB_TERMINATED;
            }
        }

        // if disturbed by an ally who has less health than you give up the spot
        if(pBot->visAllyCount > 1) {
            const edict_t* allyPtr = BotAllyAtVector(pBot, waypoints[job_ptr->waypoint].origin, 200.0, FALSE);
            if(!FNullEnt(allyPtr) && PlayerHealthPercent(pBot->pEdict) > PlayerHealthPercent(allyPtr)) {
                job_ptr->waypoint = WaypointFindRandomGoal_D(pBot->current_wp, pBot->current_team, 3000, W_FL_HEALTH);

                if(job_ptr->waypoint == -1) {
                    BlacklistJob(pBot, JOB_GET_HEALTH, random_float(8.0, 16.0));
                    return JOB_TERMINATED;
                }
            }
        }

        // waited too long?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // sometimes just give up for a while because some items
            // take a long time to respawn
            if(random_long(0, 100) < pBot->trait.aggression) {
                BlacklistJob(pBot, JOB_GET_HEALTH, random_float(10.0, 20.0));
                return JOB_TERMINATED;
            } else
                return JOB_TERMINATED; // job done
        }

        // keep waiting
        pBot->f_pause_time = pBot->f_think_time + 0.2;
        BotLookAbout(pBot); // keep the bot alert
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_GET_ARMOR job.
// i.e. go to an armor waypoint.
int JobGetArmor(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - go to the waypoint
    if(job_ptr->phase == 0) {
        // check if the bot has arrived
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0)) {
            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + random_float(7.0, 15.0);
            return JOB_UNDERWAY;
        } else // not there yet
        {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE) &&
                !BotSetAlternativeGoalWaypoint(pBot, job_ptr->waypoint, W_FL_ARMOR)) {
                BlacklistJob(pBot, JOB_GET_ARMOR, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
            // is the waypoint already occupied by someone more needy? try elsewhere
            else if(pBot->f_periodicAlert1 < pBot->f_think_time &&
                WaypointDistanceFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team) < 800) {
                const edict_t* allyPtr = BotAllyAtVector(pBot, waypoints[job_ptr->waypoint].origin, 80.0, TRUE);
                if(!FNullEnt(allyPtr) && PlayerArmorPercent(pBot->pEdict) > PlayerArmorPercent(allyPtr)) {
                    job_ptr->waypoint =
                        WaypointFindRandomGoal_D(job_ptr->waypoint, pBot->current_team, 4000, W_FL_AMMO);

                    if(job_ptr->waypoint == -1) {
                        BlacklistJob(pBot, JOB_GET_AMMO, random_float(10.0, 20.0));
                        return JOB_TERMINATED;
                    }
                }
            }
        }
    }

    // phase 1 - waiting at the waypoint in question
    if(job_ptr->phase == 1) {
        // left the waypoint for any reason?
        if(!VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0))
            job_ptr->phase = 0;

        // if disturbed by an enemy, try another waypoint or give up for a few seconds
        if(pBot->visEnemyCount > 0 || (pBot->f_injured_time + 0.5) > pBot->f_think_time) {
            job_ptr->waypoint = WaypointFindRandomGoal_D(pBot->current_wp, pBot->current_team, 3000, W_FL_ARMOR);

            if(job_ptr->waypoint == -1) {
                BlacklistJob(pBot, JOB_GET_ARMOR, random_float(2.0, 4.0));
                return JOB_TERMINATED;
            }
        }

        // if disturbed by an ally who has less armor than you give up the spot
        if(pBot->visAllyCount > 1) {
            const edict_t* allyPtr = BotAllyAtVector(pBot, waypoints[job_ptr->waypoint].origin, 200.0, FALSE);
            if(!FNullEnt(allyPtr) && PlayerArmorPercent(pBot->pEdict) > PlayerArmorPercent(allyPtr)) {
                job_ptr->waypoint = WaypointFindRandomGoal_D(pBot->current_wp, pBot->current_team, 3000, W_FL_ARMOR);

                if(job_ptr->waypoint == -1) {
                    BlacklistJob(pBot, JOB_GET_ARMOR, random_float(8.0, 16.0));
                    return JOB_TERMINATED;
                }
            }
        }

        // waited too long?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // sometimes just give up for a while because some items
            // take a long time to respawn
            if(random_long(0, 100) < pBot->trait.aggression) {
                BlacklistJob(pBot, JOB_GET_ARMOR, random_float(10.0, 20.0));
                return JOB_TERMINATED;
            } else
                return JOB_TERMINATED; // job done
        }

        // keep waiting
        pBot->f_pause_time = pBot->f_think_time + 0.2;
        BotLookAbout(pBot); // keep the bot alert
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_GET_AMMO job.
// i.e. go to an ammo waypoint.
int JobGetAmmo(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - go to the waypoint
    if(job_ptr->phase == 0) {
        // check if the bot has arrived
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0)) {
            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + random_float(7.0, 15.0);
            return JOB_UNDERWAY;
        } else // not there yet
        {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE) &&
                !BotSetAlternativeGoalWaypoint(pBot, job_ptr->waypoint, W_FL_AMMO)) {
                BlacklistJob(pBot, JOB_GET_AMMO, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
            // is the waypoint already occupied? try elsewhere
            else if(pBot->f_periodicAlert1 < pBot->f_think_time &&
                WaypointDistanceFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team) < 800) {
                const edict_t* allyPtr = BotAllyAtVector(pBot, waypoints[job_ptr->waypoint].origin, 80.0, TRUE);
                if(!FNullEnt(allyPtr)) {
                    job_ptr->waypoint =
                        WaypointFindRandomGoal_D(job_ptr->waypoint, pBot->current_team, 4000, W_FL_AMMO);

                    if(job_ptr->waypoint == -1) {
                        BlacklistJob(pBot, JOB_GET_AMMO, random_float(10.0, 20.0));
                        return JOB_TERMINATED;
                    }
                }
            }
        }
    }

    // phase 1 - waiting at the waypoint in question
    if(job_ptr->phase == 1) {
        // left the waypoint for any reason?
        if(!VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0))
            job_ptr->phase = 0;

        // if disturbed by an enemy, try another waypoint or give up for a few seconds
        if(pBot->visEnemyCount > 0 || (pBot->f_injured_time + 0.5) > pBot->f_think_time) {
            job_ptr->waypoint = WaypointFindRandomGoal_D(pBot->current_wp, pBot->current_team, 4000, W_FL_AMMO);

            if(job_ptr->waypoint == -1) {
                BlacklistJob(pBot, JOB_GET_AMMO, random_float(2.0, 4.0));
                return JOB_TERMINATED;
            }
        }

        // waited too long?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // sometimes just give up for a while because some items
            // take a long time to respawn
            if(random_long(0, 100) < pBot->trait.aggression) {
                BlacklistJob(pBot, JOB_GET_AMMO, random_float(10.0, 20.0));
                return JOB_TERMINATED;
            } else
                return JOB_TERMINATED; // job done
        }

        // keep waiting
        pBot->f_pause_time = pBot->f_think_time + 0.2;
        BotLookAbout(pBot); // keep the bot alert
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_DISGUISE job.
// i.e. make the bot sneak away and disguise itself when undisguised.
int JobDisguise(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // check if the Spy succeeded or failed at disguising
    if(pBot->disguise_state == DISGUISE_UNDERWAY) {
        // check if the disguise completed
        if(pBot->current_team != UTIL_GetTeamColor(pBot->pEdict))
            pBot->disguise_state = DISGUISE_COMPLETE;
        // check if disguise took so long it must have failed
        else if(pBot->f_disguise_time < pBot->f_think_time)
            pBot->disguise_state = DISGUISE_NONE;
    } else if(pBot->disguise_state == DISGUISE_COMPLETE) {
        // check if the Spy is still disguised
        if(pBot->current_team == UTIL_GetTeamColor(pBot->pEdict))
            pBot->disguise_state = DISGUISE_NONE;
        else
            return JOB_TERMINATED; // bot is disguised - job done
    }

    // is it time to pick a new disguise?
    if(pBot->disguise_state == DISGUISE_NONE) {
        static const int disguiseList[] = { 2, 3, 4, 5, 7, 8, 9 };
        int new_disguise = disguiseList[random_long(0, 6)];

        char choice[2];
        itoa(new_disguise, choice, 10);
        FakeClientCommand(pBot->pEdict, "disguise_enemy", choice, NULL);
        pBot->disguise_state = DISGUISE_UNDERWAY;
        pBot->f_disguise_time = pBot->f_think_time + 12.0f;
    }

    // phase zero - pick a waypoint to go to
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = WaypointFindRandomGoal_R(pBot->pEdict->v.origin, FALSE, 1400.0, -1, 0);

        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + 6.0;
        return JOB_UNDERWAY;
    }

    // phase 1 - go to the waypoint and crouch if the bot spawned recently
    if(job_ptr->phase == 1) {
        // check if the bot has arrived
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 40.0)) {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            pBot->pEdict->v.button |= IN_DUCK;
            BotLookAbout(pBot);
        } else {
            // stop the bot from taking large route variations during this job
            pBot->f_side_route_time = pBot->f_think_time + 5.0;
            pBot->sideRouteTolerance = 400; // very short route changes

            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_DISGUISE, 3.0);
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_FEIGN_AMBUSH job.
// i.e. the bot has found somewhere to feign and wait for a passing enemy to attack.
int JobFeignAmbush(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase 0 - set the next phases delay
    if(job_ptr->phase == 0) {
        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + 1.0;
    }

    // phase 1 - stop and face away from a nearby wall
    if(job_ptr->phase == 1) {
        // ready?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + random_float(2.0, 3.0);
            return JOB_UNDERWAY;
        }

        BotSetFacing(pBot, job_ptr->origin);
        pBot->pEdict->v.ideal_yaw += 180; // face away from the wall

        pBot->f_move_speed = 0.0;
    }

    // phase 2 - back into a nearby wall
    if(job_ptr->phase == 2) {
        // took too long getting into position?
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // near enough to the wall yet?
        if(VectorsNearerThan(pBot->pEdict->v.origin, job_ptr->origin, 40.0)) {
            job_ptr->phase = 3;
            job_ptr->phase_timer = pBot->f_think_time + random_float(15.0, 30.0);
            return JOB_UNDERWAY;
        }

        BotSetFacing(pBot, job_ptr->origin);
        pBot->pEdict->v.ideal_yaw += 180; // face away from the wall

        pBot->f_move_speed = -(pBot->f_max_speed / 2);
    }

    // phase 3 - watch and wait for a victim(someone with their back turned!)
    if(job_ptr->phase == 3) {
        // had enough of camping out?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // reset the ambush timer
            pBot->f_spyFeignAmbushTime = pBot->f_think_time + random_float(12.0, 24.0);

            return JOB_TERMINATED;
        }

        // give the bot time to return to it's waypoint afterwards
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // play dead
        pBot->f_feigningTime = pBot->f_think_time + 0.2;

        if(pBot->enemy.ptr != NULL)
            return JOB_TERMINATED; // success - victim found
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_SNIPE job.
int JobSnipe(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase 0 - get near enough to the waypoint to start charging the rifle
    if(job_ptr->phase == 0) {
        // stop the bot from taking large route variations during this job
        pBot->f_side_route_time = pBot->f_think_time + 5.0;
        pBot->sideRouteTolerance = 1000; // short route changes

        const float zDiff = waypoints[job_ptr->waypoint].origin.z - pBot->pEdict->v.origin.z;
        const int nextWP = WaypointRouteFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team);

        // near enough to the waypoint the bot can walk to it whilst charging the rifle?
        if(pBot->pEdict->v.flags & FL_ONGROUND &&
            (pBot->current_wp == job_ptr->waypoint || nextWP == job_ptr->waypoint) && zDiff > -45.1 &&
            zDiff < 15.0 // walkable step height(hopefully)
            && VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 200.0) &&
            FVisible(waypoints[job_ptr->waypoint].origin, pBot->pEdict)) {
            // minimum time to charge rifle based on skill level
            // (skills 4 - 5 shouldn't pre-charge)
            const float baseChargeTime[5] = { 2.0, 1.0, 0.0, -5.0, -5.0 };

            job_ptr->phase = 1;
            if(waypoints[job_ptr->waypoint].flags & W_FL_CROUCH)
                job_ptr->phase_timer = 0.0; // crouching AND charging slows you waay down
            else
                job_ptr->phase_timer = pBot->f_think_time + baseChargeTime[pBot->bot_skill] + random_float(0.5, 2.0);
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_SNIPE, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 1 - stop moving and start charging the rifle
    if(job_ptr->phase == 1) {
        // give up if an ally is waiting here already
        if(BotAllyAtVector(pBot, waypoints[job_ptr->waypoint].origin, 60.0, TRUE) != NULL) {
            BlacklistJob(pBot, JOB_SNIPE, random_float(20.0, 40.0));
            return JOB_TERMINATED;
        }

        // make sure the sniper rifle is selected and charging if possible
        if(pBot->enemy.ptr == NULL) {
            if(pBot->current_weapon.iId != TF_WEAPON_SNIPERRIFLE)
                UTIL_SelectItem(pBot->pEdict, "tf_weapon_sniperrifle");
            else {
                pBot->f_snipe_time = pBot->f_think_time + 0.3;
                pBot->pEdict->v.button |= IN_ATTACK;
            }
        }

        // been charging the rifle for a few seconds?
        if(job_ptr->phase_timer < pBot->f_think_time || pBot->enemy.ptr != NULL) {
            // give the bot time to get into position
            pBot->f_current_wp_deadline = pBot->f_think_time + 8.0;

            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + 8.0;
            return JOB_UNDERWAY;
        }

        // to begin charging the rifle we have to stop the bot for at least one frame
        pBot->f_pause_time = pBot->f_think_time + 0.4;

        // try to match the bots facing with that of the sniper point
        const int aim_index = WaypointFindNearestAiming(waypoints[job_ptr->waypoint].origin);
        if(aim_index != -1) {
            Vector vang = waypoints[aim_index].origin - waypoints[job_ptr->waypoint].origin;
            vang = UTIL_VecToAngles(vang);

            pBot->pEdict->v.ideal_yaw = vang.y;
            BotFixIdealYaw(pBot->pEdict);
        }
    }

    // phase 2 - get into position(possibly whilst charging the rifle)
    if(job_ptr->phase == 2) {
        // give up if the bot took too long getting into position
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        if(VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 20.0)) {
            // drop a neoTF grenade pod if possible
            char* cvar_ntf_feature_antimissile = (char*)CVAR_GET_STRING("ntf_feature_antimissile");
            if(strcmp(cvar_ntf_feature_antimissile, "1") == 0)
                FakeClientCommand(pBot->pEdict, "buildspecial", NULL, NULL);

            job_ptr->phase = 3;
            job_ptr->phase_timer = pBot->f_think_time + random_float(180.0, 300.0);
        } else {
            // make sure the sniper rifle is selected and charging if possible
            if(pBot->enemy.ptr == NULL) {
                if(pBot->current_weapon.iId != TF_WEAPON_SNIPERRIFLE)
                    UTIL_SelectItem(pBot->pEdict, "tf_weapon_sniperrifle");
                else
                    pBot->f_snipe_time = pBot->f_think_time + 0.5;
            }

            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, TRUE)) // navigate by strafing
            {
                BlacklistJob(pBot, JOB_SNIPE, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 3 - camp
    if(job_ptr->phase == 3) {
        // give up if the bot has been waiting a while and seen no targets
        if(job_ptr->phase_timer <= pBot->f_think_time && (pBot->enemy.f_lastSeen + 60.0) < pBot->f_think_time)
            return JOB_TERMINATED;
        else
            job_ptr->phase_timer = pBot->f_think_time + random_float(180.0, 300.0);

        if(pBot->enemy.ptr != NULL) {
            // non-campers don't like to stay if their enemy has lived long
            // enough to discover their location
            if(!pBot->trait.camper && (pBot->enemy.f_firstSeen + 3.0) < pBot->f_think_time &&
                (!(pBot->pEdict->v.button & IN_ATTACK) || pBot->f_snipe_time < pBot->f_think_time)) {
                return JOB_TERMINATED; // find another sniping spot
            }

            return JOB_UNDERWAY; // got an enemy - don't run the code below
        }

        // get back into position if the bot has somehow left it's spot
        if(!VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 50.0))
            job_ptr->phase = 0;

        // give the bot time to return to it's waypoint after sniping
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // remain paused
        pBot->f_move_speed = 0.0;
        pBot->f_side_speed = 0.0;

        // make sure the sniper rifle is selected
        if(pBot->current_weapon.iId != TF_WEAPON_SNIPERRIFLE)
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_sniperrifle");

        // charge the sniper rifle whilst waiting
        pBot->f_snipe_time = pBot->f_think_time + random_float(0.8, 1.5);

        // make snipers look about
        if(pBot->f_view_change_time <= pBot->f_think_time) {
            pBot->f_view_change_time = pBot->f_think_time + random_float(1.0f, 4.0f);

            const int aim_index = WaypointFindNearestAiming(waypoints[job_ptr->waypoint].origin);
            if(aim_index != -1) {
                Vector v_aim = waypoints[aim_index].origin - waypoints[job_ptr->waypoint].origin;
                Vector aim_angles = UTIL_VecToAngles(v_aim);
                pBot->pEdict->v.ideal_yaw = aim_angles.y + (random_long(0, 40) - 20);
                pBot->pEdict->v.idealpitch = 0 + (random_long(0, 20) - 10);
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_GUARD_WAYPOINT job.
// This job type can run until the map changes, and involves the bot
// guarding a defender waypoint.
int JobGuardWaypoint(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase 0 - decide the bots patience for inactivity
    if(job_ptr->phase == 0) {
        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + random_float(90.0, 180.0);
    }

    // been waiting a long time without seeing any enemies?
    if(job_ptr->phase == 1 && job_ptr->phase_timer < pBot->f_think_time) {
        if((pBot->enemy.f_lastSeen + 60.0) < pBot->f_think_time)
            return JOB_TERMINATED;
        // enemies were seen, set the timer again
        else
            job_ptr->phase_timer = pBot->f_think_time + random_float(90.0, 180.0);
    }

    // arrived at the defender waypoint?
    if(pBot->current_wp == job_ptr->waypoint &&
        VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 50.0)) {
        // don't guard here if another bot already is
        if(BotDefenderAtWaypoint(pBot, job_ptr->waypoint, 300) != NULL)
            return JOB_TERMINATED;

        // remain paused and looking in the right direction.
        pBot->f_move_speed = 0.0;
        pBot->f_side_speed = 0.0;

        // give the bot time to return to it's waypoint afterwards
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // look about occasionally
        if(pBot->f_view_change_time <= pBot->f_think_time) {
            pBot->f_view_change_time = pBot->f_think_time + random_float(1.0f, 4.0f);

            int aim_index = WaypointFindNearestAiming(waypoints[job_ptr->waypoint].origin);
            if(aim_index != -1) {
                Vector v_aim = waypoints[aim_index].origin - waypoints[pBot->current_wp].origin;
                Vector aim_angles = UTIL_VecToAngles(v_aim);
                pBot->pEdict->v.ideal_yaw = aim_angles.y + (random_long(0, 60) - 30);
                pBot->pEdict->v.idealpitch = 0 + (random_long(0, 20) - 10);
            }
        }
    } else if(pBot->enemy.ptr == NULL) {
        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_GUARD_WAYPOINT, random_float(5.0, 20.0));
            return JOB_TERMINATED;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_DEFEND_FLAG job.
// i.e. the bot has knows of a flag the enemy team has dropped and wants to guard it
// so that the enemy team can't pick it up.
int JobDefendFlag(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase 0 - pick somewhere to guard the flag from
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = WaypointFindInRange(job_ptr->object->v.origin, 100.0, 1000.0, pBot->current_team, TRUE);

        job_ptr->phase = 1; // it's time to guard the flag
        job_ptr->phase_timer = pBot->f_think_time + random_float(30.0, 120.0);
        return JOB_UNDERWAY;
    }

    // phase 1 - guard the flag until it returns to it's spawn point
    if(job_ptr->phase == 1) {
        // make sure the bot has guarded the flag long enough
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // check if the bot has arrived
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 40.0)) {
            // look about and wait
            pBot->f_pause_time = pBot->f_think_time + 0.2;
            BotLookAbout(pBot);
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_DEFEND_FLAG, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_GET_FLAG job.
// This job makes the bot go to flag waypoints in order to collect a flag.
int JobGetFlag(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - go to the flag waypoint
    if(job_ptr->phase == 0) {
        if(pBot->current_wp == job_ptr->waypoint) {
            if(VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0)) {
                job_ptr->phase = 1;
                job_ptr->phase_timer = pBot->f_think_time + random_float(10.0, 40.0);
                return JOB_UNDERWAY;
            }
            // don't repeat the job for a while, if this flag waypoint is occupied
            else if(BotAllyAtVector(pBot, waypoints[job_ptr->waypoint].origin, 100.0, TRUE) != NULL) {
                BlacklistJob(pBot, JOB_GET_FLAG, random_float(15.0, 35.0));
                return JOB_TERMINATED;
            }
        }

        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_GET_FLAG, random_float(5.0, 20.0));
            return JOB_TERMINATED;
        }
    }

    // phase 1 - wait for a while
    if(job_ptr->phase == 1) {
        // been waiting for a while?
        if(job_ptr->phase_timer < pBot->f_think_time) {
            BlacklistJob(pBot, JOB_GET_FLAG, random_float(20.0, 40.0));
            return JOB_TERMINATED;
        }

        // has the bot left the spot for any reason?
        if(!VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 20.0)) {
            job_ptr->phase = 0;
        } else {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            // look about whilst waiting
            BotLookAbout(pBot);

            // randomly crouch
            if(pBot->trait.aggression < 30 && pBot->f_periodicAlert1 < pBot->f_think_time && pBot->visEnemyCount < 1 &&
                random_long(1, 1000) < 200)
                pBot->f_duck_time = pBot->f_think_time + random_float(0.9, 1.1);
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_CAPTURE_FLAG job.
// i.e. if the bot has a flag then bring it to a capture point.
int JobCaptureFlag(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - make sure the bot has a capture waypoint destination
    if(job_ptr->phase == 0) {
        if(job_ptr->waypoint == -1) {
            job_ptr->waypoint = BotFindFlagGoal(pBot);
            if(job_ptr->waypoint == -1) {
                //	UTIL_HostSay(pBot->pEdict, 0, "JOB_CAPTURE_FLAG no goal"); //DebugMessageOfDoom!
                BlacklistJob(pBot, JOB_CAPTURE_FLAG, 8.0);
                return JOB_TERMINATED;
            }
        }

        job_ptr->phase = 1;
        return JOB_UNDERWAY;
    }

    // phase 1 - check if the bot has arrived at the flag capture point
    if(job_ptr->phase == 1) {
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 30.0)) {
            return JOB_TERMINATED; // job done
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE) &&
                !BotSetAlternativeGoalWaypoint(pBot, job_ptr->waypoint, W_FL_TFC_FLAG_GOAL)) {
                BlacklistJob(pBot, JOB_CAPTURE_FLAG, random_float(5.0, 15.0));
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_HARRASS_DEFENSE job.
// This job involves the bot roaming around areas where enemy defenders are
// likely to be and making their lives difficult.
int JobHarrassDefense(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // maybe go for a flag waypoint if the bot is near enough to one
    if(pBot->f_periodicAlert3 < pBot->f_think_time) {
        const int flagWP = BotFindFlagWaypoint(pBot);
        if(flagWP != -1) {
            const int flagDistance = WaypointDistanceFromTo(pBot->current_wp, flagWP, pBot->current_team);

            // the nearer the bot is the more likely it will go for the flag
            if(flagDistance != -1 && (flagDistance < 800 // always when this close
                                         || flagDistance < 2000 && random_long(1, 2000) > flagDistance)) {
                job_struct* newJob = InitialiseNewJob(pBot, JOB_GET_FLAG);
                if(newJob != NULL) {
                    newJob->waypoint = flagWP;
                    if(SubmitNewJob(pBot, JOB_GET_FLAG, newJob) == TRUE)
                        return JOB_TERMINATED; // end this job so the bot can go for the flag instead
                }
            }
        }
    }

    // make sure the bot has a waypoint to go visit
    if(job_ptr->waypoint == -1) {
        job_ptr->waypoint = BotTargetDefenderWaypoint(pBot);
        if(job_ptr->waypoint == -1) {
            //	UTIL_HostSay(pBot->pEdict, 0, "no harrass goal");
            BlacklistJob(pBot, JOB_HARRASS_DEFENSE, 8.0);
            return JOB_TERMINATED;
        }

        return JOB_UNDERWAY; // to conserve CPU use the new goal on the next program cycle
    }

    // check if the bot has arrived at it's latest destination
    if(pBot->current_wp == job_ptr->waypoint &&
        VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 60.0)) {
        // wait for a random amount of time before choosing a new goal
        if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) < 400)
            job_ptr->waypoint = -1;
        else {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            // look about whilst waiting
            BotLookAbout(pBot);
        }
    } else {
        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_HARRASS_DEFENSE, random_float(5.0, 20.0));
            return JOB_TERMINATED;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_ROCKET_JUMP job.
int JobRocketJump(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // always head towards the rocket jump waypoint
    pBot->current_wp = job_ptr->waypoint;
    BotSetFacing(pBot, waypoints[job_ptr->waypoint].origin);
    pBot->pEdict->v.button |= IN_FORWARD;
    pBot->f_move_speed = pBot->f_max_speed;
    pBot->f_duck_time = 0;

    // debug stuff
    //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
    //		waypoints[job_ptr->waypoint].origin, 10, 2, 250, 250, 250, 200, 10);

    if(pBot->current_weapon.iId != TF_WEAPON_RPG)
        UTIL_SelectItem(pBot->pEdict, "tf_weapon_rpg");

    // phase zero - set up a timer for the next phase and select the rocket launcher
    if(job_ptr->phase == 0) {
        //	UTIL_BotLogPrintf("%s: Jumping - RJ waypoint %d\n", pBot->name, job_ptr->waypoint);
        //	UTIL_HostSay(pBot->pEdict, 0, "starting rocket jump");
        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + 5.0;
    }

    // phase 1 - get close enough then look down and press jump
    if(job_ptr->phase == 1) {
        // abort if the bot's taking too long with this phase
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // make sure the bot is facing the RJ waypoint
        const Vector v_aim = waypoints[job_ptr->waypoint].origin - pBot->pEdict->v.origin;
        if(BotInFieldOfView(pBot, v_aim) < 30) {
            const float dist2d = (waypoints[job_ptr->waypoint].origin - pBot->pEdict->v.origin).Length2D();

            // look down in preparation
            if(dist2d < 280.0) {
                pBot->pEdict->v.idealpitch = -90;
                BotChangePitch(pBot->pEdict, 99999);
            }

            if(dist2d < 200.0) {
                pBot->pEdict->v.button |= IN_JUMP;
                job_ptr->phase = 2;

                // set the delay between the jump and the shot
                job_ptr->phase_timer = pBot->f_think_time + random_float(0.0, 0.02);
                if(pBot->bot_skill > 2)
                    job_ptr->phase_timer += random_float(0.0, 0.03);

                //	UTIL_BotLogPrintf("%s: Jumping, velocity Z %f, time %f, firing at %f\n",
                //		pBot->name, pBot->pEdict->v.velocity.z, pBot->f_think_time,
                //		job_ptr->phase_timer);
            }
        }

        return JOB_UNDERWAY;
    }

    //	if(job_ptr->phase == 2)
    //		UTIL_BotLogPrintf("%s: firing, velocity Z %f, time %f\n",
    //			pBot->name, pBot->pEdict->v.velocity.z, pBot->f_think_time);

    // phase 2 - fire when ready
    if(job_ptr->phase == 2 && job_ptr->phase_timer < pBot->f_think_time) {
        // look down and fire
        pBot->pEdict->v.idealpitch = -90;
        BotChangePitch(pBot->pEdict, 99999);
        pBot->pEdict->v.button |= IN_ATTACK;
        job_ptr->phase = 3;
        job_ptr->phase_timer = pBot->f_think_time + 5.0;

        return JOB_UNDERWAY;
    }

    // phase 3 - gliding towards the RJ waypoint
    if(job_ptr->phase == 3) {
        //	UTIL_BotLogPrintf("%s: Gliding, velocity Z %f, time %f\n",
        //		pBot->name, pBot->pEdict->v.velocity.z, pBot->f_think_time);

        // abort if the bot's taking too long with this phase
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // check if the rocketjump succeeded
        if(pBot->pEdict->v.velocity.z < 5.0) {
            // UTIL_HostSay(pBot->pEdict,0," RJ done");
            return JOB_TERMINATED;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_CONCUSSION_JUMP job.
int JobConcussionJump(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // stop the bot from taking any route variations during this job
    pBot->f_side_route_time = pBot->f_think_time + 5.0;
    pBot->branch_waypoint = -1;

    // phase zero - prime the grenade
    if(job_ptr->phase == 0) {
        job_ptr->phase = 1;
        FakeClientCommand(pBot->pEdict, "+gren2", "101", NULL);
        pBot->primeTime = pBot->f_think_time;
        //	UTIL_HostSay(pBot->pEdict, 0, "Starting concussion jump!"); //DebugMessageOfDoom!
    }

    // phase 1 - countdown till the grenade goes off - then jump
    if(job_ptr->phase == 1) {
        const float timeToDet = 4.0f - (pBot->f_think_time - pBot->primeTime);

        // make sure the bot stops hopping about to evade enemies during this job
        pBot->f_dontEvadeTime = pBot->f_think_time + 1.0;

        // aim for the RJ waypoint at the last moment
        if(timeToDet < 0.8)
            BotSetFacing(pBot, waypoints[job_ptr->waypointTwo].origin);
        else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_CONCUSSION_JUMP, random_float(10.0, 20.0));
                return JOB_TERMINATED;
            }
        }

        // jump
        if(timeToDet <= 0.35f) {
            pBot->current_wp = job_ptr->waypointTwo;

            if(timeToDet <= 0.2f) {
                pBot->pEdict->v.button |= IN_JUMP;
                pBot->f_side_speed = 0;

                job_ptr->phase = 2;
            }
        }
    }

    // phase 2 - cruising through the air to the concussion jump waypoint
    if(job_ptr->phase == 2) {
        float speed = pBot->pEdict->v.velocity.Length2D();

        // debug stuff
        //	if(pBot->f_periodicAlert1 < pBot->f_think_time)
        //		WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
        //			waypoints[job_ptr->waypointTwo].origin, 10, 2, 250, 250, 250, 200, 10);

        BotSetFacing(pBot, waypoints[job_ptr->waypointTwo].origin);

        // head for the CJ waypoint
        pBot->pEdict->v.button |= IN_FORWARD;
        pBot->f_move_speed = pBot->f_max_speed;
        pBot->f_side_speed = 0.0;

        // Abort this routine once we are decending.
        if(pBot->pEdict->v.velocity.z < -1.0) {
            const float zDiff = (pBot->pEdict->v.origin.z - waypoints[job_ptr->waypointTwo].origin.z);
            const float dist2d = (waypoints[job_ptr->waypointTwo].origin - pBot->pEdict->v.origin).Length2D();

            pBot->f_duck_time = 0.0;

            // if the bot is descending, and lower than, or too far from the
            // concussion jump waypoint, assume the concussion jump failed
            if(zDiff < -50.0f || (zDiff < 100.0f && dist2d > 600.0f)) {
                BotFindCurrentWaypoint(pBot);
            } else
                pBot->current_wp = job_ptr->waypointTwo;

            //	UTIL_HostSay(pBot->pEdict, 0, "Concussion jump finished!"); //DebugMessageOfDoom!
            return JOB_TERMINATED;
        } else if(speed > 600.0)
            pBot->f_duck_time = pBot->f_think_time + 0.3;
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_DETPACK_WAYPOINT job.
int JobDetpackWaypoint(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - get to the detpack waypoint
    if(job_ptr->phase == 0) {
        // in position?
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 30.0)) {
            // abort if there's a detpack here already
            edict_t* pent = NULL;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "detpack")) != NULL && !FNullEnt(pent)) {
                if(VectorsNearerThan(pBot->pEdict->v.origin, pent->v.origin, 200.0))
                    return JOB_TERMINATED;
            }

            // abort if the waypoint has been detpacked already
            if(waypoints[pBot->current_wp].flags & W_FL_TFC_DETPACK_CLEAR && !DetpackClearIsBlocked(pBot->current_wp))
                return JOB_TERMINATED;
            if(waypoints[pBot->current_wp].flags & W_FL_TFC_DETPACK_SEAL && !DetpackSealIsClear(pBot->current_wp))
                return JOB_TERMINATED;

            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + random_float(2.0, 3.0);
            return JOB_UNDERWAY;
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_DETPACK_WAYPOINT, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 1 - set the detpack
    if(job_ptr->phase == 1) {
        // you can only detpack while touching land beneath you
        if(pBot->pEdict->v.flags & FL_ONGROUND) {
            FakeClientCommand(pBot->pEdict, "+det5", NULL, NULL);

            // find somewhere to run away to
            if(pBot->current_team > -1 && pBot->current_team < 4 && spawnAreaWP[pBot->current_team] != -1)
                job_ptr->waypoint = spawnAreaWP[pBot->current_team];

            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + random_float(12.0, 16.0);

            //	UTIL_HostSay(pBot->pEdict, 0, "DETPACKING NOW");  //DebugMessageOfDoom!
        } else if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED; // took too long
    }

    // phase 2 - run away!
    if(job_ptr->phase == 2) {
        // quit the job if you've been running away for long enough
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // make sure the bot stops hopping about to evade enemies during this job
        pBot->f_dontEvadeTime = pBot->f_think_time + 1.0;

        // stop the bot from taking large route variations during this job
        pBot->f_side_route_time = pBot->f_think_time + 5.0;
        pBot->sideRouteTolerance = 200; // very short route changes

        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_DETPACK_WAYPOINT, 10.0);
            return JOB_TERMINATED;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_PIPETRAP job.
int JobPipetrap(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - get to the pipetrap waypoint
    if(job_ptr->phase == 0) {
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 20.0)) {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            // make sure the bot faces the right way
            int aim_index = WaypointFindNearestAiming(waypoints[pBot->current_wp].origin);
            if(aim_index != -1) {
                BotSetFacing(pBot, waypoints[aim_index].origin);

                const Vector v_aim = waypoints[aim_index].origin - pBot->pEdict->v.origin;

                if(BotInFieldOfView(pBot, v_aim) == 0) {
                    job_ptr->phase = 1;
                    job_ptr->phase_timer = pBot->f_think_time + 8.0;
                }
            } else
                return JOB_TERMINATED;
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_PIPETRAP, random_float(10.0, 30.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 1 - reload
    if(job_ptr->phase == 1) {
        if(pBot->current_weapon.iId == TF_WEAPON_PL) {
            if(pBot->current_weapon.iClip < 6)
                pBot->pEdict->v.button |= IN_RELOAD;
            else
                job_ptr->phase = 2;
        } else
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_pl");

        // abort if this phase took too long(something went wrong)
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;
    }

    // phase 2 - laying the trap
    if(job_ptr->phase == 2) {
        // count the number of pipebombs the bot owns
        // ignore visibility because pipebombs can bounce behind other things
        int pipeBombTally = 0;
        edict_t* pent = NULL;
        while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "tf_gl_pipebomb")) != NULL && (!FNullEnt(pent))) {
            if(pBot->pEdict == pent->v.owner)
                ++pipeBombTally;
        }

        if(pipeBombTally < 4)
            pBot->pEdict->v.button |= IN_ATTACK;
        else {
            pBot->pEdict->v.button |= IN_RELOAD;
            job_ptr->phase = 3;
            job_ptr->phase_timer = pBot->f_think_time + random_float(180.0, 300.0);
        }
    }

    // phase 3 - wait for "customers" to come buy some whoop ass
    if(job_ptr->phase == 3) {
        // give up if the bots been waiting here too long
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // reset the job if the bots pipebombs have been destroyed for any reason
        int pipeBombTally = 0;
        edict_t* pent = NULL;
        while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "tf_gl_pipebomb")) != NULL && (!FNullEnt(pent))) {
            if(pBot->pEdict == pent->v.owner)
                ++pipeBombTally;
        }

        if(pipeBombTally < 4) {
            job_ptr->phase = 0;
            return JOB_UNDERWAY;
        }

        // look about occasionally
        if(pBot->f_view_change_time <= pBot->f_think_time) {
            pBot->f_view_change_time = pBot->f_think_time + random_float(1.0f, 4.0f);

            int aim_index = WaypointFindNearestAiming(waypoints[pBot->current_wp].origin);
            if(aim_index != -1) {
                Vector v_aim = waypoints[aim_index].origin - waypoints[pBot->current_wp].origin;
                Vector aim_angles = UTIL_VecToAngles(v_aim);
                pBot->pEdict->v.ideal_yaw = aim_angles.y + (random_long(0, 90) - 45);
                pBot->pEdict->v.idealpitch = 0 + (random_long(0, 20) - 10);
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_INVESTIGATE_AREA job.
// This job involves the bot going to a preselected area(possibly because
// the bot heard something it can't see and wants to check it out).
int JobInvestigateArea(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // stop the bot from taking large route variations during this job
    pBot->f_side_route_time = pBot->f_think_time + 5.0;
    pBot->sideRouteTolerance = 400; // very short route changes

    // check if the bot has arrived at the designated waypoint
    if(pBot->current_wp == job_ptr->waypoint &&
        VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 60.0)) {
        // wait for a random amount of time before choosing a new goal
        if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) < 500)
            job_ptr->waypoint = -1;
        else {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            // look about whilst waiting
            BotLookAbout(pBot);
        }
    } else {
        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_INVESTIGATE_AREA, random_float(10.0, 20.0));
            return JOB_TERMINATED;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_PURSUE_ENEMY job.
// If the bot can see the enemy move towards them, if not, try to guess where
// they have gone to and go there.
int JobPursueEnemy(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // added for readability
    enum phaseNames {
        SET_WAYPOINT_VISIBLE_ENEMY = 0, // must be 0 for the job to work
        PURSUE_VISIBLE_ENEMY,
        SET_WAYPOINT_UNSEEN_ENEMY,
        PURSUE_UNSEEN_ENEMY
    };

    // stop the bot from taking large route variations during this job
    pBot->f_side_route_time = pBot->f_think_time + 5.0;
    pBot->sideRouteTolerance = 100; // very short route changes

    // phase zero - we assume the enemy is visible and must find a waypoint near them
    if(job_ptr->phase == SET_WAYPOINT_VISIBLE_ENEMY) {
        job_ptr->waypoint = WaypointFindNearest_S(job_ptr->origin, NULL, 700.0, pBot->current_team, W_FL_DELETED);

        job_ptr->phase = PURSUE_VISIBLE_ENEMY;
        return JOB_UNDERWAY;
    }

    // phase 1 - go for the waypoint nearest to the enemy(who we assume is visible)
    if(job_ptr->phase == PURSUE_VISIBLE_ENEMY) {
        // if the enemy is no longer visible - switch to the phase for handling that
        if(pBot->enemy.ptr != job_ptr->player) {
            job_ptr->phase = SET_WAYPOINT_UNSEEN_ENEMY;
            return JOB_UNDERWAY;
        } else
            job_ptr->origin = job_ptr->player->v.origin; // remember where we saw them

        // terminate the job if the bot arrived at the chosen waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 50.0))
            return JOB_TERMINATED;
        else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_PURSUE_ENEMY, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 2 - assuming the enemy is not visible, make a guess at where the
    // bot can find them
    if(job_ptr->phase == SET_WAYPOINT_UNSEEN_ENEMY) {
        job_ptr->waypoint = BotGuessPlayerPosition(pBot, job_ptr->origin);

        //	UTIL_HostSay(pBot->pEdict, 0, "get over here!"); //DebugMessageOfDoom!

        job_ptr->phase = PURSUE_UNSEEN_ENEMY;

        // pursue longer if the enemy has a flag
        if(PlayerHasFlag(job_ptr->player))
            job_ptr->phase_timer = pBot->f_think_time + random_float(45.0, 60.0);
        else
            job_ptr->phase_timer = pBot->f_think_time + random_float(15.0, 25.0);
        return JOB_UNDERWAY;
    }

    // phase 3 - assuming the enemy is not visible, go for the waypoint where
    // the bot may find them
    if(job_ptr->phase == PURSUE_UNSEEN_ENEMY) {
        // if the enemy is visible again - switch to the phase for handling that
        if(pBot->enemy.ptr == job_ptr->player) {
            job_ptr->phase = SET_WAYPOINT_VISIBLE_ENEMY;
            return JOB_UNDERWAY;
        }

        // give up if the bot hasn't seen the enemy it's pursuing for a while
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // terminate the job if the bot arrived at the chosen waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(pBot->pEdict->v.origin, waypoints[job_ptr->waypoint].origin, 50.0))
            return JOB_TERMINATED;
        else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_PURSUE_ENEMY, random_float(5.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_PATROL_HOME job.
// This job involves the bot patrolling it's teams defensive area looking
// out for enemy intruders.
int JobPatrolHome(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // make sure the bot has a waypoint to patrol to
    if(job_ptr->waypoint == -1 || WaypointRouteFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team) == -1) {
        // waypoint types to patrol to
        const int wantedFlags = (W_FL_TFC_PL_DEFEND | W_FL_TFC_SENTRY | W_FL_TFC_PIPETRAP);

        int defencePoint = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, wantedFlags);

        if(defencePoint != -1)
            job_ptr->waypoint = WaypointFindRandomGoal_R(waypoints[defencePoint].origin, TRUE, 500.0, -1, 0);

        // abort and blacklist the job for a while if no waypoint was found
        if(job_ptr->waypoint == -1) {
            //	UTIL_HostSay(pBot->pEdict, 0, "no patrol goal found"); //DebugMessageOfDoom!
            BlacklistJob(pBot, JOB_PATROL_HOME, random_float(30.0, 60.0));
            return JOB_TERMINATED;
        }

        return JOB_UNDERWAY; // to conserve the CPU use the new goal on the next program cycle
    }

    // check if the bot has arrived at it's next patrol point
    if(pBot->current_wp == job_ptr->waypoint &&
        VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 60.0)) {
        // wait for a random amount of time before choosing a new goal
        if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) < 300)
            job_ptr->waypoint = -1;
        else {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            // look about whilst waiting
            BotLookAbout(pBot);
        }
    } else {
        pBot->goto_wp = job_ptr->waypoint;
        if(!BotNavigateWaypoints(pBot, FALSE)) {
            BlacklistJob(pBot, JOB_PATROL_HOME, random_float(5.0, 10.0));
            return JOB_TERMINATED;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_SPOT_STIMULUS job.
// i.e. make the bot turn to face an interesting event that is not in it's view cone.
int JobSpotStimulus(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - set up how long the bot will look at the stimulus
    if(job_ptr->phase == 0) {
        // also decide whether the bot will stop and look, or keep moving
        bool moveNlook = FALSE;
        if(pBot->current_wp != job_ptr->waypoint && job_ptr->waypoint > -1 && job_ptr->waypoint < num_waypoints) {
            if(pBot->bot_skill > 2) // low skill bots
            {
                if(random_long(0, 1000) < 400)
                    moveNlook = TRUE;
            } else // skills 0 - 2
            {
                if(pBot->trait.aggression > 50 || pBot->pEdict->v.waterlevel == WL_HEAD_IN_WATER ||
                    (pBot->f_injured_time + 2.0) > pBot->f_think_time || random_long(0, 1000) > 300)
                    moveNlook = TRUE;
            }
        }

        if(moveNlook == TRUE) {
            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + 0.5 + (pBot->bot_skill * 0.5) + random_float(0.5, 1.5);
        } else {
            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + random_float(0.7, 1.4);
        }
    }

    // phase 1 - stop and look at whatever got the bots attention
    if(job_ptr->phase == 1) {
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // a random jump or duck, might help if shot at
        if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) < 501) {
            if(random_long(1, 1000) < 501)
                pBot->pEdict->v.button |= IN_JUMP;
            else
                pBot->f_duck_time = pBot->f_think_time + random_float(0.4, 0.6);
        }

        BotSetFacing(pBot, job_ptr->origin);

        // give the bot time to return to it's waypoint afterwards
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        pBot->f_move_speed = 0.0;
        pBot->f_side_speed = 0.0;
    }

    // phase 2 - keep moving whilst looking at whatever got the bots attention
    if(job_ptr->phase == 2) {
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        BotSetFacing(pBot, job_ptr->origin);

        if(pBot->current_wp != job_ptr->waypoint && job_ptr->waypoint > -1 && job_ptr->waypoint < num_waypoints) {
            if(!BotNavigateWaypoints(pBot, TRUE)) {
                BlacklistJob(pBot, JOB_SPOT_STIMULUS, 5.0);
                return JOB_TERMINATED;
            }
        } else // just in case
        {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_ATTACK_BREAKABLE job.
// i.e. head over to a breakable object and smack it up.
int JobAttackBreakable(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - set up how long to keep trying
    if(job_ptr->phase == 0) {
        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + 10.0;
        job_ptr->origin = VecBModelOrigin(job_ptr->object);
    }

    // phase 1 - smashing time!
    if(job_ptr->phase == 1) {
        // give up if the bots been trying(and failing) for too long
        if(job_ptr->phase_timer < pBot->f_think_time) {
            BlacklistJob(pBot, JOB_ATTACK_BREAKABLE, random_float(3.0, 6.0));
            return JOB_TERMINATED;
        }

        // trace a line from the bot's eyes to the func_breakable entity...
        TraceResult tr;
        UTIL_TraceLine(pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs, job_ptr->origin, dont_ignore_monsters,
            pBot->pEdict->v.pContainingEntity, &tr);

        // lost sight of it?
        if(tr.flFraction < 1.0 && tr.pHit != job_ptr->object) {
            BlacklistJob(pBot, JOB_ATTACK_BREAKABLE, random_float(3.0, 6.0));
            return JOB_TERMINATED;
        }

        // use a suitable weapon, based on the bots class
        if(pBot->pEdict->v.playerclass == TFC_CLASS_SCOUT || pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN ||
            pBot->pEdict->v.playerclass == TFC_CLASS_PYRO) {
            if(pBot->current_weapon.iId != TF_WEAPON_SHOTGUN)
                UTIL_SelectItem(pBot->pEdict, "tf_weapon_shotgun");
        } else if(pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER && pBot->current_weapon.iId != TF_WEAPON_AUTORIFLE)
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_autorifle");
        else if(pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN)
            ; // the umbrella will do
        else if(pBot->current_weapon.iId != TF_WEAPON_SUPERSHOTGUN)
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_supershotgun");

        BotSetFacing(pBot, job_ptr->origin);
        BotNavigateWaypointless(pBot);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        // near enough to attack it?
        if(VectorsNearerThan(pBot->pEdict->v.origin, job_ptr->origin, 300.0)) {
            // duck if it is lower than the bots head
            if(job_ptr->origin.z < (pBot->pEdict->v.origin.z + pBot->pEdict->v.view_ofs.z))
                pBot->f_duck_time = pBot->f_think_time + 0.3;

            if(pBot->current_weapon.iId == TF_WEAPON_SHOTGUN || pBot->current_weapon.iId == TF_WEAPON_SUPERSHOTGUN) {
                if(pBot->current_weapon.iClip > 1)
                    pBot->pEdict->v.button |= IN_ATTACK;
                else
                    pBot->pEdict->v.button |= IN_RELOAD;
            } else
                pBot->pEdict->v.button |= IN_ATTACK;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_ATTACK_TELEPORT job.
// i.e. the bot has seen an enemy teleporter and now wants to destroy it.
int JobAttackTeleport(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - set a waypoint near the Teleport
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = WaypointFindNearest_E(job_ptr->object, 600.0, pBot->current_team);
        job_ptr->phase = 1;
        return JOB_UNDERWAY; // reduce CPU consumption a bit
    }

    // phase 1 - get to the waypoint
    if(job_ptr->phase == 1) {
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 40.0)) {
            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + random_float(7.0, 12.0);
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_ATTACK_TELEPORT, random_float(10.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 2 - go to the teleport, then shotgun it
    if(job_ptr->phase == 2) {
        // time out if this is taking too long or the bot can't see the teleporter
        if(job_ptr->phase_timer < pBot->f_think_time || !BotCanSeeOrigin(pBot, job_ptr->object->v.origin)) {
            BlacklistJob(pBot, JOB_ATTACK_TELEPORT, 5.0);
            return JOB_TERMINATED;
        }

        // make sure the bot has a shotgun out
        if(pBot->pEdict->v.playerclass == TFC_CLASS_SCOUT || pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN ||
            pBot->pEdict->v.playerclass == TFC_CLASS_PYRO) {
            if(pBot->current_weapon.iId != TF_WEAPON_SHOTGUN)
                UTIL_SelectItem(pBot->pEdict, "tf_weapon_shotgun");
        } else if(pBot->current_weapon.iId != TF_WEAPON_SUPERSHOTGUN)
            UTIL_SelectItem(pBot->pEdict, "tf_weapon_supershotgun");

        BotSetFacing(pBot, job_ptr->object->v.origin);

        const float dist2D = (pBot->pEdict->v.origin - job_ptr->object->v.origin).Length2D();

        // near enough to attack it?
        if(dist2D < 100.0) {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;

            if(pBot->current_weapon.iId == TF_WEAPON_SHOTGUN || pBot->current_weapon.iId == TF_WEAPON_SUPERSHOTGUN) {
                if(pBot->current_weapon.iClip > 1)
                    pBot->pEdict->v.button |= IN_ATTACK;
                else
                    pBot->pEdict->v.button |= IN_RELOAD;
            }
        } else // approach the teleporter
        {
            BotNavigateWaypointless(pBot);
            pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_SEEK_BACKUP job.
// i.e. make the bot go and find an ally, for example when it is outnumbered
// by enemies.  This job can be particularly useful for civilian bots.
int JobSeekBackup(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - set a waypoint near the last seen allies last known location
    if(job_ptr->phase == 0) {
        //	UTIL_HostSay(pBot->pEdict, 0, "JOB_SEEK_BACKUP started");
        job_ptr->waypoint = WaypointFindNearest_S(pBot->lastAllyVector, NULL, 700.0, pBot->current_team, W_FL_DELETED);

        job_ptr->phase = 1;
        return JOB_UNDERWAY;
    }

    // phase 1 - go to the waypoint near to where the last seen ally was seen
    if(job_ptr->phase == 1) {
        // debugging stuff
        /*	WaypointDrawBeam(INDEXENT(1),
                        pBot->pEdict->v.origin, pBot->lastAllyVector,
                        10, 2, 50, 250, 250, 200, 10);*/

        // check if an enemy is blocking the route to the bots backup
        if(pBot->enemy.ptr != NULL) {
            const int nextWP = WaypointRouteFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team);

            // is the waypoint the bot is going to nearer to the enemy?
            if(nextWP == -1 ||
                VectorsNearerThan(waypoints[nextWP].origin, pBot->enemy.ptr->v.origin, pBot->enemy.f_seenDistance)) {
                BlacklistJob(pBot, JOB_SEEK_BACKUP, 5.0);
                return JOB_TERMINATED;
            }
        }

        // make sure the bot stops hopping about to evade enemies during this job
        pBot->f_dontEvadeTime = pBot->f_think_time + 1.0;

        // stop the bot from taking large route variations during this job
        pBot->f_side_route_time = pBot->f_think_time + 5.0;
        pBot->sideRouteTolerance = 200; // very short route changes

        // check if the bot has arrived at the waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 60.0))
            return JOB_TERMINATED; // job done
        else {
            if(pBot->visAllyCount > 2 && (pBot->enemy.f_lastSeen + 5.0) < pBot->f_think_time)
                return JOB_TERMINATED; // job done(assume bot has found an ally)

            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_SEEK_BACKUP, random_float(5.0, 10.0));
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_AVOID_ENEMY job.
// i.e. head to a pre-assigned waypoint that is some distance from an enemy
// and not visible to that enemy.
int JobAvoidEnemy(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - set up the waypoint and the maximum time the bot will retreat for
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = BotFindRetreatPoint(pBot, 800, job_ptr->player->v.origin);

        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + random_float(12.0, 20.0);
    }

    // phase 1 - get to the waypoint the bot is retreating to
    if(job_ptr->phase == 1) {
        // check if an enemy is blocking the route to the bots backup
        if(pBot->enemy.ptr != NULL) {
            const int nextWP = WaypointRouteFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team);

            // is the waypoint the bot is going to nearer to the enemy?
            if(nextWP == -1 ||
                VectorsNearerThan(waypoints[nextWP].origin, pBot->enemy.ptr->v.origin, pBot->enemy.f_seenDistance)) {
                return JOB_TERMINATED;
            }
        }

        // make sure the bot stops hopping about to evade enemies during this job
        pBot->f_dontEvadeTime = pBot->f_think_time + 1.0;

        // stop the bot from taking large route variations during this job
        pBot->f_side_route_time = pBot->f_think_time + 5.0;
        pBot->sideRouteTolerance = 100; // very short route changes

        // check if the bot has arrived at the waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 60.0)) {
            // don't wait here if the bot is fighting
            if(pBot->enemy.ptr != NULL)
                return JOB_TERMINATED;

            // don't wait here if the bot is underwater and injured much
            if(pBot->pEdict->v.waterlevel == WL_HEAD_IN_WATER && PlayerHealthPercent(pBot->pEdict) < 60)
                return JOB_TERMINATED;

            // been waiting here for long enough?
            if(job_ptr->phase_timer < pBot->f_think_time)
                return JOB_TERMINATED;

            // has the cavalry arrived?
            if((pBot->enemy.f_lastSeen + 5.0) < pBot->f_think_time && pBot->visAllyCount > 1)
                return JOB_TERMINATED;

            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;
            BotLookAbout(pBot);
            return JOB_UNDERWAY;
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_AVOID_ENEMY, 5.0);
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_AVOID_AREA_DAMAGE job.
// i.e. head to a pre-assigned waypoint that is some distance from a temporary entity
// that will cause area damage.  e.g. hand grenades or a detpack.
int JobAvoidAreaDamage(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // point at the object the bot is avoiding(for debugging purposes)
    //	if(!FNullEnt(job_ptr->object))
    //		WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
    //			job_ptr->object->v.origin, 10, 2, 250, 50, 50, 200, 10);

    // phase zero - set up the waypoint and the maximum time the bot will retreat for
    if(job_ptr->phase == 0) {
        /*	// debug info
                char classname[30];
                strncpy(classname, STRING(job_ptr->object->v.classname), 30);
                classname[29] = '\0';
                UTIL_BotLogPrintf("%s: avoiding %s at %f,%f,%f\n", pBot->name, classname,
                        job_ptr->object->v.origin.x, job_ptr->object->v.origin.y,
                        job_ptr->object->v.origin.z);*/

        job_ptr->waypoint = BotFindThreatAvoidPoint(pBot, 700, job_ptr->object);
        job_ptr->phase_timer = pBot->f_think_time + random_float(12.0, 20.0);

        // check the waypoint returned
        if(WaypointAvailable(job_ptr->waypoint, pBot->current_team) &&
            WaypointRouteFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team) != -1)
            job_ptr->phase = 1; // plan A - found a waypoint the bot can get to
        else {
            //	UTIL_HostSay(pBot->pEdict, 0, "JOB_AVOID_AREA_DAMAGE - bad path"); //DebugMessageOfDoom!
            job_ptr->phase = 2; // time for plan B - no waypoint found to get to
        }
    }

    // phase 1 - get to the waypoint the bot is retreating to
    if(job_ptr->phase == 1) {
        // been avoiding the threat for longer than the bots patience will allow?
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // check the waypoint is still reachable
        if(WaypointRouteFromTo(pBot->current_wp, job_ptr->waypoint, pBot->current_team) == -1)
            return JOB_TERMINATED;

        // make sure the bot stops hopping about to evade enemies during this job
        pBot->f_dontEvadeTime = pBot->f_think_time + 1.0;

        // stop the bot from taking large route variations during this job
        pBot->f_side_route_time = pBot->f_think_time + 5.0;
        pBot->sideRouteTolerance = 10; // very short route changes

        // check if the bot has arrived at it's retreat waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 60.0)) {
            pBot->f_move_speed = 0.0;
            pBot->f_side_speed = 0.0;
            BotLookAbout(pBot);
            return JOB_UNDERWAY;
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_AVOID_AREA_DAMAGE, 5.0);
                return JOB_TERMINATED;
            }
        }
    }

    // phase 2 - the bot can't get to a waypoint so run backwards until
    // a good distance from the threat is achieved
    if(job_ptr->phase == 2) {
        // been avoiding the threat for longer than the bots patience will allow?
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        if(VectorsNearerThan(pBot->pEdict->v.origin, job_ptr->object->v.origin, 400.0))
            pBot->f_move_speed = -pBot->f_max_speed;
        else
            pBot->f_pause_time = pBot->f_think_time + 0.2;
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_INFECTED_ATTACK job.
// i.e. charge into enemy base and assault all enemy defenders.
int JobInfectedAttack(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - make sure the bot has somewhere to go
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = BotFindSuicideGoal(pBot);
        if(job_ptr->waypoint != -1) {
            job_ptr->phase = 1;
            //	UTIL_HostSay(pBot->pEdict, 0, "JOB_INFECTED_ATTACK waypoint success"); //DebugMessageOfDoom!
            return JOB_UNDERWAY;
        } else {
            //	UTIL_HostSay(pBot->pEdict, 0, "JOB_INFECTED_ATTACK waypoint failure"); //DebugMessageOfDoom!
            return JOB_TERMINATED;
        }
    }

    // phase 1 - get to the waypoint
    if(job_ptr->phase == 1) {
        // make sure the bot stops hopping about to evade enemies during this job
        pBot->f_dontEvadeTime = pBot->f_think_time + 1.0;

        // stop the bot from taking large route variations during this job
        pBot->f_side_route_time = pBot->f_think_time + 5.0;
        pBot->sideRouteTolerance = 400; // very short route changes

        // check if the bot has arrived at it's chosen waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 60.0)) {
            if(pBot->visEnemyCount < 1) {
                pBot->f_move_speed = 0.0;
                BotLookAbout(pBot);
            } else
                pBot->f_move_speed = pBot->f_max_speed;
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_INFECTED_ATTACK, random_float(5.0, 15.0));
                return JOB_TERMINATED;
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_BIN_GRENADE job.
// i.e. throw a primed grenade away to avoid suicide.
int JobBinGrenade(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // this job doesn't last long - a debug thingy to show when it runs
    //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
    //		pBot->pEdict->v.origin + Vector(0, 0, 100.0), 10, 2, 250, 50, 250, 200, 10);

    // phase zero - figure out where to throw the grenade
    if(job_ptr->phase == 0) {
        // can we throw at where the enemy was last seen?
        job_ptr->origin = pBot->enemy.lastLocation;
        int targetWP = BotFindGrenadePoint(pBot, job_ptr->origin);

        // if that fails, find an open space to throw at
        if(targetWP == -1)
            targetWP = WaypointFindInRange(pBot->pEdict->v.origin, 400.0f, 1200.0f, pBot->current_team, FALSE);

        if(targetWP != -1)
            job_ptr->origin = waypoints[targetWP].origin;
        else
            return JOB_TERMINATED; // couldn't find anywhere to throw

        job_ptr->phase = 1;
    }

    // phase 1 - face where to throw the grenade
    if(job_ptr->phase == 1) {
        pBot->f_side_speed = 0.0;
        pBot->f_move_speed = 0.0;
        BotSetFacing(pBot, job_ptr->origin);

        const Vector v_aim = job_ptr->origin - pBot->pEdict->v.origin;

        if(BotInFieldOfView(pBot, v_aim) == 0) {
            // thrown the grenade yet?
            if(pBot->nadePrimed == FALSE) {
                job_ptr->phase = 2;
                job_ptr->phase_timer = pBot->f_think_time + 1.0;
            }
            return JOB_UNDERWAY;
        } else
            pBot->f_move_speed = 0.0;
    }

    // phase 2 - keep backing away for a second or two, after throwing the grenade
    if(job_ptr->phase == 2) {
        // backed away for long enough?
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        BotSetFacing(pBot, job_ptr->origin);
        pBot->f_side_speed = 0.0;
        pBot->f_move_speed = -pBot->f_max_speed;
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_DROWN_RECOVER job.
// i.e. swim to a waypoint near or above the surface, and surface so the
// bot can breathe again.
int JobDrownRecover(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - get to the waypoint
    if(job_ptr->phase == 0) {
        // stop the bot from taking large route variations during this job
        pBot->f_side_route_time = pBot->f_think_time + 5.0;
        pBot->sideRouteTolerance = 100; // very short route changes

        // check if the bot has arrived at it's chosen waypoint
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->pEdict->v.origin, 60.0)) {
            job_ptr->phase = 1;
            job_ptr->phase_timer = pBot->f_think_time + 5.0;
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_DROWN_RECOVER, random_float(5.0, 10.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 1 - swim upwards for a few seconds till the bot surfaces(hopefully!)
    if(job_ptr->phase == 1) {
        // took too long?
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        pBot->pEdict->v.idealpitch = 90;
        BotChangePitch(pBot->pEdict, 99999);
        pBot->pEdict->v.button |= IN_FORWARD;
        pBot->f_move_speed = pBot->f_max_speed;
        pBot->f_side_speed = 0.0;
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_MELEE_WARRIOR job.
// i.e. make the bot go roam for a while attacking any enemies seen with
// melee weapons only.  This job is meant for humour purposes only!
int JobMeleeWarrior(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // melee weapons only foo!
    if(pBot->pEdict->v.playerclass == TFC_CLASS_SPY)
        UTIL_SelectItem(pBot->pEdict, "tf_weapon_knife");
    else if(pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER)
        UTIL_SelectItem(pBot->pEdict, "tf_weapon_spanner");
    else if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC)
        UTIL_SelectItem(pBot->pEdict, "tf_weapon_medikit");
    else if(pBot->pEdict->v.playerclass != TFC_CLASS_CIVILIAN && pBot->current_weapon.iId != TF_WEAPON_AXE)
        UTIL_SelectItem(pBot->pEdict, "tf_weapon_axe");

    // always go for your enemies
    if(pBot->enemy.ptr != NULL) {
        BotSetFacing(pBot, pBot->enemy.ptr->v.origin);
        BotNavigateWaypointless(pBot);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        pBot->pEdict->v.button |= IN_ATTACK;

        // override the normal combat behaviour
        pBot->f_shoot_time = pBot->f_think_time + 1.0;

        return JOB_UNDERWAY;
    } else {
        // there's always time for a few practice swings!
        if(pBot->f_periodicAlertFifth < pBot->f_think_time && random_long(1, 1000) < 91)
            pBot->pEdict->v.button |= IN_ATTACK;
    }

    // phase 0 - pick a waypoint to visit
    if(job_ptr->phase == 0) {
        job_ptr->waypoint = WaypointFindRandomGoal(pBot->current_wp, -1, 0);
        job_ptr->phase = 1;
        return JOB_UNDERWAY;
    }

    // phase 1 - get to the waypoint
    if(job_ptr->phase == 1) {
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 60.0)) {
            job_ptr->phase = 2;
            job_ptr->phase_timer = pBot->f_think_time + random_float(2.0, 6.0);
        } else {
            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_MELEE_WARRIOR, random_float(10.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 2 - wait for a bit here
    if(job_ptr->phase == 2) {
        if(job_ptr->phase_timer < pBot->f_think_time) {
            // maybe give up, maybe go look for trouble elsewhere
            if(random_long(1, 1000) < 333)
                job_ptr->phase = 0;
            else
                return JOB_TERMINATED;
        } else {
            // make sure the bot is in the right place looking about
            if(!VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 60.0))
                job_ptr->phase = 1;
            else {
                if(random_long(1, 1000) < 100)
                    pBot->f_duck_time = pBot->f_think_time + random_float(0.2, 1.5);

                pBot->f_move_speed = 0.0;
                pBot->f_side_speed = 0.0;
                BotLookAbout(pBot);
            }
        }
    }

    return JOB_UNDERWAY;
}

// This function handles bot behaviour for the JOB_GRAFFITI_ARTIST job.
// i.e. the bot will try to pick out a wall in the area and go spray it.
int JobGraffitiArtist(bot_t* pBot)
{
    job_struct* job_ptr = &pBot->job[pBot->currentJob];

    // phase zero - pick a waypoint to go to
    if(job_ptr->phase == 0) {
        //	UTIL_HostSay(pBot->pEdict, 0, "JOB_GRAFFITI_ARTIST - Starting"); //DebugMessageOfDoom!
        job_ptr->waypoint = WaypointFindRandomGoal_R(pBot->pEdict->v.origin, FALSE, 1000.0, -1, 0);

        job_ptr->phase = 1;
        job_ptr->phase_timer = pBot->f_think_time + random_float(45.0, 60.0);
        return JOB_UNDERWAY;
    }

    // phase 1 - go to the waypoint
    if(job_ptr->phase == 1) {
        // took too long?
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        // check if the bot has arrived
        if(pBot->current_wp == job_ptr->waypoint &&
            VectorsNearerThan(waypoints[job_ptr->waypoint].origin, pBot->pEdict->v.origin, 60.0)) {
            job_ptr->phase = 2;
        } else {
            // stop the bot from taking large route variations during this job
            pBot->f_side_route_time = pBot->f_think_time + 5.0;
            pBot->sideRouteTolerance = 400; // very short route changes

            pBot->goto_wp = job_ptr->waypoint;
            if(!BotNavigateWaypoints(pBot, FALSE)) {
                BlacklistJob(pBot, JOB_GRAFFITI_ARTIST, random_float(10.0, 20.0));
                return JOB_TERMINATED;
            }
        }
    }

    // phase 2 - look for a wall fairly near to this waypoint
    if(job_ptr->phase == 2) {
        // try a few random directions, looking for a nearby wall
        for(int i = 0; i < 4; i++) {
            Vector newAngle = Vector(0.0, random_float(-180.0, 180.0), 0.0);
            UTIL_MakeVectors(newAngle);

            // do a trace 400 units ahead of the new view angle to check for sight barriers
            const Vector v_forwards = pBot->pEdict->v.origin + (gpGlobals->v_forward * 400.0);

            TraceResult tr;
            UTIL_TraceLine(pBot->pEdict->v.origin, v_forwards, ignore_monsters, NULL, &tr);

            // if the new view angle is blocked assume the bot can spray here
            if(tr.flFraction < 1.0) {
                job_ptr->origin = tr.vecEndPos; // remember the walls location
                job_ptr->phase = 3;
                job_ptr->phase_timer = pBot->f_think_time + random_float(9.0, 11.0);
                return JOB_UNDERWAY;
            }
        }

        return JOB_TERMINATED; // couldn't find a wall to graffiti
    }

    // phase 3 - walk towards the wall, then spray it
    if(job_ptr->phase == 3) {
        // took too long?
        if(job_ptr->phase_timer < pBot->f_think_time)
            return JOB_TERMINATED;

        BotSetFacing(pBot, job_ptr->origin);
        BotNavigateWaypointless(pBot);

        if(VectorsNearerThan(pBot->pEdict->v.origin, job_ptr->origin, 60.0)) {
            BotSprayLogo(pBot->pEdict, FALSE);
            return JOB_TERMINATED; // success
        }
    }

    return JOB_UNDERWAY;
}
