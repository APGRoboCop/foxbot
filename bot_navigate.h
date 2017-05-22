//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_navigate.h
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

#ifndef BOT_NAVIGATE_H
#define BOT_NAVIGATE_H

// standard amount of time to reach the bots current waypoint
#define BOT_WP_DEADLINE 7.0

void BotUpdateHomeInfo(bot_t *pBot);

void BotFindCurrentWaypoint(bot_t *pBot);

void BotSetFacing(bot_t *pBot, Vector v_focus);

void BotFixIdealPitch( edict_t *pEdict );

float BotChangePitch( edict_t *pEdict, float speed );

void BotFixIdealYaw( edict_t *pEdict );

float BotChangeYaw( edict_t *pEdict, float speed );

void BotNavigateWaypointless(bot_t *pBot);

bool BotNavigateWaypoints(bot_t *pBot, bool navByStrafe);

bool BotHeadTowardWaypoint( bot_t *pBot, bool &r_navByStrafe );

void BotUseLift( bot_t *pBot );

bool BotCheckWallOnLeft( bot_t *pBot );

bool BotCheckWallOnRight( bot_t *pBot );

int BotFindFlagWaypoint(bot_t * pBot);

int BotFindFlagGoal(bot_t * pBot);

int BotGoForSniperSpot(bot_t *pBot);

int BotTargetDefenderWaypoint(bot_t *pBot);

int BotGetDispenserBuildWaypoint(bot_t *pBot);

int BotGetTeleporterBuildWaypoint(bot_t *pBot, const bool buildEntrance);

void BotFindSideRoute(bot_t *pBot);

bool BotPathCheck(const int sourceWP, const int destWP);

bool BotChangeRoute(bot_t *pBot);

bool BotSetAlternativeGoalWaypoint(bot_t * const pBot, int &r_goalWP, const WPT_INT32 flags);

int BotFindSuicideGoal(bot_t *pBot);

int BotFindRetreatPoint(bot_t * const pBot, const int min_dist,
	const Vector &r_threatOrigin);

int BotFindThreatAvoidPoint(bot_t * const pBot, const int min_dist, edict_t *pent);

int BotDrowningWaypointSearch(bot_t *pBot);

bool BotFindTeleportShortCut(bot_t *pBot);

#endif // BOT_NAVIGATE_H
