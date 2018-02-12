//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_navigate.cpp
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
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_job_think.h"
#include "waypoint.h"
#include "bot_navigate.h"
#include "bot_weapons.h"
#include "list.h"

extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern edict_t* clients[32];
extern bot_t bots[32];

extern int mod_id;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern PATH* paths[MAX_WAYPOINTS];
extern int num_waypoints; // number of waypoints currently in use

extern bool g_waypoint_on;

extern edict_t* pent_info_ctfdetect;
extern float is_team_play;
extern bool checked_teamplay;

extern int bot_use_grenades;
extern bool bot_can_use_teleporter;

// Rocket jump globals
extern int RJPoints[MAXRJWAYPOINTS][2];

int CheckTeleporterExitTime = 0;

extern int flf_bug_fix;
extern bool g_bot_debug;

extern float last_frame_time;

// bit field of waypoint types to ignore when the bot is lost
// and looking for a new current waypoint to head for
static const WPT_INT32 lostBotIgnoreFlags = 0 +
    (W_FL_DELETED | W_FL_AIMING | W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY | W_FL_TFC_DETPACK_CLEAR |
                                                W_FL_TFC_DETPACK_SEAL |
                                                W_FL_SNIPER |
                                                W_FL_TFC_TELEPORTER_ENTRANCE |
                                                W_FL_TFC_TELEPORTER_EXIT |
                                                W_FL_TFC_JUMP |
                                                W_FL_LIFT |
                                                W_FL_PATHCHECK);

int spawnAreaWP[4] = { -1, -1, -1, -1 }; // used for tracking the areas where each team spawns
extern int team_allies[4];

// FUNCTION PROTOTYPES
static int BotShouldJumpOver(bot_t* pBot);
static int BotShouldDuckUnder(bot_t* pBot);
static bool BotFallenOffCheck(bot_t* const pBot);
static bool BotEscapeWaypointTrap(bot_t* const pBot, const int goalWP);
static bool BotUpdateRoute(bot_t* pBot);
static void BotHandleLadderTraffic(bot_t* pBot);
static void BotCheckForRocketJump(bot_t* pBot);
static void BotCheckForConcJump(bot_t* pBot);

// This function allows bots to report in the waypoint they last spawned
// nearby.
// This should be run each time a bot spawns, so as to keep the information
// up to date on maps such as warpath where the spawn areas keep changing.
void BotUpdateHomeInfo(bot_t* pBot)
{
    if(mod_id != TFC_DLL)
        return;

    // if the spawn location is totally unknown try to update it now
    if(spawnAreaWP[pBot->current_team] < 0 && (pBot->f_killed_time + 15.0f) > gpGlobals->time) {
        spawnAreaWP[pBot->current_team] = WaypointFindNearest_V(pBot->pEdict->v.origin, 800.0, pBot->current_team);

        return;
    }

    // keep the spawn area info up to date
    if(pBot->current_wp != -1 && (pBot->f_killed_time + 4.0f) > gpGlobals->time) {
        spawnAreaWP[pBot->current_team] = pBot->current_wp;
    }
}

// This function should be called each time the map changes.
// It resets the bots knowledge of where they are spawning.
void ResetBotHomeInfo(void)
{
    // sanity check
    if(mod_id != TFC_DLL)
        return;

    spawnAreaWP[0] = -1;
    spawnAreaWP[1] = -1;
    spawnAreaWP[2] = -1;
    spawnAreaWP[3] = -1;
}

// This function should be used when the bot has just spawned and has no current
// waypoint yet, because it also updates the bots knowledge of where it spawned.
void BotFindCurrentWaypoint(bot_t* pBot)
{
    Vector distance;
    double distance_squared;
    int min_index = -1;
    double min_distance_squared = 640000.0f; // 800 * 800 = 640000
    int runnerUp = -1;
    TraceResult tr;

    // find the nearest waypoint...
    for(int index = 0; index < num_waypoints; index++) {
        // skip unwanted waypoints
        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING ||
            waypoints[index].flags & lostBotIgnoreFlags)
            continue;

        // skip this waypoint if it's team specific and teams don't match
        if(waypoints[index].flags & W_FL_TEAM_SPECIFIC && (waypoints[index].flags & W_FL_TEAM) != pBot->current_team)
            continue;

        // don't give the bot the same waypoint it already has,
        // some waypoints are cursed
        if(index == pBot->current_wp)
            continue;

        // square the Manhattan distance, this way we can avoid using sqrt()
        distance = waypoints[index].origin - pBot->pEdict->v.origin;
        distance_squared = (distance.x * distance.x) + (distance.y * distance.y) + (distance.z * distance.z);

        // if the waypoint is above the bot only remember it in case no
        // other waypoint could be found(higher waypoints may be unreachable)
        if((pBot->pEdict->v.origin.z + 50.0) < waypoints[index].origin.z) {
            if(runnerUp == -1 && distance_squared < 640000.0) // 800 * 800 = 640000
            {
                // check if the waypoint is visible to the bot
                UTIL_TraceLine(pBot->pEdict->v.origin, waypoints[index].origin, dont_ignore_monsters,
                    pBot->pEdict->v.pContainingEntity, &tr);

                // it is visible, so store it
                if(tr.flFraction >= 1.0)
                    runnerUp = index;
            }

            continue;
        }

        // if it's the nearest found so far
        if(distance_squared < min_distance_squared) {
            // check if the waypoint is visible to the bot
            UTIL_TraceLine(pBot->pEdict->v.origin, waypoints[index].origin, dont_ignore_monsters,
                pBot->pEdict->v.pContainingEntity, &tr);

            // it is visible, so store it
            if(tr.flFraction >= 1.0) {
                min_index = index;
                min_distance_squared = distance_squared;
            }
        }
    }

    if(min_index == -1)
        pBot->current_wp = runnerUp;
    else
        pBot->current_wp = min_index;

    // give the bot time to arrive at the new waypoint
    pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

    // if the bot just spawned make sure it has a current waypoint
    if(pBot->current_wp != -1 && (pBot->f_killed_time + 4.0) > pBot->f_think_time) {
        BotUpdateHomeInfo(pBot); // if the bot just spawned, report where

        //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin + Vector(0, 0, 70),
        //		waypoints[pBot->current_wp].origin,
        //		10, 2, 250, 250, 250, 200, 10);
    }
}

CBaseEntity* UTIL_FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius)
{
    edict_t* pentEntity = NULL;

    if(pStartEntity != NULL)
        pentEntity = pStartEntity->edict();

    pentEntity = FIND_ENTITY_IN_SPHERE(pentEntity, vecCenter, flRadius);

    if(!FNullEnt(pentEntity))
        return CBaseEntity::Instance(pentEntity);
    return NULL;
}

// This function will tell the bot to face the map coordinates indicated by v_focus
void BotSetFacing(bot_t* pBot, Vector v_focus)
{
    v_focus = v_focus - (pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs);
    const Vector bot_angles = UTIL_VecToAngles(v_focus);
    pBot->pEdict->v.ideal_yaw = bot_angles.y;
    pBot->pEdict->v.idealpitch = bot_angles.x;
    BotFixIdealYaw(pBot->pEdict);
    BotFixIdealPitch(pBot->pEdict);
}

// This function will tell the bot to face the map coordinates
// indicated by v_focus
void BotMatchFacing(bot_t* pBot, Vector v_source, Vector v_focus)
{
    v_focus = v_focus - (pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs);
    const Vector bot_angles = UTIL_VecToAngles(v_focus);
    pBot->pEdict->v.ideal_yaw = bot_angles.y;
    pBot->pEdict->v.idealpitch = bot_angles.x;
    BotFixIdealYaw(pBot->pEdict);
    BotFixIdealPitch(pBot->pEdict);
}

void BotFixIdealPitch(edict_t* pEdict)
{
    // check for wrap around of angle...
    if(pEdict->v.idealpitch > 180)
        pEdict->v.idealpitch -= 360;
    else if(pEdict->v.idealpitch < -180)
        pEdict->v.idealpitch += 360;
}

float BotChangePitch(edict_t* pEdict, float speed)
{
    float ideal = pEdict->v.idealpitch;
    float current = -pEdict->v.v_angle.x;

    // turn from the current v_angle pitch to the idealpitch by selecting
    // the quickest way to turn to face that direction
    // find the difference in the current and ideal angle
    float diff = fabs(current - ideal);

    // check if the bot is already facing the idealpitch direction...
    if(diff <= 0.01)
        return diff; // return number of degrees turned

    // should keep turn speed the same under any fps...hopefully
    speed *= (gpGlobals->time - last_frame_time);
    speed *= 10;

    // check if difference is less than the max degrees per turn
    if(diff < speed)
        speed = diff; // just need to turn a little bit (less than max)

    // here we have four cases, both angle positive, one positive and
    // the other negative, one negative and the other positive, or
    // both negative.  handle each case separately...

    if((current >= 0) && (ideal >= 0)) // both positive
    {
        if(current > ideal)
            current -= speed;
        else
            current += speed;
    } else if((current >= 0) && (ideal < 0)) {
        const float current_180 = current - 180;

        if(current_180 > ideal)
            current += speed;
        else
            current -= speed;
    } else if((current < 0) && (ideal >= 0)) {
        const float current_180 = current + 180;
        if(current_180 > ideal)
            current += speed;
        else
            current -= speed;
    } else // (current < 0) && (ideal < 0)  both negative
    {
        if(current > ideal)
            current -= speed;
        else
            current += speed;
    }

    // check for wrap around of angle...
    if(current > 180)
        current -= 360;
    else if(current < -180)
        current += 360;

    pEdict->v.v_angle.x = current + pEdict->v.punchangle.x;
    pEdict->v.angles.x = pEdict->v.v_angle.x / 3;
    pEdict->v.v_angle.x = -pEdict->v.v_angle.x;
    pEdict->v.angles.z = 0;

    return speed; // return number of degrees turned
}

void BotFixIdealYaw(edict_t* pEdict)
{
    // check for wrap around of angle...
    if(pEdict->v.ideal_yaw > 180)
        pEdict->v.ideal_yaw -= 360;
    else if(pEdict->v.ideal_yaw < -180)
        pEdict->v.ideal_yaw += 360;
}

float BotChangeYaw(edict_t* pEdict, float speed)
{
    float ideal = pEdict->v.ideal_yaw;
    float current = pEdict->v.v_angle.y;

    // turn from the current v_angle yaw to the ideal_yaw by selecting
    // the quickest way to turn to face that direction
    // find the difference in the current and ideal angle
    float diff = fabs(current - ideal);

    // check if the bot is already facing the ideal_yaw direction...
    if(diff <= 0.01)
        return diff; // return number of degrees turned

    speed *= (gpGlobals->time - last_frame_time);
    speed *= 10;

    // check if difference is less than the max degrees per turn
    if(diff < speed)
        speed = diff; // just need to turn a little bit (less than max)

    // here we have four cases, both angle positive, one positive and
    // the other negative, one negative and the other positive, or
    // both negative.  handle each case separately...
    if((current >= 0) && (ideal >= 0)) // both positive
    {
        if(current > ideal)
            current -= speed;
        else
            current += speed;
    } else if((current >= 0) && (ideal < 0)) {
        const float current_180 = current - 180;
        if(current_180 > ideal)
            current += speed;
        else
            current -= speed;
    } else if((current < 0) && (ideal >= 0)) {
        const float current_180 = current + 180;
        if(current_180 > ideal)
            current += speed;
        else
            current -= speed;
    } else // (current < 0) && (ideal < 0)  both negative
    {
        if(current > ideal)
            current -= speed;
        else
            current += speed;
    }

    // check for wrap around of angle...
    if(current > 180)
        current -= 360;
    else if(current < -180)
        current += 360;

    pEdict->v.v_angle.y = current + pEdict->v.punchangle.y;
    pEdict->v.angles.y = pEdict->v.v_angle.y;
    pEdict->v.angles.z = 0;

    return speed; // return number of degrees turned
}

// This function is useful when you are trying to steer the bot towards some
// map location without using waypoints.
// For example, this function will try to make the bot jump over or duck under obstacles.
void BotNavigateWaypointless(bot_t* pBot)
{
    pBot->f_move_speed = pBot->f_max_speed;
    pBot->pEdict->v.button |= IN_FORWARD;

    // give the bot time to return to it's waypoint afterwards
    pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

    // navigating waypointless may send the bot too far from it's current
    // waypoint, so periodically try to keep it up to date
    if(pBot->f_periodicAlert1 < pBot->f_think_time) {
        if(!VectorsNearerThan(pBot->pEdict->v.origin, waypoints[pBot->current_wp].origin, 800.0) ||
            !BotCanSeeOrigin(pBot, waypoints[pBot->current_wp].origin))
            BotFindCurrentWaypoint(pBot);
    }

    BotFallenOffCheck(pBot);

    // if not obstructed by a player is the bot obstructed by anything else?
    if(BotContactThink(pBot) == NULL) {
        // buoyed by water, or on a ladder?
        if((pBot->pEdict->v.waterlevel > WL_FEET_IN_WATER && !(pBot->pEdict->v.flags & FL_ONGROUND)) ||
            pBot->pEdict->v.movetype == MOVETYPE_FLY) {
            // hopefully the bot wont get stuck
        } else // not on a ladder or in some water
        {
            const float botVelocity = pBot->pEdict->v.velocity.Length();

            // is the bot getting stuck?
            if(botVelocity < 50.0) {
                const int jumpResult = BotShouldJumpOver(pBot);
                if(jumpResult == 2) // can the bot jump onto something?
                {
                    pBot->pEdict->v.button |= IN_JUMP;
                } else {
                    const int duckResult = BotShouldDuckUnder(pBot);
                    if(duckResult == 2) // can the bot duck under something?
                    {
                        // duck down and move forward
                        pBot->f_duck_time = pBot->f_think_time + 0.3;
                    } else // can't duck or jump - try random movement tricks
                    {
                        // some maps(e.g. hunted) have undetectable obstacles
                        // jumping and/or ducking can overcome some of those
                        if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) <= 501) {
                            if(random_long(1, 1000) <= 501)
                                pBot->pEdict->v.button |= IN_JUMP;
                            else
                                pBot->f_duck_time = pBot->f_think_time + random_float(0.3, 1.2);
                        }

                        // randomly switch direction to try and get unstuck
                        if(pBot->f_periodicAlert3 < pBot->f_think_time && random_long(1, 1000) <= 501) {
                            if(pBot->side_direction == SIDE_DIRECTION_RIGHT)
                                pBot->side_direction = SIDE_DIRECTION_LEFT;
                            else
                                pBot->side_direction = SIDE_DIRECTION_RIGHT;
                        }

                        // make sure the bot isn't stuck strafing into a wall
                        if(BotCheckWallOnRight(pBot))
                            pBot->side_direction = SIDE_DIRECTION_LEFT;
                        else if(BotCheckWallOnLeft(pBot))
                            pBot->side_direction = SIDE_DIRECTION_RIGHT;

                        if(pBot->side_direction == SIDE_DIRECTION_RIGHT) {
                            pBot->f_side_speed = pBot->f_max_speed;
                            pBot->pEdict->v.button |= IN_MOVERIGHT; // so crates can be pushed
                        } else {
                            pBot->f_side_speed = -pBot->f_max_speed;
                            pBot->pEdict->v.button |= IN_MOVELEFT; // so crates can be pushed
                        }
                    }
                }
            }
        }
    }
}

// You should call this function when pBot->goto_wp is valid and you want
// the bot to get to it.
// Set navByStrafe to TRUE if you wish the bot to navigate by using axial
// movement speeds only(i.e. without having to look at the next waypoint).
bool BotNavigateWaypoints(bot_t* pBot, bool navByStrafe)
{
    if(num_waypoints < 1)
        return FALSE;

    // has the bot been getting stuck a little too often?
    if(pBot->routeFailureTally > 1) {
        BotEscapeWaypointTrap(pBot, pBot->goto_wp);
        pBot->routeFailureTally = 0;
        return FALSE;
    }

    pBot->f_move_speed = pBot->f_max_speed;
    pBot->f_side_speed = 0.0;

    // is it time to consider taking some kind of route shortcut?
    if(pBot->f_shortcutCheckTime < pBot->f_think_time ||
        (pBot->f_shortcutCheckTime - 60.0) > pBot->f_think_time) // sanity check
    {
        pBot->f_shortcutCheckTime = pBot->f_think_time + 1.0;

        if(BotFindTeleportShortCut(pBot) == FALSE && pBot->bot_skill < 4) {
            if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC || pBot->pEdict->v.playerclass == TFC_CLASS_SCOUT)
                BotCheckForConcJump(pBot);
            else if(pBot->pEdict->v.playerclass == TFC_CLASS_SOLDIER)
                BotCheckForRocketJump(pBot);
        }
    }

    // try to navigate by strafing if the bot has an enemy
    if(pBot->enemy.ptr != NULL)
        navByStrafe = TRUE;

    bool botIsSniping = FALSE;
    if(pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER && pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE &&
        (pBot->f_snipe_time >= pBot->f_think_time || pBot->pEdict->v.button & IN_ATTACK)) {
        botIsSniping = TRUE;
    }

    // this bit of code checks if the bot is getting stuck whilst navigating
    // or not.  it does so by making sure that the bot is making some kind
    // of progress towards it's waypoint
    if(!botIsSniping) // this code confuses sniping snipers
    {
        const float waypointDistance = (pBot->pEdict->v.origin - waypoints[pBot->current_wp].origin).Length();

        // if the bots waypoint has changed recently assume the bot isn't stuck
        if(pBot->current_wp != pBot->curr_wp_diff) {
            pBot->curr_wp_diff = pBot->current_wp;
            pBot->f_progressToWaypoint = waypointDistance + 0.1;
        }

        // is the bot the nearest to the waypoint it's been so far?
        // (add 1.0 because we want to see sizeable, significant progress)
        if((waypointDistance + 1.0) < pBot->f_progressToWaypoint) {
            pBot->f_progressToWaypoint = waypointDistance;

            // no problem - so reset this
            pBot->f_navProblemStartTime = 0.0;
        } else // uh-oh - looks liek troubble!
        {
            // remember when the poor bots troubles began(tell us about your mother)
            if(pBot->f_navProblemStartTime < 0.1)
                pBot->f_navProblemStartTime = pBot->f_think_time;
        }
    }

    BotContactThink(pBot);

    // if the bot has run into some kind of movement problem try to
    // handle it here
    if(!botIsSniping && pBot->f_navProblemStartTime > 0.1 && (pBot->f_navProblemStartTime + 0.5) < pBot->f_think_time) {
        // face the waypoint when navigation is getting hindered
        // and it's been going on for too long
        if(pBot->enemy.ptr == NULL || (pBot->f_navProblemStartTime + 4.0) < pBot->f_think_time)
            navByStrafe = FALSE;

        // buoyed by water, or on a ladder?
        if((pBot->pEdict->v.waterlevel > WL_FEET_IN_WATER && !(pBot->pEdict->v.flags & FL_ONGROUND)) ||
            pBot->pEdict->v.movetype == MOVETYPE_FLY) {
            if((pBot->f_navProblemStartTime + 2.0) < pBot->f_think_time) {
                job_struct* newJob = InitialiseNewJob(pBot, JOB_GET_UNSTUCK);
                if(newJob != NULL)
                    SubmitNewJob(pBot, JOB_GET_UNSTUCK, newJob);
            }
        } else // not on a ladder or in some water
        {
            // if the bot is airborne, duck(useful if jumping onto something)
            if(!(pBot->pEdict->v.flags & FL_ONGROUND)) {
                // in case the bot forgets it is duck-jumping
                if(pBot->pEdict->v.velocity.z > 0.0)
                    pBot->f_duck_time = pBot->f_think_time + 0.3;
            } else // not airborne - time to try and get around an obstruction
            {
                int jumpResult = BotShouldJumpOver(pBot);

                // can the bot jump over something?
                if(jumpResult == 2) // 2 == jumpable
                {
                    pBot->pEdict->v.button |= IN_JUMP;
                } else // can the bot duck under something?
                {
                    int duckResult = BotShouldDuckUnder(pBot);

                    if(duckResult == 2) // 2 == duckable
                    {
                        pBot->f_duck_time = pBot->f_think_time + 0.3;
                    }
                    // can't jump over, can't duck under either, but blocked by something
                    else if(jumpResult == 1 // 1 = blocked by something
                        || duckResult == 1) {
                        // try to get unstuck, but not too soon
                        if((pBot->f_navProblemStartTime + 2.0) < pBot->f_think_time) {
                            job_struct* newJob = InitialiseNewJob(pBot, JOB_GET_UNSTUCK);
                            if(newJob != NULL)
                                SubmitNewJob(pBot, JOB_GET_UNSTUCK, newJob);
                        } else // a little too soon for calling JOB_GET_UNSTUCK
                        {
                            // randomly switch direction to try and get unstuck
                            if(pBot->f_periodicAlert3 < pBot->f_think_time && random_long(1, 1000) <= 501) {
                                // jump too(it might help occasionally)
                                pBot->pEdict->v.button |= IN_JUMP;

                                if(pBot->side_direction == SIDE_DIRECTION_RIGHT)
                                    pBot->side_direction = SIDE_DIRECTION_LEFT;
                                else
                                    pBot->side_direction = SIDE_DIRECTION_RIGHT;
                            }

                            // make sure the bot isn't stuck strafing into a wall
                            if(BotCheckWallOnRight(pBot))
                                pBot->side_direction = SIDE_DIRECTION_LEFT;
                            else if(BotCheckWallOnLeft(pBot))
                                pBot->side_direction = SIDE_DIRECTION_RIGHT;

                            if(pBot->side_direction == SIDE_DIRECTION_RIGHT) {
                                pBot->f_side_speed = pBot->f_max_speed;
                                pBot->pEdict->v.button |= IN_MOVERIGHT; // so crates can be pushed
                            } else {
                                pBot->f_side_speed = -pBot->f_max_speed;
                                pBot->pEdict->v.button |= IN_MOVELEFT; // so crates can be pushed
                            }
                        }
                    } else // the bot doesn't appear to be blocked by anything it can see
                    {
                        // we don't know what's stalling the bot, but we can try a
                        // few simple tricks(if the bots been stalled for 2 seconds or more)
                        if((pBot->f_navProblemStartTime + 2.0) < pBot->f_think_time) {
                            // if the bot is directly below it's current waypoint and not on a ladder
                            // then assume the bot doesn't know it's fallen off and fix it
                            if(pBot->pEdict->v.flags & FL_ONGROUND && pBot->pEdict->v.waterlevel != WL_HEAD_IN_WATER &&
                                !(waypoints[pBot->current_wp].flags & W_FL_LIFT) &&
                                (pBot->pEdict->v.origin.z + 60.0) < waypoints[pBot->current_wp].origin.z &&
                                (pBot->pEdict->v.origin - waypoints[pBot->current_wp].origin).Length2D() < 15.0) {
                                //	UTIL_HostSay(pBot->pEdict, 0, "fixed running vertically!");
                                ////DebugMessageOfDoom!
                                BotFindCurrentWaypoint(pBot);
                                return FALSE; // to help avoid a repetitive failure
                            }

                            // slow down for a couple of seconds(see if that helps)
                            if(pBot->enemy.ptr == NULL && (pBot->f_navProblemStartTime + 4.0) > pBot->f_think_time)
                                pBot->f_move_speed = pBot->f_max_speed / 2.0;

                            // some maps(e.g. hunted) have undetectable obstacles
                            // jumping and/or ducking can overcome some of those
                            if(pBot->f_periodicAlert1 < pBot->f_think_time && random_long(1, 1000) <= 501) {
                                if(random_long(1, 1000) <= 501)
                                    pBot->pEdict->v.button |= IN_JUMP;
                                else
                                    pBot->f_duck_time = pBot->f_think_time + random_float(0.3, 1.2);
                            }
                        }
                    }
                }
            }
        }
    }

    // keep navigating the waypoints, unless a serious problem occurred
    if(!BotHeadTowardWaypoint(pBot, navByStrafe))
        return FALSE;

    // allow the bot to react according to the characteristics of it's current waypoint
    if(pBot->current_wp != -1) {
        // remember how far the bot is from it's current waypoint
        const float current_wp_distance = (waypoints[pBot->current_wp].origin - pBot->pEdict->v.origin).Length();

        if(navByStrafe == FALSE) {
            // slow the bot down as it approaches the final waypoint on it's route
            if(pBot->current_wp == pBot->goto_wp) {
                if(current_wp_distance < 70.0)
                    pBot->f_move_speed = pBot->f_max_speed / 4;
                else if(current_wp_distance < 130.0)
                    pBot->f_move_speed = pBot->f_max_speed / 2;
            }

            // slow down if the next waypoint is a walk waypoint...
            if(waypoints[pBot->current_wp].flags & W_FL_WALK && navByStrafe == FALSE)
                pBot->f_move_speed = pBot->f_max_speed / 3;
        }

        // if the bot is approaching a lift
        if(waypoints[pBot->current_wp].flags & W_FL_LIFT) {
            BotUseLift(pBot);
        }

        // check if the bot is using a ladder
        if(waypoints[pBot->current_wp].flags & W_FL_LADDER || pBot->pEdict->v.movetype == MOVETYPE_FLY) {
            BotHandleLadderTraffic(pBot); // try to avoid ladder traffic jams

            // if not stopping on the ladder hold the forwards key
            if(pBot->f_pause_time < pBot->f_think_time || pBot->current_wp != pBot->goto_wp) {
                pBot->pEdict->v.button |= IN_FORWARD;
            }
        }

        // check if the next waypoint is a jump waypoint...
        if(waypoints[pBot->current_wp].flags & W_FL_JUMP) {
            if(pBot->pEdict->v.flags & FL_ONGROUND && pBot->f_pause_time < pBot->f_think_time) {
                const Vector jumpPoint =
                    waypoints[pBot->current_wp].origin - (pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs);

                // pause briefly if the bot is not facing the jump waypoint enough
                if(BotInFieldOfView(pBot, jumpPoint) > 20)
                    pBot->f_pause_time = pBot->f_think_time + 0.2;
                else if(current_wp_distance < 100.0f) // otherwise jump
                    pBot->pEdict->v.button |= IN_JUMP;
            }

            if(pBot->pEdict->v.velocity.z > 0.0)
                pBot->f_duck_time = pBot->f_think_time + 0.25; // for extra clearance
        }

        // see if the bot is about to get stuck along a blocked route
        // e.g. a detpack tunnel
        if(pBot->current_wp != pBot->goto_wp && (waypoints[pBot->current_wp].flags & W_FL_PATHCHECK ||
                                                    waypoints[pBot->current_wp].flags & W_FL_TFC_DETPACK_CLEAR ||
                                                    waypoints[pBot->current_wp].flags & W_FL_TFC_DETPACK_SEAL)) {
            // remember if the bot is going to a goal waypoint or a route branching waypoint
            int nextWP;
            if(pBot->branch_waypoint == -1)
                nextWP = WaypointRouteFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team);
            else
                nextWP = WaypointRouteFromTo(pBot->current_wp, pBot->branch_waypoint, pBot->current_team);

            if(nextWP > -1 && BotPathCheck(pBot->current_wp, nextWP) == FALSE)
                BotChangeRoute(pBot);
        }
    }

    return TRUE; // all is well so far
}

// This function would probably be a good place to check to see how close
// to the current waypoint the bot is, and if the bot is close enough to
// the desired waypoint then call BotFindWaypoint to find the next one.
// Set navByStrafe to TRUE if you wish the bot to navigate by using axial
// movement speeds only(i.e. without having to look at the next waypoint).
bool BotHeadTowardWaypoint(bot_t* pBot, bool& r_navByStrafe)
{
    // check if the bot is taking too long to reach it's current waypoint
    if(pBot->f_current_wp_deadline < pBot->f_think_time) {
        // tidy up the bots current waypoint, in case it's behind a wall
        pBot->current_wp = WaypointFindNearest_S(
            pBot->pEdict->v.origin, pBot->pEdict, REACHABLE_RANGE, pBot->current_team, lostBotIgnoreFlags);
        pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

        ++pBot->routeFailureTally; // chalk up another route failure
        return FALSE;              // give up
    }

    BotFallenOffCheck(pBot);

    // if the bot has a destination waypoint and a current waypoint
    if(pBot->current_wp != -1 && pBot->goto_wp != -1) {
        // turn r_navByStrafe off if the bot is using a ladder unless
        // it has seen an enemy recently and isn't getting seriously stuck
        if(r_navByStrafe == TRUE &&
            (waypoints[pBot->current_wp].flags & W_FL_LADDER || pBot->pEdict->v.movetype == MOVETYPE_FLY)) {
            if(pBot->enemy.ptr == NULL)
                r_navByStrafe = FALSE;
            else if(pBot->f_navProblemStartTime > 0.1 && (pBot->f_navProblemStartTime + 4.0) < pBot->f_think_time)
                r_navByStrafe = FALSE;
        }

        // if the bot is not navigating by strafing or is approaching
        // a jump waypoint then face towards the current waypoint
        if(r_navByStrafe == FALSE || waypoints[pBot->current_wp].flags & W_FL_JUMP) {
            Vector v_direction = waypoints[pBot->current_wp].origin - pBot->pEdict->v.origin;
            Vector bot_angles = UTIL_VecToAngles(v_direction);

            pBot->idle_angle = bot_angles.y;

            // allow bots to approach non-goal waypoints imprecisely
            if(pBot->current_wp != pBot->goto_wp)
                pBot->idle_angle += pBot->f_waypoint_drift;

            pBot->pEdict->v.ideal_yaw = pBot->idle_angle;
            pBot->pEdict->v.idealpitch = bot_angles.x;
        } else // navByStrafe is active
        {
            // navigate towards the current waypoint using axial movement speeds only
            // to do this we need to know the angle of the bots current waypoint
            // from the bots view angle

            // get the angle towards the bots current waypoint
            Vector offset = waypoints[pBot->current_wp].origin - pBot->pEdict->v.origin;
            Vector vang = UTIL_VecToAngles(offset);
            vang.y -= pBot->pEdict->v.v_angle.y;
            vang.y = 360.0 - vang.y;

            // wrap Y if need be
            if(vang.y < 0.0)
                vang.y += 360.0;
            else if(vang.y > 360.0)
                vang.y -= 360.0;

            /*	// debug stuff
                    char msg[96];
                    sprintf(msg, "vang.y %f, v_angle %f",
                            vang.y, pBot->pEdict->v.v_angle.y);
                    UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!*/

            // is the waypoint ahead?
            if(vang.y > 275.0 || vang.y < 85.0) {
                // do nothing, leave the bots forward speed as it is
            }
            // if the waypoint is behind, go backwards
            else if(vang.y > 95.0 && vang.y < 265.0)
                pBot->f_move_speed = -pBot->f_move_speed;
            else
                pBot->f_move_speed = 0.0; // waypoint is not ahead or behind

            // if the waypoint is on the left, sidestep left
            if(vang.y > 185.0 && vang.y < 355.0) {
                pBot->side_direction = SIDE_DIRECTION_LEFT;
                pBot->f_side_speed = -(pBot->f_max_speed);
            }
            // if the waypoint is on the right, sidestep right
            else if(vang.y > 5.0 && vang.y < 175.0) {
                pBot->side_direction = SIDE_DIRECTION_RIGHT;
                pBot->f_side_speed = pBot->f_max_speed;
            } else
                pBot->f_side_speed = 0.0; // not on left or right, don't strafe sideways

            // now to handle vertical movement when swimming(or bobbing on the surface of water)
            if(pBot->pEdict->v.waterlevel == WL_HEAD_IN_WATER ||
                (pBot->pEdict->v.waterlevel == WL_WAIST_IN_WATER // on the surface
                    && !(pBot->pEdict->v.flags & FL_ONGROUND))) {
                // swim up if below the waypoint
                if(pBot->pEdict->v.origin.z < (waypoints[pBot->current_wp].origin.z - 5.0)) {
                    //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
                    //		pBot->pEdict->v.origin + Vector(0, 0, 100.0),
                    //		10, 2, 250, 250, 50, 200, 10);

                    pBot->f_vertical_speed = pBot->f_max_speed;
                }

                // swim down if above the waypoint
                else if(pBot->pEdict->v.origin.z > (waypoints[pBot->current_wp].origin.z + 5.0)) {
                    //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
                    //		pBot->pEdict->v.origin + Vector(0, 0, 100.0),
                    //		10, 2, 250, 250, 250, 200, 10);

                    pBot->f_vertical_speed = -pBot->f_max_speed;
                }
            }
        }
    }

    if(BotUpdateRoute(pBot))
        return TRUE;
    else
        return FALSE;
}

// BotUpdateRoute()
// Selects the next waypoint along the bot's route.
static bool BotUpdateRoute(bot_t* pBot)
{
    int new_current_wp; // what pBot->current_wp might be set to
    edict_t* pEdict = pBot->pEdict;

    // If the bot doesn't have a current waypoint, then lets
    // try sending the bot to the closest waypoint.
    if(pBot->current_wp < 0) {
        // UTIL_HostSay(pBot->pEdict, 0, "BotFindWaypoint, current_wp == -1"); //DebugMessageOfDoom!

        // find the nearest visible waypoint
        new_current_wp = WaypointFindNearest_S(
            pBot->pEdict->v.origin, pEdict, REACHABLE_RANGE, pBot->current_team, lostBotIgnoreFlags);

        if(new_current_wp != -1) {
            pBot->current_wp = new_current_wp;
            pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

            return TRUE;
        } else
            return FALSE; // couldn't find a current waypoint for the bot
    } else                // the bot already has a current waypoint
    {
        new_current_wp = pBot->current_wp;

        // if the bot has a new goal waypoint, dump/reset the current branching waypoint
        if(pBot->goto_wp != pBot->lastgoto_wp)
            pBot->branch_waypoint = -1;

        // if the bot took a branching route and has arrived at the branching waypoint
        // then continue onwards to the goal waypoint
        if(pBot->current_wp == pBot->branch_waypoint) {
            //	UTIL_HostSay(pBot->pEdict, 0, "resuming from sideroute"); //DebugMessageOfDoom!
            pBot->branch_waypoint = -1;
        }

        pBot->lastgoto_wp = pBot->goto_wp; // allows us to monitor for changed goal waypoints

        const float dist = (waypoints[new_current_wp].origin - pBot->pEdict->v.origin).Length();

        // try to find the next waypoint on the route to the bots goal
        int nextWP;
        if(pBot->branch_waypoint == -1)
            nextWP = WaypointRouteFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team);
        else // bots current goal is a branching waypoint
            nextWP = WaypointRouteFromTo(pBot->current_wp, pBot->branch_waypoint, pBot->current_team);

        // figure out how near to the bot's current waypoint the bot has to be
        // before it will move on the next waypoint
        float needed_distance = 50.0; // standard distance for most waypoint types
        bool heightCheck = TRUE;      // used only when climbing ladders
        if(pBot->pEdict->v.movetype == MOVETYPE_FLY) {
            // if the bot is on a ladder make sure it is just above the waypoint
            needed_distance = 20.0; // got to be near when on a ladder
            if(pBot->pEdict->v.origin.z < waypoints[new_current_wp].origin.z ||
                pBot->pEdict->v.origin.z > (waypoints[new_current_wp].origin.z + 10.0))
                heightCheck = FALSE;
        } else if(waypoints[new_current_wp].flags & W_FL_JUMP) {
            // gotta be similar height or higher than jump waypoint
            if(pBot->pEdict->v.origin.z < (waypoints[new_current_wp].origin.z - 15.0) ||
                pBot->pEdict->v.flags & FL_ONGROUND)
                heightCheck = FALSE;
            needed_distance = 80.0;
        } else if(waypoints[new_current_wp].flags & W_FL_LIFT)
            needed_distance = 25.0; // some lifts are small(e.g. rock2's lifts)
        else if(waypoints[new_current_wp].flags & W_FL_WALK)
            needed_distance = 20.0;

        bool waypointTouched = FALSE;

        // if the bot's near enough to it's current waypoint, and it hasn't
        // reached it's destination then pick the next waypoint on the route
        if(new_current_wp != pBot->goto_wp && dist < needed_distance && heightCheck == TRUE) {
            waypointTouched = TRUE;
        }
        // this check can solve waypoint circling problems
        else if(dist < 100.0 && pBot->f_navProblemStartTime > 0.1 &&
            (pBot->f_navProblemStartTime + 0.5) < pBot->f_think_time) {
            if(nextWP != -1 && nextWP != pBot->goto_wp) {
                const float pathDistance =
                    static_cast<float>(WaypointDistanceFromTo(new_current_wp, nextWP, pBot->current_team));
                const float distToNext = (waypoints[nextWP].origin - pBot->pEdict->v.origin).Length();

                // see if the bot is near enough to the next waypoint despite
                // not touching the current waypoint
                if(distToNext < pathDistance && (dist + distToNext) < (pathDistance + 50.0)) {
                    //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                    //		pBot->pEdict->v.origin + Vector(0, 0, 100.0),
                    //		10, 2, 250, 250, 250, 200, 10);

                    waypointTouched = TRUE;
                }
            }
        }

        if(waypointTouched) {
            if(nextWP != -1 && nextWP < num_waypoints) {
                new_current_wp = nextWP;
                pBot->routeFailureTally = 0; // all is well, reset this
            } else                           // bot has no route to it's goal
            {
                ++pBot->routeFailureTally; // chalk up another route failure
                new_current_wp = pBot->current_wp;
            }

            // set the amount of time to get to the new current waypoint
            if(new_current_wp != pBot->current_wp)
                pBot->f_current_wp_deadline = pBot->f_think_time + BOT_WP_DEADLINE;

            pBot->current_wp = new_current_wp;

            // lemming check! try to stop the bot running off a cliff
            /*	if(nextWP != -1
                            && nextWP != pBot->goto_wp
                            && static_cast<int>(pBot->pEdict->v.health) < 91
                            && !(waypoints[nextWP].flags & W_FL_LADDER)
                            && (waypoints[nextWP].origin.z + 300.0) < pBot->pEdict->v.origin.z)
                            {
                                    char msg[80];
                                    sprintf(msg, "<Avoiding drop of height: %f>",
                                            pBot->pEdict->v.origin.z - waypoints[nextWP].origin.z);
                                    UTIL_HostSay(pBot->pEdict, 0, msg);//DebugMessageOfDoom!
                                    BotChangeRoute(pBot);
                            }*/

            // now that the bot has picked the next waypoint on the route
            // to its goal, consider trying an alternate route
            BotFindSideRoute(pBot);
        }

        return TRUE;
    }
}

// This is a fairly simple function.
// If the bot is on a ladder going down, jump off if someone is on
// the ladder coming up.
static void BotHandleLadderTraffic(bot_t* pBot)
{
    // only handle bots that are on ladders heading downwards
    if(pBot->current_wp == -1 || pBot->pEdict->v.movetype != MOVETYPE_FLY ||
        waypoints[pBot->current_wp].origin.z > pBot->pEdict->v.origin.z)
        return;

    // trace a line straight down
    TraceResult tr;
    UTIL_TraceLine(pBot->pEdict->v.origin, pBot->pEdict->v.origin - Vector(0, 0, 120.0), dont_ignore_monsters,
        pBot->pEdict->v.pContainingEntity, &tr);

    // see if we detected a player below
    if(tr.flFraction < 1.0 && tr.pHit != NULL) {
        edict_t* pPlayer;

        // search the world for players...
        for(int i = 1; i <= gpGlobals->maxClients; i++) {
            pPlayer = INDEXENT(i);

            // skip invalid players
            if(pPlayer && !pPlayer->free && pPlayer == tr.pHit) {
                // jump off the ladder
                pBot->pEdict->v.button = 0; // in case IN_FORWARD is active
                pBot->pEdict->v.button |= IN_JUMP;
                pBot->f_pause_time = pBot->f_think_time + 1.2; // so the jump will work
                //	UTIL_HostSay(pBot->pEdict, 0, "ladder jam"); //DebugMessageOfDoom!
                break;
            }
        }
    }
}

// An all in one function for handling bot behaviour as they try to use lifts.
void BotUseLift(bot_t* pBot)
{
    if(pBot->current_wp == pBot->goto_wp)
        return; // shouldn't happen, but just in case

    // remember if the bot is going to a goal waypoint or a route branching waypoint
    int goalWP = pBot->goto_wp;
    if(pBot->branch_waypoint != -1)
        goalWP = pBot->branch_waypoint;

    const int nextWP = WaypointRouteFromTo(pBot->current_wp, goalWP, pBot->current_team);
    if(nextWP == -1)
        return; // shouldn't happen, but just in case

    const float distanceWP2D = (waypoints[pBot->current_wp].origin - pBot->pEdict->v.origin).Length2D();

    // see if the bot has arrived, a simple way of checking is to see if the next
    // waypoint on the route is not a lift waypoint
    if(!(waypoints[nextWP].flags & W_FL_LIFT)) {
        /*	// close enough to the final lift waypoint?
                if(waypoints[pBot->current_wp].origin.z < pBot->pEdict->v.origin.z
                        && waypoints[pBot->current_wp].origin.z > (pBot->pEdict->v.origin.z - 36.0))
                        return;*/

        // stop and wait when in line with the lift waypoint
        if(distanceWP2D < 25.0)
            pBot->f_pause_time = pBot->f_think_time + 0.2;
        return;
    } else // current waypoint is a lift waypoint, as is the next waypoint on the route
    {
        // is the lift waypoint roughly the same altitude as the bot?
        if(waypoints[pBot->current_wp].origin.z < (pBot->pEdict->v.origin.z + 36.0) &&
            waypoints[pBot->current_wp].origin.z > (pBot->pEdict->v.origin.z - 36.0)) {
            bool liftReady = FALSE;

            // traceline a short distance up from the waypoint to make sure
            // the lift isn't there
            TraceResult tr;
            UTIL_TraceLine(waypoints[pBot->current_wp].origin,
                waypoints[pBot->current_wp].origin + Vector(0.0, 0.0, 36.0), dont_ignore_monsters,
                pBot->pEdict->v.pContainingEntity, &tr);

            if(tr.pHit != NULL) {
                char className[10];
                strncpy(className, STRING(tr.pHit->v.classname), 10);
                className[9] = '\0';

                //	UTIL_HostSay(pBot->pEdict, 0, className); //DebugMessageOfDoom!

                if(strncmp(STRING(tr.pHit->v.classname), "func_door", 9) != 0 &&
                    strncmp(STRING(tr.pHit->v.classname), "func_plat", 9) != 0) {
                    // the space at the lift waypoint appears to be empty
                    // is the lift unoccupied by bot teammates?
                    if(BotTeammatesNearWaypoint(pBot, pBot->current_wp) < 1) {
                        // do a traceline straight down, to see if the lift is there
                        UTIL_TraceLine(waypoints[pBot->current_wp].origin,
                            waypoints[pBot->current_wp].origin - Vector(0.0, 0.0, 50.0), ignore_monsters,
                            pBot->pEdict->v.pContainingEntity, &tr);

                        if(tr.pHit != NULL) {
                            strncpy(className, STRING(tr.pHit->v.classname), 10);
                            className[9] = '\0';
                            if(strncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0 ||
                                strncmp(STRING(tr.pHit->v.classname), "func_plat", 9) == 0) {
                                //	WaypointDrawBeam(INDEXENT(1), tr.pHit->v.absmin,
                                //		VecBModelOrigin(tr.pHit), 10, 2, 50, 50, 250, 200, 10);

                                //	WaypointDrawBeam(INDEXENT(1), tr.pHit->v.absmin,
                                //		tr.pHit->v.absmin + tr.pHit->v.size, 10, 2, 50, 250, 50, 200, 10);

                                liftReady = TRUE;
                            }
                        }
                    }
                }
            }

            if(liftReady) {
                // walk into the lift
                pBot->f_move_speed = pBot->f_max_speed / 2;
                //	UTIL_HostSay(pBot->pEdict, 0, "approaching lift"); //DebugMessageOfDoom!
            } else {
                //	UTIL_HostSay(pBot->pEdict, 0, "awaiting lift's return"); //DebugMessageOfDoom!

                if(distanceWP2D < 300.0f)
                    pBot->f_move_speed = -(pBot->f_max_speed / 3);
                else
                    pBot->f_pause_time = pBot->f_think_time + 0.2;
            }
        } else // lift waypoint is too far above or below the bot to walk into
        {
            // do a traceline straight down, to see if the bot is on a lift
            TraceResult tr;
            UTIL_TraceLine(pBot->pEdict->v.origin, pBot->pEdict->v.origin - Vector(0.0, 0.0, 50.0), ignore_monsters,
                pBot->pEdict->v.pContainingEntity, &tr);

            if(tr.pHit != NULL) {
                char className[10];
                strncpy(className, STRING(tr.pHit->v.classname), 10);
                className[9] = '\0';
                if(strncmp(STRING(tr.pHit->v.classname), "func_door", 9) == 0 ||
                    strncmp(STRING(tr.pHit->v.classname), "func_plat", 9) == 0) {
                    pBot->current_wp = nextWP;
                }
                // bot is not on a lift, danger Will Robinson!
                //	else UTIL_HostSay(pBot->pEdict, 0, className); //DebugMessageOfDoom!
            }
        }
    }
}

// This function returns 1 if the bot has detected an obstacle at ankle height.
// 2 if that obstacle can be jumped over.  0 if no such obstacles were found.
static int BotShouldJumpOver(bot_t* pBot)
{
    // bots have a standing height of 72 units
    // and their origin is 37 units above the bottom of their boots
    // when crouching bots are 36 units tall and their origin is 19 units high

    bool obstacleFound = FALSE;

    // make vectors based upon the bots view yaw angle only
    UTIL_MakeVectors(Vector(0.0, pBot->pEdict->v.v_angle.y, 0.0));

    TraceResult tr;
    const Vector botBottom = Vector(pBot->pEdict->v.origin.x, pBot->pEdict->v.origin.y, pBot->pEdict->v.absmin.z) +
        (gpGlobals->v_forward * 4.0); // to the front of the bot slightly

    // set how far apart the left and right scans will be,
    // decide randomly so that we increase the bots chances of detecting
    // a jumpable obstacle
    const float aperture = 5.0 * static_cast<float>(random_long(1, 3));

    Vector v_source;

    // whether or not to trace down from the bots origin or up from the bots ankles
    // random so as to maximise chance of detecting obstacles
    bool searchDownwards = TRUE;
    if(random_long(1, 1000) > 500)
        searchDownwards = FALSE; // search upwards instead

    // here we can check from the left leg of the bot, first to see if there's
    // an obstacle there, then to see if it can be jumped over
    {
        // trace a short line forward and left at the lowest height
        // of something that needs jumping
        if(searchDownwards)
            v_source = botBottom + Vector(0, 0, 45.0); // waist downwards
        else
            v_source = botBottom + Vector(0, 0, 15.0); // ankles upwards

        Vector v_dest = botBottom + Vector(0, 0, 17.0) + gpGlobals->v_forward * 24.0; // forwards
        v_dest = v_dest + (gpGlobals->v_right * -aperture);                           // left a bit
        UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

        // obstructed by something?
        if(tr.flFraction < 1.0) {
            // show where the traceline hit
            //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 250, 250, 250, 200, 10);

            // get the distance of the obstacle from the bots lower leg
            const float shinObstacleDistance = (v_source - tr.vecEndPos).Length2D();

            // trace a short line forward and left at maximum jump/mantling height
            // also take into account whether or not the bot is crouching
            if(pBot->pEdict->v.button & IN_DUCK)
                v_source = botBottom + Vector(0, 0, 37.0);
            else
                v_source = botBottom + Vector(0, 0, 49.9);
            v_dest = v_source + gpGlobals->v_forward * 24.0;    // forwards a bit
            v_dest = v_dest + (gpGlobals->v_right * -aperture); // left a bit
            UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

            // show where the traceline goes
            //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 250, 250, 250, 200, 10);

            // if there is line of sight report that the bot can try jumping here
            if(tr.flFraction >= 1.0)
                return 2;
            // did the traceline go further then it did when traced from lower down the body?
            // if so then there appears to be some kind of ledge here
            else if((v_source - tr.vecEndPos).Length2D() > (shinObstacleDistance + 1.0))
                return 2;
            else
                obstacleFound = TRUE;
        }
    }

    // checking the left side didn't find a jumpable object
    // so we can check from the right leg of the bot, first to see if there's
    // an obstacle there, then to see if it can be jumped over
    {
        // trace a short line forward and right at the lowest height
        // of something that needs jumping
        if(searchDownwards)
            v_source = botBottom + Vector(0, 0, 45.0); // waist downwards
        else
            v_source = botBottom + Vector(0, 0, 15.0); // ankles upwards

        Vector v_dest = botBottom + Vector(0, 0, 17.0) + gpGlobals->v_forward * 24.0; // forwards
        v_dest = v_dest + (gpGlobals->v_right * aperture);                            // right a bit
        UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

        // obstructed by something?
        if(tr.flFraction < 1.0) {
            // show where the traceline hit
            //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 250, 250, 250, 200, 10);

            // get the distance of the obstacle from the bots lower leg
            const float shinObstacleDistance = (v_source - tr.vecEndPos).Length2D();

            // trace a short line forward and right at maximum jump/mantling height
            // also take into account whether or not the bot is crouching
            if(pBot->pEdict->v.button & IN_DUCK)
                v_source = botBottom + Vector(0, 0, 37.0);
            else
                v_source = botBottom + Vector(0, 0, 49.9);
            v_dest = v_source + gpGlobals->v_forward * 24.0;   // forwards a bit
            v_dest = v_dest + (gpGlobals->v_right * aperture); // right a bit
            UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

            // show where the traceline goes
            //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 250, 250, 250, 200, 10);

            // if there is line of sight report that the bot can try jumping here
            if(tr.flFraction >= 1.0)
                return 2;
            // did the traceline go further then it did when traced from lower down the body?
            // if so then there appears to be some kind of ledge here
            else if((v_source - tr.vecEndPos).Length2D() > (shinObstacleDistance + 1.0))
                return 2;
            else
                obstacleFound = TRUE;
        }
    }

    if(obstacleFound)
        return 1;
    else
        return 0; // all clear!
}

// This function returns 1 if the bot has detected an obstacle at head height.
// 2 if that obstacle can be ducked under.  0 if no such obstacles were found.
static int BotShouldDuckUnder(bot_t* pBot)
{
    // doh!
    if(pBot->pEdict->v.button & IN_DUCK)
        return 2;

    bool obstacleFound = FALSE;

    // make vectors based upon the bots view yaw angle only
    UTIL_MakeVectors(Vector(0.0, pBot->pEdict->v.v_angle.y, 0.0));

    const Vector botBottom = Vector(pBot->pEdict->v.origin.x, pBot->pEdict->v.origin.y, pBot->pEdict->v.absmin.z) +
        (gpGlobals->v_forward * 4.0); // to the front of the bot slightly

    TraceResult tr;

    // here we can check from the left side of the bot forwards, first to
    // see if there's an obstacle there, then to see if it can be ducked under
    {
        // trace a short line forward and left at the height of the bot's head
        // to see if there's something that needs ducking
        Vector v_source = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
        Vector v_dest = v_source + gpGlobals->v_forward * 24.0; // forwards a bit
        v_dest = v_dest + (gpGlobals->v_right * -10.0);         // left a bit
        UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

        // show where the traceline goes
        //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 50, 50, 250, 200, 10);

        // obstructed by something?
        if(tr.flFraction < 1.0) {
            // get the distance of the obstacle from the bots head
            const float headObstacleDistance = (v_source - tr.vecEndPos).Length();

            // trace a short line forward and right just above ducking height
            v_source = botBottom + Vector(0, 0, 37.0);
            v_dest = v_source + gpGlobals->v_forward * 24.0; // forwards a bit
            v_dest = v_dest + (gpGlobals->v_right * -10.0);  // left a bit
            UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

            // show where the traceline goes
            //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 50, 50, 250, 200, 10);

            // if there is line of sight report that the bot can try ducking here
            if(tr.flFraction >= 1.0)
                return 2;
            // did the traceline go further then it did when traced from the head?
            // if so then ducking may help
            else if((v_source - tr.vecEndPos).Length() > (headObstacleDistance + 1.0))
                return 2;
            else
                obstacleFound = TRUE;
        }
    }

    // checking the left side didn't find a duckable object
    // so we can check from the right side of the bot forwards, first to
    // see if there's an obstacle there, then to see if it can be ducked under
    {
        // trace a short line forward and right at the height of the bot's head
        // to see if there's something that needs ducking
        Vector v_source = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
        Vector v_dest = v_source + gpGlobals->v_forward * 24.0; // forwards a bit
        v_dest = v_dest + (gpGlobals->v_right * 10.0);          // right a bit
        UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

        // show where the traceline goes
        //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 50, 50, 250, 200, 10);

        // obstructed by something?
        if(tr.flFraction < 1.0) {
            // get the distance of the obstacle from the bots head
            const float headObstacleDistance = (v_source - tr.vecEndPos).Length();

            // trace a short line forward and right just above ducking height
            v_source = botBottom + Vector(0, 0, 37.0);
            v_dest = v_source + gpGlobals->v_forward * 24.0; // forwards a bit
            v_dest = v_dest + (gpGlobals->v_right * 10.0);   // right a bit
            UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

            // show where the traceline goes
            //	WaypointDrawBeam(INDEXENT(1), v_source, v_dest, 10, 2, 50, 50, 250, 200, 10);

            // if there is line of sight report that the bot can try ducking here
            if(tr.flFraction >= 1.0)
                return 2;
            // did the traceline go further then it did when traced from the head?
            // if so then ducking may help
            else if((v_source - tr.vecEndPos).Length() > (headObstacleDistance + 1.0))
                return 2;
            else
                obstacleFound = TRUE;
        }
    }

    if(obstacleFound)
        return 1;
    else
        return 0; // all clear!
}

// This function attempts to detect whether or not the bot has fallen off
// some kind of ledge.  It returns TRUE if the bot appears to have
// fallen off, and will also attempt to correct the bots current waypoint.
static bool BotFallenOffCheck(bot_t* const pBot)
{
    if(pBot->current_wp == -1)
        return FALSE;

    // fix going to wrong waypoint if falling, could help with rocket jumping :D
    // 87.0 = bot's standing origin(37) plus maximum jump height(48 - 50)
    if(pBot->pEdict->v.velocity.z < -200.0 && pBot->pEdict->v.waterlevel != WL_HEAD_IN_WATER &&
        !(waypoints[pBot->current_wp].flags & W_FL_LADDER) &&
        waypoints[pBot->current_wp].origin.z > pBot->pEdict->v.absmin.z + 87.0) {
        int i;
        PATH* p;
        float distanceToCurr;
        bool waypointIsConnected;

        // waypoints with an origin higher than this should be considered unreachable
        const float heightThreshold = waypoints[pBot->current_wp].origin.z - 10.0;

        // look for a waypoint that is connected directly to the bots
        // current waypoint and that is reachable
        // if none are found the bot must have fallen off a ledge
        for(int index = 0; index < num_waypoints; index++) {
            // skip deleted waypoints
            if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
                continue;

            // skip this waypoint if it's team specific and teams don't match
            if(waypoints[index].flags & W_FL_TEAM_SPECIFIC &&
                (waypoints[index].flags & W_FL_TEAM) != pBot->current_team)
                continue;

            // skip the current waypoint and any that are above the bot
            if(index == pBot->current_wp || index == pBot->goto_wp || waypoints[index].origin.z > heightThreshold)
                continue;

            // does this waypoint have a direct path to the bots current waypoint?
            p = paths[index];
            waypointIsConnected = FALSE;
            while(p != NULL && !waypointIsConnected) {
                for(i = 0; i < MAX_PATH_INDEX; i++) {
                    if(p->index[i] == pBot->current_wp) {
                        waypointIsConnected = TRUE;
                        break;
                    }
                }

                p = p->next; // go to next node in linked list
            }

            // if this waypoint is nearer to the bot then the current waypoint is
            // and visible to the bot we shall assume the bot can reach it first
            if(waypointIsConnected) {
                //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                //		waypoints[index].origin, 10, 2, 250, 250, 250, 200, 10);

                distanceToCurr = (pBot->pEdict->v.origin - waypoints[pBot->current_wp].origin).Length();

                if(VectorsNearerThan(pBot->pEdict->v.origin, waypoints[index].origin, distanceToCurr) &&
                    BotCanSeeOrigin(pBot, waypoints[index].origin))
                    return FALSE;
            }
        }
    } else
        return FALSE;

    const int nextWP = WaypointRouteFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team);

    // set current waypoint to next waypoint if next waypoint is lower
    // this can help the president on hunted when he falls from spawn
    if(nextWP != -1 && nextWP != pBot->current_wp && waypoints[nextWP].origin.z < pBot->pEdict->v.origin.z) {
        pBot->current_wp = nextWP;
    } else {
        BotFindCurrentWaypoint(pBot);
    }

    //	UTIL_HostSay(pBot->pEdict, 0, "I fell off"); //DebugMessageOfDoom
    return TRUE;
}

// This function traces a line several units out to the bots left.
// It returns true if an obstacle was encountered, false otherwise.
bool BotCheckWallOnLeft(bot_t* pBot)
{
    UTIL_MakeVectors(pBot->pEdict->v.v_angle);

    // do a trace 40 units to the left
    Vector v_left = pBot->pEdict->v.origin + (gpGlobals->v_right * -40);

    // lower v_left to height of a low wall
    v_left.z = pBot->pEdict->v.absmin.z + 17.0;

    TraceResult tr;
    UTIL_TraceLine(pBot->pEdict->v.origin, v_left, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

    // check if the trace hit something...
    if(tr.flFraction < 1.0) {
        //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
        //		v_left, 10, 2, 250, 50, 50, 200, 10);

        return TRUE;
    }

    return FALSE;
}

// This function traces a line several units out to the bots right.
// It returns true if an obstacle was encountered, false otherwise.
bool BotCheckWallOnRight(bot_t* pBot)
{
    UTIL_MakeVectors(pBot->pEdict->v.v_angle);

    // do a trace 40 units to the right
    Vector v_right = pBot->pEdict->v.origin + (gpGlobals->v_right * 40.0);

    // lower v_right to height of a low wall
    v_right.z = pBot->pEdict->v.absmin.z + 17.0;

    TraceResult tr;
    UTIL_TraceLine(pBot->pEdict->v.origin, v_right, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

    // check if the trace hit something...
    if(tr.flFraction < 1.0) {
        //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
        //		v_right, 10, 2, 250, 50, 50, 200, 10);

        return TRUE;
    }

    return FALSE;
}

// This function returns the index of a waypoint suitable for building a
// dispenser at, or -1 on failure.
int BotGetDispenserBuildWaypoint(bot_t* pBot)
{
    // if the bot has a sentry build the dispenser near it
    if(pBot->has_sentry == TRUE && !FNullEnt(pBot->sentry_edict)) {
        return WaypointFindRandomGoal_R(pBot->sentry_edict->v.origin, FALSE, 800.0, -1, 0);
    } else // build near a random sentry waypoint
    {
        if(pBot->current_wp < 0)
            return -1;

        int sentryWP = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_SENTRY);

        if(sentryWP == -1)
            return -1;

        return WaypointFindRandomGoal_R(waypoints[sentryWP].origin, FALSE, 800.0, -1, 0);
    }
}

// This function returns the index of a waypoint suitable for building a
// Teleporter at, or -1 on failure.
int BotGetTeleporterBuildWaypoint(bot_t* pBot, const bool buildEntrance)
{
    WPT_INT32 neededFlags;
    int otherEndWP = -1; // remembers the waypoint for the other built teleport
    int connectedDistance;

    int indices[10];
    int count = 0;

    if(buildEntrance) {
        neededFlags = W_FL_TFC_TELEPORTER_ENTRANCE;
        if(!FNullEnt(pBot->tpExit) && pBot->tpExitWP > -1)
            otherEndWP = pBot->tpExitWP;
    } else {
        neededFlags = W_FL_TFC_TELEPORTER_EXIT;
        if(!FNullEnt(pBot->tpEntrance) && pBot->tpEntranceWP > -1)
            otherEndWP = pBot->tpEntranceWP;
    }

    // start the search
    for(int index = 0; index < num_waypoints; index++) {
        // skip any deleted or aiming waypoints
        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
            continue;

        // skip irrelevant waypoints
        if(!(waypoints[index].flags & neededFlags))
            continue;

        // skip this waypoint if it's unavailable or unreachable
        if(!WaypointAvailable(index, pBot->current_team) ||
            WaypointRouteFromTo(pBot->current_wp, index, pBot->current_team) == -1)
            continue;

        // if the other teleporter has already been built
        // make sure this teleporter isn't too near to it
        if(otherEndWP != -1) {
            connectedDistance = WaypointDistanceFromTo(index, otherEndWP, pBot->current_team);
            if(connectedDistance > -1 && connectedDistance < 1400)
                continue;

            // check in both directions in case route is one way only
            connectedDistance = WaypointDistanceFromTo(otherEndWP, index, pBot->current_team);
            if(connectedDistance > -1 && connectedDistance < 1400)
                continue;
        }

        // found one
        indices[count] = index;
        ++count;

        if(count >= 10)
            break; // we have filled the list
    }

    if(count > 0)
        return indices[random_long(0, count - 1)];
    else
        return -1;
}

// This function allows bots to pick a new goal waypoint that will lead
// them along a branching route from their current route.
void BotFindSideRoute(bot_t* pBot)
{
    // don't allow branching if the bot is already branching,
    // because the bot will take longer and longer to arrive
    if(pBot->branch_waypoint != -1)
        return;

    if(pBot->current_wp == -1 || pBot->goto_wp == -1)
        return;

    // 50/50 chance of not branching at all
    if(random_long(1, 1000) > 500)
        return;

    // bit field of waypoint types to ignore
    static const WPT_INT32 ignoreFlags =
        0 + (W_FL_DELETED | W_FL_AIMING | W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY | W_FL_SNIPER |
                W_FL_TFC_DETPACK_CLEAR | W_FL_TFC_DETPACK_SEAL | W_FL_TFC_TELEPORTER_ENTRANCE |
                W_FL_TFC_TELEPORTER_EXIT | W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_TFC_JUMP);

    // find out if the bot is at a junction waypoint by counting
    // the number of paths connected from it
    PATH* p = paths[pBot->current_wp];
    int i;
    int paths_total = 0; // number of paths from the bots current waypoint
    while(p != NULL && paths_total < 3) {
        for(i = 0; i < MAX_PATH_INDEX && paths_total < 3; i++) {
            // count the path node if it's available to the bots team
            if(p->index[i] != -1 && !(waypoints[p->index[i]].flags & ignoreFlags) &&
                (!(waypoints[p->index[i]].flags & W_FL_TEAM_SPECIFIC) ||
                    (waypoints[p->index[i]].flags & W_FL_TEAM) == pBot->current_team))
                paths_total++;
        }

        p = p->next; // go to next node in linked list
    }

    // don't branch unless at a junction waypoint
    if(paths_total < 3)
        return;

    // remember the next waypoint on the bots route to it's current goal
    const int nextPredictedWP = WaypointRouteFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team);
    if(nextPredictedWP == -1)
        return;

    // remember how far the bot is from it's goal
    const int currentGoalDistance = WaypointDistanceFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team);
    if(currentGoalDistance < 400)
        return;

    int newPredictedWP;
    int newGoalDistance;
    int excessDistance; // new distance total - current distance total

    // has the bot changed it's mind on how far it's willing to branch?
    if(pBot->f_side_route_time < pBot->f_think_time) {
        pBot->f_side_route_time = pBot->f_think_time + random_float(15.0, 60.0);

        int randomatic = random_long(1, 1000);
        if(randomatic < 401)
            pBot->sideRouteTolerance = 400; // very short route changes
        else if(randomatic < 701)
            pBot->sideRouteTolerance = 2400; // mid-length route changes
        else
            pBot->sideRouteTolerance = 8000; // map covering distances
    }

    // start the search(from a randomly selected waypoint)
    i = static_cast<int>(random_long(0, num_waypoints - 1));
    for(int index = 0; index < num_waypoints; index++, i++) {
        // wrap the search if the number of waypoints has been exceeded
        if(i >= num_waypoints)
            i = 0;

        // skip waypoints we don't want to consider
        if(waypoints[i].flags & ignoreFlags)
            continue;

        // skip this waypoint if it's unavailable
        if(!WaypointAvailable(i, pBot->current_team))
            continue;

        // skip the bots current waypoint
        if(i == pBot->current_wp)
            continue;

        newPredictedWP = WaypointRouteFromTo(pBot->current_wp, i, pBot->current_team);

        // skip waypoints that do not branch off from the bots current route
        if(newPredictedWP == nextPredictedWP || newPredictedWP == -1)
            continue;

        newGoalDistance = WaypointDistanceFromTo(i, pBot->goto_wp, pBot->current_team);

        // skip waypoints that are not nearer to the goal than the bot already is
        if(newGoalDistance > currentGoalDistance || newGoalDistance == -1)
            continue;

        excessDistance =
            newGoalDistance + WaypointDistanceFromTo(pBot->current_wp, i, pBot->current_team) - currentGoalDistance;

        // find out if the extra travel is more than the bot is currently
        // willing to accept
        if(excessDistance > pBot->sideRouteTolerance)
            continue;

        // ignore shorter routes as they undo a previous branch selection
        // (because branched routes are never shorter than normal routes)
        if(excessDistance < 0)
            continue;

        // abort if the first few waypoints on this route contains any
        // unavailable waypoints or unblown detpack waypoints
        int nextWP = newPredictedWP;
        for(int j = 0; j < 18 && nextWP != i; j++) {
            if(!WaypointAvailable(nextWP, pBot->current_team))
                return;

            if((waypoints[nextWP].flags & W_FL_PATHCHECK) || (waypoints[nextWP].flags & W_FL_TFC_DETPACK_CLEAR) ||
                (waypoints[nextWP].flags & W_FL_TFC_DETPACK_SEAL)) {
                int endWP = WaypointRouteFromTo(nextWP, i, pBot->current_team);
                if(BotPathCheck(nextWP, endWP) == FALSE) {
                    //	char msg[96];
                    //	sprintf(msg, "path %d - %d blocked", nextWP, endWP);
                    //	UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!
                    return;
                } else {
                    //	char msg[96];
                    //	sprintf(msg, "path %d - %d clear", nextWP, endWP);
                    //	UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!
                }
            }

            nextWP = WaypointRouteFromTo(nextWP, i, pBot->current_team);
        }

        // found a suitable waypoint
        pBot->branch_waypoint = i;

        //	char msg[96]; //DebugMessageOfDoom!
        //	sprintf(msg, "found a side route, tolerance: %d, re-think in %f seconds",
        //		pBot->sideRouteTolerance, pBot->f_side_route_time - pBot->f_think_time);
        //	UTIL_HostSay(pBot->pEdict, 0, msg);

        return;
    }
}

// Give this function two waypoints and it'll see if there is an obstruction
// between them.  It returns TRUE if the path is clear.
bool BotPathCheck(const int sourceWP, const int destWP)
{
    TraceResult tr;

    // trace a line from waypoint to waypoint
    UTIL_TraceLine(waypoints[sourceWP].origin, waypoints[destWP].origin, ignore_monsters, NULL, &tr);

    // if line of sight is not blocked
    if(tr.flFraction >= 1.0)
        return TRUE;

    return FALSE;
}

// This function is like BotFindSideRoute() but was designed to help bots
// free themselves if they get lost somewhere(e.g. in a detpack tunnel).
// Basically, this function looks for a waypoint that:
// 1.) is on a different route from the bots current route
// 2.) will not route back though the bots current waypoint to the goal
// It returns TRUE if it sent the bot along a new route.
bool BotChangeRoute(bot_t* pBot)
{
    if(pBot->current_wp == -1 || pBot->goto_wp == -1)
        return FALSE;

    // remember if the bot is going to a goal waypoint or a route branching waypoint
    int goalWP = pBot->goto_wp;
    if(pBot->branch_waypoint != -1)
        goalWP = pBot->branch_waypoint;

    // remember the next waypoint on the bots route to it's current goal
    const int nextPredictedWP = WaypointRouteFromTo(pBot->current_wp, goalWP, pBot->current_team);
    if(nextPredictedWP == -1)
        return FALSE;

    // the bots current distance from it's goal
    int currentGoalDistance = WaypointDistanceFromTo(pBot->current_wp, goalWP, pBot->current_team);
    if(currentGoalDistance < 200)
        return FALSE;

    int newPredictedWP;
    int newGoalDistance;
    int interimDist;
    int maxRouteDistance;

    // used for remembering the best waypoint found
    int newBranchWP = -1;

    // bit field of waypoint types to ignore
    static const WPT_INT32 ignoreFlags =
        0 + (W_FL_DELETED | W_FL_AIMING | W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY | W_FL_SNIPER |
                W_FL_TFC_TELEPORTER_ENTRANCE | W_FL_TFC_TELEPORTER_EXIT | W_FL_TFC_JUMP | W_FL_LIFT);

    // pick a random waypoint to start searching from
    int index = static_cast<int>(RANDOM_LONG(0, num_waypoints - 1));

    // start the search
    for(int i = 0; i < num_waypoints; i++) {
        // wrap the search if it exceeds the number of available waypoints
        if(index >= num_waypoints)
            index = 0;

        // skip waypoints with flags we don't want to consider
        // or unavailable waypoints
        if(waypoints[index].flags & ignoreFlags || !WaypointAvailable(index, pBot->current_team))
            continue;

        newPredictedWP = WaypointRouteFromTo(pBot->current_wp, index, pBot->current_team);

        // skip waypoints that do not branch off from the bots current route
        // or that don't have a route to the goal
        if(newPredictedWP == nextPredictedWP || newPredictedWP == -1)
            continue;

        // distance from this new waypoint to the bots current waypoint
        interimDist = WaypointDistanceFromTo(index, pBot->current_wp, pBot->current_team);

        // the best waypoints have no route back through the bot's current waypoint
        if(interimDist == -1) {
            newBranchWP = index;
            break;
        } else {
            // distance from this waypoint to the bots goal
            newGoalDistance = WaypointDistanceFromTo(index, goalWP, pBot->current_team);

            // distance from waypoint to bot + bots current distance from goal
            maxRouteDistance = interimDist + currentGoalDistance;

            // look for a waypoint that will take the bot nearer to it's goal
            // without coming back through the bots current waypoint
            if(newGoalDistance < maxRouteDistance) {
                newBranchWP = index;
                break;
            }
        }
    }

    // found a suitable waypoint?
    if(newBranchWP != -1) {
        /*	char msg[80];
                sprintf(msg, "Got lost at %d going to %d, found another route via: %d",
                        pBot->current_wp, goalWP, newBranchWP);
                UTIL_HostSay(pBot->pEdict, 0, msg); //DebugMessageOfDoom!*/

        pBot->branch_waypoint = newBranchWP;

        return TRUE;
    }

    return FALSE;
}

// This function can be used to make the bot pick a different goal waypoint
// to the specified one, and that has the specified waypoint flags.
// i.e. if a bot has gotten stuck going to one flag waypoint then this can
// be used to send the bot to a different flag waypoint instead.
bool BotSetAlternativeGoalWaypoint(bot_t* const pBot, int& r_goalWP, const WPT_INT32 flags)
{
    if(r_goalWP < 0)
        return FALSE; // sanity check

    // make sure the bot has a sane current waypoint(e.g. not behind a wall)
    BotFindCurrentWaypoint(pBot);

    const int nextWP = WaypointRouteFromTo(pBot->current_wp, r_goalWP, pBot->current_team);
    if(nextWP == -1)
        return FALSE; // sanity check

    enum { MAX_INDICES = 5 };
    int indices[MAX_INDICES];
    int count = 0;
    int routeDistance;
    int routeNextWP;

    // find suitable waypoints with matching flags...
    for(int index = 0; index < num_waypoints; index++) {
        if(!(waypoints[index].flags & flags))
            continue; // skip this waypoint if the flags don't match

        // skip any deleted or aiming waypoints
        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
            continue;

        // need to skip wpt if not available to the bots team
        // also - make sure a route exists to this waypoint
        if(!WaypointAvailable(index, pBot->current_team) ||
            WaypointRouteFromTo(pBot->current_wp, index, pBot->current_team) == -1)
            continue;

        // ignore if this new waypoint goes on the same route as the old one
        routeNextWP = WaypointRouteFromTo(pBot->current_wp, index, pBot->current_team);
        if(routeNextWP == nextWP)
            continue;

        // check the distance from this waypoint to the current goal waypoint
        // to see if they are too near to each other
        routeDistance = WaypointDistanceFromTo(index, r_goalWP, pBot->current_team);
        if(routeDistance != -1 && routeDistance < 800)
            continue;

        // now check in the other direction, in case there is only a
        // one way path between the two waypoints
        routeDistance = WaypointDistanceFromTo(r_goalWP, index, pBot->current_team);
        if(routeDistance != -1 && routeDistance < 800)
            continue;

        indices[count] = index;
        ++count;

        if(count >= MAX_INDICES)
            break; // list is full
    }

    // found one?
    if(count > 0) {
        //	UTIL_HostSay(pBot->pEdict, 0, "alternative goal waypoint found"); //DebugMessageOfDoom!
        r_goalWP = indices[random_long(0, count - 1)];
        return TRUE;
    }

    return FALSE;
}

// Some waypoint files are not properly tested and contain waypoint traps.
// e.g. a map area has a waypoint route leading in, but no route leading out.
// Or the nearest current waypoint is completely unreachable.
// This function is designed to find a way out for a trapped bot by giving
// it a new current waypoint that should help.  Returns FALSE upon failure.
static bool BotEscapeWaypointTrap(bot_t* const pBot, const int goalWP)
{
    if(goalWP < 0)
        return FALSE; // sanity check

    //	UTIL_HostSay(pBot->pEdict, 0, "Escaping a waypoint trap"); //DebugMessageOfDoom!

    const int currentRoutDist = WaypointDistanceFromTo(pBot->current_wp, goalWP, pBot->current_team);

    int routeDistance;

    // find all the waypoints with the matching flags...
    for(int index = 0; index < num_waypoints; index++) {
        // skip any deleted or aiming waypoints
        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
            continue;

        // always pick a new current waypoint, the old one may be unreachable
        if(index == pBot->current_wp)
            continue;

        // make sure a route exists from this waypoint to the goal waypoint,
        // and that the bot can see it
        if(WaypointAvailable(index, pBot->current_team)) {
            routeDistance = WaypointDistanceFromTo(index, goalWP, pBot->current_team);

            if(routeDistance > -1 && (currentRoutDist == -1 || routeDistance < currentRoutDist) &&
                VectorsNearerThan(pBot->pEdict->v.origin, waypoints[index].origin, 800.0) &&
                BotCanSeeOrigin(pBot, waypoints[index].origin)) {
                // found a suitable waypoint
                pBot->current_wp = index;
                return TRUE;
            }
        }
    }

    return FALSE; // not a good result, the bot may stay stuck in a loop
}

// This function looks for a waypoint that will lead a bot away from the
// specified vector, and that is not visible from the specified vector.
// min_dist specifies the minimum retreat distance you want.
// Returns the waypoint found on success, -1 on failure.
int BotFindRetreatPoint(bot_t* const pBot, const int min_dist, const Vector& r_threatOrigin)
{
    if(pBot->current_wp == -1)
        return -1;

    // bit field of waypoint types to ignore
    static const WPT_INT32 ignoreFlags =
        0 + (W_FL_DELETED | W_FL_AIMING | W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY | W_FL_SNIPER |
                W_FL_TFC_TELEPORTER_ENTRANCE | W_FL_TFC_TELEPORTER_EXIT | W_FL_TFC_JUMP | W_FL_LIFT);

    // distance from bot to threat
    const float botThreatDistance = (pBot->pEdict->v.origin - r_threatOrigin).Length();

    int routeDistance;
    int nextWP;
    const int max_dist = min_dist + 1000; // used to stop bots running too far away
    int bestIndex = -1;
    TraceResult tr;

    // start the search
    int i, index = static_cast<int>(random_long(0, num_waypoints));
    for(i = 0; i < num_waypoints; i++, index++) {
        if(index >= num_waypoints)
            index = 0; // wrap if waypoint list exceeded

        // skip waypoints we don't want to consider
        if(waypoints[index].flags & ignoreFlags)
            continue;

        // skip this waypoint if it's team specific and teams don't match...
        if(!WaypointAvailable(index, pBot->current_team))
            continue;

        if(index == pBot->current_wp)
            continue; // skip the bots current waypoint

        routeDistance = WaypointDistanceFromTo(pBot->current_wp, index, pBot->current_team);

        if(routeDistance < min_dist || routeDistance > max_dist)
            continue; // ignore waypoints that are too near or too far away

        // find the next waypoint on the route to this waypoint
        nextWP = WaypointRouteFromTo(pBot->current_wp, index, pBot->current_team);

        // check for the ideal waypoint
        // is the next waypoint on the route further from the threat?
        if(!VectorsNearerThan(waypoints[nextWP].origin, r_threatOrigin, botThreatDistance)) {
            // is the end waypoint also further from the threat?
            if(!VectorsNearerThan(waypoints[index].origin, r_threatOrigin, botThreatDistance)) {
                bestIndex = index;

                // check for non-visibility
                UTIL_TraceLine(waypoints[index].origin, r_threatOrigin, ignore_monsters, NULL, &tr);

                // is this waypoint hidden by scenery?
                // if so immediately accept this waypoint as the best candidate
                if(tr.flFraction < 1.0)
                    break;
            }
        }
        // in case the search isn't having much luck remember any waypoint
        // that will at least send the bot away from the threat
        else {
            if(bestIndex == -1 && !VectorsNearerThan(waypoints[index].origin, r_threatOrigin, botThreatDistance))
                bestIndex = index;
        }
    }

    // if successful consider going for the next waypoint on the route right now
    if(bestIndex != -1) {
        // show where the bot should be going (for debugging purposes)
        //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
        //		waypoints[bestIndex].origin, 10, 2, 50, 250, 250, 200, 10);

        nextWP = WaypointRouteFromTo(pBot->current_wp, bestIndex, pBot->current_team);
        if(FVisible(waypoints[nextWP].origin, pBot->pEdict))
            pBot->current_wp = nextWP;
    }

    return bestIndex;
}

// This function looks for a waypoint that will lead a bot away from the
// specified entity(which is assumed to be some kind of explosive object).
// min_dist specifies the minimum retreat distance you want.
// Returns the waypoint found on success, -1 on failure.
int BotFindThreatAvoidPoint(bot_t* const pBot, const int min_dist, edict_t* pent)
{
    if(pBot->current_wp == -1 || FNullEnt(pent))
        return -1;

    // bit field of waypoint types to ignore
    static const WPT_INT32 ignoreFlags =
        0 + (W_FL_DELETED | W_FL_AIMING | W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY | W_FL_SNIPER |
                W_FL_TFC_TELEPORTER_ENTRANCE | W_FL_TFC_TELEPORTER_EXIT | W_FL_TFC_JUMP | W_FL_LIFT);

    // distance from bot to threat
    const float botThreatDistance = (pBot->pEdict->v.origin - pent->v.origin).Length();

    int routeDistance;
    int nextWP;
    const int max_dist = min_dist + 700; // used to stop bots running too far away

    // start the search
    int i, index = static_cast<int>(random_long(0, num_waypoints));
    for(i = 0; i < num_waypoints; i++, index++) {
        if(index >= num_waypoints)
            index = 0; // wrap if waypoint list exceeded

        // skip waypoints we don't want to consider
        if(waypoints[index].flags & ignoreFlags)
            continue;

        // skip this waypoint if it's team specific and teams don't match...
        if(!WaypointAvailable(index, pBot->current_team))
            continue;

        if(index == pBot->current_wp)
            continue; // skip the bots current waypoint

        routeDistance = WaypointDistanceFromTo(pBot->current_wp, index, pBot->current_team);

        if(routeDistance < min_dist || routeDistance > max_dist)
            continue; // ignore waypoints that are too near or too far away

        // pick the next waypoint on the route to this waypoint
        nextWP = WaypointRouteFromTo(pBot->current_wp, index, pBot->current_team);

        // we've found the right waypoint if the next waypoint on the route is further
        // from the threat than the bot is, and the end waypoint is far enough from the threat
        if(nextWP != -1 && !VectorsNearerThan(waypoints[nextWP].origin, pent->v.origin, botThreatDistance) &&
            !VectorsNearerThan(waypoints[index].origin, pent->v.origin, min_dist)) {
            // show where the bot should be going (for debugging purposes)
            //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
            //		waypoints[index].origin, 10, 2, 250, 50, 50, 200, 10);

            // consider going for the next waypoint on the route right now
            if(FVisible(waypoints[nextWP].origin, pBot->pEdict))
                pBot->current_wp = nextWP;

            return index;
        }
    }

    return -1; // failure
}

// This function returns the waypoint of a randomly selected flag waypoint
// or -1 on failure.
int BotFindFlagWaypoint(bot_t* pBot)
{
    int flagWP = -1;

    if(WaypointTypeExists(W_FL_TFC_FLAG, pBot->current_team))
        flagWP = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_FLAG);

    if(flagWP == -1)
        flagWP = WaypointFindRandomGoal(pBot->current_wp, -1, W_FL_TFC_FLAG);

    return flagWP;
}

// This function finds a waypoint near a random enemy defender waypoint.
// Returns the waypoint found on success, -1 on failure.
int BotTargetDefenderWaypoint(bot_t* pBot)
{
    // perform basic sanity checks
    if(pBot->current_wp < 0 || pBot->current_wp >= num_waypoints)
        return FALSE;

    // pick an enemy team to harrass
    const int target_team = PickRandomEnemyTeam(pBot->current_team);
    if(target_team == -1)
        return FALSE;

    // pick a random waypoint to start searching from
    int index = static_cast<int>(random_long(0, num_waypoints - 1));

    // bit field of waypoint types the bot is looking for
    static const WPT_INT32 validFlags = 0 + (W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY | W_FL_SNIPER);

    // start the search for an enemy defender waypoint
    int goalWP;
    for(int waypoints_checked = 0; waypoints_checked < num_waypoints; waypoints_checked++, index++) {
        // wrap the search if it exceeds the number of available waypoints
        if(index >= num_waypoints)
            index = 0;

        // skip any deleted or aiming waypoints
        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
            continue;

        // skip non-defender type waypoints
        // (i.e. not defender, demoman, sentry gun, sniper)
        if(!(waypoints[index].flags & validFlags))
            continue;

        if(index == pBot->current_wp)
            continue; // skip the bots current waypoint

        // skip this waypoint if it's not available to the enemy team
        if(!WaypointAvailable(index, target_team))
            continue;

        // skip waypoints that are too near to the bot
        if(VectorsNearerThan(waypoints[index].origin, waypoints[pBot->current_wp].origin, 400.0))
            continue;

        // look for a waypoint near the target and return true
        goalWP = WaypointFindRandomGoal_R(waypoints[index].origin, TRUE, 500.0, -1, 0);

        if(goalWP != -1) {
            //	char msg[80]; //DebugMessageOfDoom!
            //	sprintf(msg, "targetting defender waypoints, currentWP %d, newWP %d",
            //		pBot->current_wp, goalWP);
            //	UTIL_HostSay(pBot->pEdict, 0, msg);

            return goalWP;
        }
    }

    return -1;
}

// This function finds a waypoint near the furthest enemy defender
// waypoint, and was mainly intended for use when a bot becomes infected.
// Returns the waypoint found, -1 on failure.
int BotFindSuicideGoal(bot_t* pBot)
{
    if(pBot->current_wp == -1)
        return -1;

    // pick an enemy team to attack
    const int target_team = PickRandomEnemyTeam(pBot->current_team);
    if(target_team == -1)
        return -1;

    // if the bot has a known spawn location, set that as the waypoint to
    // search for the furthest enemy defender waypoint from
    int waypoint_from = spawnAreaWP[pBot->current_team];
    if(waypoint_from == -1)
        waypoint_from = pBot->current_wp;

    // bit field of waypoint types the bot is looking for
    static const WPT_INT32 validFlags = 0 + (W_FL_TFC_PL_DEFEND | W_FL_TFC_PIPETRAP | W_FL_TFC_SENTRY | W_FL_SNIPER);

    // try to find a waypoint which is further from the bot
    int furthestIndex = -1;
    float furthestDistance = 800.0f;
    float distance;
    for(int index = 0; index < num_waypoints; index++) {
        // skip non-defender type waypoints
        // (i.e. not defender, sentry gun, sniper)
        if(!(waypoints[index].flags & validFlags))
            continue;

        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
            continue; // skip any deleted or aiming waypoints

        // skip this waypoint if it's not available to the enemy team
        if(!WaypointAvailable(index, target_team))
            continue;

        if(index == pBot->current_wp)
            continue; // skip the bots current waypoint

        distance = (waypoints[index].origin - waypoints[waypoint_from].origin).Length();
        if(distance > furthestDistance) {
            furthestDistance = distance;
            furthestIndex = index;
        }
    }

    if(furthestIndex == -1)
        return -1;

    // look for a waypoint near the target waypoint and return it
    int goalWP = WaypointFindNearest_V(waypoints[furthestIndex].origin, REACHABLE_RANGE, pBot->current_team);

    return goalWP;
}

// BotFindFlagGoal - This function looks for a flag goal waypoint.
// Returns the waypoint found if successful, -1 otherwise.
int BotFindFlagGoal(bot_t* pBot)
{
    int goalWP = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_TFC_FLAG_GOAL);

    if(goalWP == -1)
        goalWP = WaypointFindRandomGoal(pBot->current_wp, -1, W_FL_TFC_FLAG_GOAL);

    return goalWP;
}

// BotGoForSniperSpot - This function sends a bot after a sniper spot.
// Returns the waypoint found.
int BotGoForSniperSpot(bot_t* pBot)
{
    int sniperWP;

    // occasionally go for the nearest snipe point(nearer is usually safer)
    if(static_cast<int>(pBot->pEdict->v.frags) <= pBot->scoreAtSpawn && random_long(1, 1000) < 334) {
        sniperWP = WaypointFindNearestGoal(pBot->current_wp, pBot->current_team, INT_MAX, W_FL_SNIPER);

        if(sniperWP == -1)
            sniperWP = WaypointFindNearestGoal(pBot->current_wp, -1, INT_MAX, W_FL_SNIPER);

        return sniperWP;
    }

    sniperWP = WaypointFindRandomGoal(pBot->current_wp, pBot->current_team, W_FL_SNIPER);

    if(sniperWP == -1)
        sniperWP = WaypointFindRandomGoal(pBot->current_wp, -1, W_FL_SNIPER);

    return sniperWP;
}

// This function will search for the nearest waypoint that is higher than the bot
// and near or above the surface of water.  i.e. somewhere a bot can go breathe.
// Returns the waypoint found, or -1 if not found.
int BotDrowningWaypointSearch(bot_t* pBot)
{
    int routeDistance;
    int minDistance = 2500;
    int bestIndex = -1;

    for(int index = 0; index < num_waypoints; index++) {
        // only consider plain waypoints(i.e. with no flags)
        if(waypoints[index].flags != 0)
            continue;

        // make sure the waypoint is higher than the bot
        if(waypoints[index].origin.z <= pBot->pEdict->v.origin.z)
            continue;

        routeDistance = WaypointDistanceFromTo(pBot->current_wp, index, pBot->current_team);
        if(routeDistance < minDistance && routeDistance != -1) {
            // accept the waypoint if there is empty space just above it
            if(UTIL_PointContents(waypoints[index].origin + Vector(0.0, 0.0, 40.0)) == CONTENTS_EMPTY) {
                minDistance = routeDistance;
                bestIndex = index;
            }
        }
    }

    return bestIndex;
}

// This function attempts to send the specified bot to a teleport entrance
// that will provide a quicker route to its ultimate goal.
// It returns TRUE on success, FALSE on failure.
bool BotFindTeleportShortCut(bot_t* pBot)
{
    if(bot_can_use_teleporter == FALSE || pBot->bot_has_flag || BufferContainsJobType(pBot, JOB_USE_TELEPORT))
        return FALSE;

    // the teleporter found has got to cut the route distance to less than this amount
    int shortestRoute = WaypointDistanceFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team) - 1200;

    // is the route too short anyway to bother with teleports?
    if(shortestRoute < 300)
        return FALSE;

    int shortestIndex = -1;
    int totalDistance;

    // search the bots memory of Teleporter pairs for a short cut
    for(int i = 0; i < MAX_BOT_TELEPORTER_MEMORY; i++) {
        // is this a pair of teleporters the bot has used before?
        // we assume the exit still exists, if it doesn't the bot can learn
        // about that when it tries to teleport
        if(pBot->telePair[i].entranceWP > -1 && pBot->telePair[i].entranceWP < num_waypoints &&
            pBot->telePair[i].exitWP > -1 && pBot->telePair[i].exitWP < num_waypoints &&
            pBot->telePair[i].entrance != NULL) {
            // distance via this known teleporter pair
            totalDistance = WaypointDistanceFromTo(pBot->current_wp, pBot->telePair[i].entranceWP, pBot->current_team) +
                WaypointDistanceFromTo(pBot->telePair[i].exitWP, pBot->goto_wp, pBot->current_team);

            // shortest route yet?
            if(totalDistance > -1 && totalDistance < shortestRoute) {
                shortestRoute = totalDistance;
                shortestIndex = i;
            }
        }
    }

    // found a teleporter to use that saves travel time?
    if(shortestIndex != -1) {
        job_struct* newJob = InitialiseNewJob(pBot, JOB_USE_TELEPORT);
        if(newJob != NULL) {
            newJob->object = pBot->telePair[shortestIndex].entrance;
            newJob->waypoint = pBot->telePair[shortestIndex].entranceWP;
            SubmitNewJob(pBot, JOB_USE_TELEPORT, newJob);
            //	UTIL_BotLogPrintf("%s, got tele shortcut via %d\n", pBot->name, newJob->waypoint);
        }

        return TRUE;
    }

    return FALSE;
}

// BotCheckForRocketJump - Here, we look through the list of RJ/CJ waypoints
// in the map to find the closest one.
// Once we find the closest we check to see if we can see it, and if
// it would save us distance to our current goal.
// If so, we initiate a Rocket Jump for that waypoint.
// There are numerous checks for abort conditions in here.
// TODO : This needs to be modified to consider the closest, say..
// 2-5 RJ waypoints. Otherwise points set somewhat close will be ignored
// and the closest will get favored every time, often resulting in the same point.
// AVOID TRACELINES TILL NECESSARY. Prioritize the top 3-5
// points by distance saved to the bots goal before
// checking for visibility.
static void BotCheckForRocketJump(bot_t* pBot)
{
    // sanity checking
    if(pBot->current_wp == -1 || pBot->bot_skill > 3 || num_waypoints < 1 ||
        BufferContainsJobType(pBot, JOB_ROCKET_JUMP))
        return;

    char* cvar_ntf = (char*)CVAR_GET_STRING("neotf");
    // char *cvar_ntfexclusive = (char *)CVAR_GET_STRING("ntf_feature_exclusive");

    // allow Neotf spies and pyros to jump at RJ points
    if(pBot->pEdict->v.playerclass == TFC_CLASS_PYRO) {
        char* cvar_jetpack = (char*)CVAR_GET_STRING("ntf_feature_jetpack");
        if((strcmp(cvar_ntf, "1") == 0) && (strcmp(cvar_jetpack, "1") == 0)) // No neotf or jetpack
        {
            // Jetpack enabled
        } else
            return;
    } else if(pBot->pEdict->v.playerclass == TFC_CLASS_SPY) {
        return; // TODO : KICK SPIES OUT FOR NOW, UNTIL HOVERBOARD.

        /*	char *cvar_hoverboard = (char *)CVAR_GET_STRING("ntf_feature_hoverboard");
                if((strcmp(cvar_ntf, "1") == 0)
                        && (strcmp(cvar_hoverboard, "1") == 0)) // No neotf or jetpack
                {
                        // Hoverboard enabled
                }
                else return;*/
    } else if((PlayerHealthPercent(pBot->pEdict) - 15) < pBot->trait.health ||
        (PlayerArmorPercent(pBot->pEdict) - 26) < pBot->trait.health) {
        // stop soldiers from rocket jumping when with low health/armour
        // 1 rocket jump subtracts a maximum of 15 health and 55 armour
        return;
    }

    int closestRJ = -1;
    float closest2D = 500.1;
    float distance2D;
    float zDiff;

    // random to simulate how high the bot thinks the jump will go this time
    const float maxJumpHeight = random_float(340.0, 440.0);

    // find the closest rocket jump point
    for(int i = 0; i < MAXRJWAYPOINTS; i++) {
        // -1 means we are at the end of the list.
        if(RJPoints[i][RJ_WP_INDEX] == -1)
            break;

        // If its our team or not team specific.
        if(RJPoints[i][RJ_WP_TEAM] == -1 || RJPoints[i][RJ_WP_TEAM] == pBot->current_team) {
            zDiff = waypoints[RJPoints[i][RJ_WP_INDEX]].origin.z - pBot->pEdict->v.origin.z;

            // is this RJ waypoints height reachable with a rocket jump?
            // on a server with 800 gravity rocket jumps can reach a height of about 440
            if(zDiff > 54.0 && zDiff < maxJumpHeight) {
                distance2D = (pBot->pEdict->v.origin - waypoints[RJPoints[i][RJ_WP_INDEX]].origin).Length2D();

                if(distance2D > 150.0 // don't want RJ points that are too close
                    && distance2D < closest2D) {
                    closest2D = distance2D;
                    closestRJ = RJPoints[i][RJ_WP_INDEX];
                }
            }
        }
    }

    /*char msg[80];
       sprintf(msg, "Closest Jump Index: %d Distance2D: %f ",
                    closestRJ, closest2D);
       UTIL_HostSay(pBot->pEdict, 0, msg);*/

    // If theres no RJ point get out.
    if(closestRJ == -1)
        return;

    // Check if its a good time to jump
    // Resulting waypoint closer to goal than we are now
    // (> 1000 distance savings for now)
    int distanceSaved = (WaypointDistanceFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team) -
        WaypointDistanceFromTo(closestRJ, pBot->goto_wp, pBot->current_team));

    // Don't bother if the distance it saves us is less than this.
    if(distanceSaved < 1000)
        return;

    TraceResult result;

    // Check visibility from where the bot's head is to a point in the air above
    // i.e. check for a low ceiling
    zDiff = waypoints[closestRJ].origin.z - pBot->pEdict->v.origin.z;
    UTIL_TraceLine(pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs, pBot->pEdict->v.origin + Vector(0.0, 0.0, zDiff),
        ignore_monsters, pBot->pEdict->v.pContainingEntity, &result);

    if(result.flFraction < 1.0)
        return; // can't see it

    // Check visibility from a point in the air above the bot to the RJ waypoint
    // this improves the bots ability to properly detect RJ waypoints
    UTIL_TraceLine(pBot->pEdict->v.origin + Vector(0.0, 0.0, zDiff), waypoints[closestRJ].origin, ignore_monsters,
        pBot->pEdict->v.pContainingEntity, &result);

    if(result.flFraction < 1.0)
        return; // can't see it
    else {
        // debug stuff
        //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
        //		waypoints[closestRJ].origin, 10, 2, 250, 250, 50, 200, 10);

        //	UTIL_HostSay(pBot->pEdict, 0, "RJ waypoint seen");

        // set up a job to handle the jump
        job_struct* newJob = InitialiseNewJob(pBot, JOB_ROCKET_JUMP);
        if(newJob != NULL) {
            newJob->waypoint = closestRJ;
            SubmitNewJob(pBot, JOB_ROCKET_JUMP, newJob);
        }

        //	UTIL_BotLogPrintf("%s: RJ waypoint %d\n", pBot->name, newJob.waypoint);

        /*char msg[80];
        sprintf(msg, "CloseDist: %f Distance: %f zDiff %f ",
                pBot->RJClosingDistance, distance2D, zDiff);
        UTIL_HostSay(pBot->pEdict, 0, msg);*/
    }
}

// BotCheckForConcJump - Here, we look through the list of RJ/CJ
// waypoints in the map to find the closest one.
// Once we find the closest we check to see if we can see it, and if it
// would save us distance to our current goal.
// If so, we initiate a Conc Jump for that waypoint. There are numerous
// checks for abort conditions in here.
// TODO : This needs to be modified to consider the closest, say.. 2-5
// Conc waypoints. Otherwise points set somewhat close will be ignored and
// the closest will get favored every time, often resulting in the same point.
// AVOID TRACELINES TILL NECESSARY. Prioritize the top 3-5 points by
// distance saved to the bots goal before checking for visibility.
static void BotCheckForConcJump(bot_t* pBot)
{
    // sanity check
    if(pBot->current_wp == -1 || pBot->goto_wp == -1 || pBot->bot_skill > 3 || pBot->grenades[SecondaryGrenade] < 1 ||
        num_waypoints < 1 || bot_use_grenades == 0)
        return;

    // Lets not check for concs if we are close to our goal.
    if(WaypointDistanceFromTo(pBot->current_wp, pBot->goto_wp, pBot->current_team) < 2000)
        return;

    // make sure we can set up a concussion jump job to handle the jump itself
    if(BufferContainsJobType(pBot, JOB_CONCUSSION_JUMP))
        return;

    // We need to look ahead based on this speed, to try and estimate
    // where we will be in 4 seconds
    // If we can see a conc point from there, prime now.

    // Need to get my velocity
    const float mySpeed = pBot->pEdict->v.velocity.Length2D();

    // Lets try enforcing a minimum speed for considering jumps.
    if(mySpeed < 180.0f)
        return;

    int currentWP = pBot->current_wp;
    int endWP = -1;
    float distAhead = 0;
    int safetyCounter = 0;

    // try to predict what waypoint the bot will be at a few seconds
    // from now
    while((distAhead < (mySpeed * 3.0f)) && (safetyCounter < 30)) {
        // Get the next waypoint in our route.
        endWP = WaypointRouteFromTo(currentWP, pBot->goto_wp, pBot->current_team);
        if(endWP == -1 || endWP > num_waypoints)
            return;

        // Add the distance
        distAhead += WaypointDistanceFromTo(currentWP, endWP, pBot->current_team);

        // Set current wp to the next one.
        // This should keep checking ahead with each loop iteration.
        currentWP = endWP;
        safetyCounter++;
    }

    // Abort if for some reason the safety counters limit was reached.
    if(safetyCounter == 30) {
        // UTIL_HostSay(pBot->pEdict, 0, "Safety counter hit");
        return;
    }

    int closestJumpWP = -1;
    float closest2D = random_float(400.0, 700.0); // random to cover different situations
    float distance2D;
    float zDiff;

    // Find the closest RJ point from the bots predicted waypoint location
    for(int i = 0; i < MAXRJWAYPOINTS; i++) {
        // -1 means we are at the end of the list.
        if(RJPoints[i][RJ_WP_INDEX] == -1)
            break;

        // If its our team or not team specific.
        if((RJPoints[i][RJ_WP_TEAM] == -1 || RJPoints[i][RJ_WP_TEAM] == pBot->current_team) &&
            RJPoints[i][RJ_WP_INDEX] != -1) {
            zDiff = waypoints[RJPoints[i][RJ_WP_INDEX]].origin.z - waypoints[endWP].origin.z;

            // is this RJ waypoints height reachable with a concussion jump?
            // on a server with 800 gravity concussion jumps can reach a height of about 490
            if(zDiff > 54.0 && zDiff < 450.0) {
                distance2D = (waypoints[endWP].origin - waypoints[RJPoints[i][RJ_WP_INDEX]].origin).Length2D();

                if(distance2D < closest2D) {
                    closest2D = distance2D;
                    closestJumpWP = RJPoints[i][RJ_WP_INDEX];
                }
            }
        }
    }

    // Abort if theres no RJ waypoint near enough.
    if(closestJumpWP == -1)
        return;

    const float distanceSaved = (WaypointDistanceFromTo(endWP, pBot->goto_wp, pBot->current_team) -
        WaypointDistanceFromTo(closestJumpWP, pBot->goto_wp, pBot->current_team));

    // Don't bother if the distance it saves us is less than this.
    if(distanceSaved < 1000.0f)
        return;

    // Got the waypoint out near our reach.
    /*if(g_bot_debug)
            WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                    waypoints[endWP].origin, 30, 0, 125, 125, 125, 250, 5);*/

    TraceResult result;

    // Check visibility from where the bot's head will be to a point in the air above
    // i.e. check for a low ceiling
    zDiff = waypoints[closestJumpWP].origin.z - waypoints[endWP].origin.z;
    UTIL_TraceLine(waypoints[endWP].origin + pBot->pEdict->v.view_ofs,
        waypoints[endWP].origin + Vector(0.0, 0.0, zDiff), ignore_monsters, pBot->pEdict->v.pContainingEntity, &result);

    if(result.flFraction < 1.0)
        return; // can't see it

    // Check visibility from a point in the air above the bot to the RJ waypoint
    // this improves the bots ability to properly detect RJ waypoints
    UTIL_TraceLine(waypoints[endWP].origin + Vector(0.0, 0.0, zDiff), waypoints[closestJumpWP].origin, ignore_monsters,
        pBot->pEdict->v.pContainingEntity, &result);

    if(result.flFraction < 1.0)
        return; // can't see it
    else        // success - it's time to set up a concussion jump job
    {
        job_struct* newJob = InitialiseNewJob(pBot, JOB_CONCUSSION_JUMP);
        if(newJob != NULL) {
            newJob->waypoint = endWP;
            newJob->waypointTwo = closestJumpWP;

            SubmitNewJob(pBot, JOB_CONCUSSION_JUMP, newJob);
        }
    }
}
