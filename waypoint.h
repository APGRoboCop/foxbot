//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// waypoint.h
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

#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <limits.h>

// standard variable sizes, useful for the waypoint code because the waypoint files
// are saved in binary format
// systypes.h can provide this but some compilers(e.g an old Borland one) don't use it
typedef signed char WPT_INT8;
typedef signed short WPT_INT16;
typedef signed int WPT_INT32;

#define MAX_WAYPOINTS 32000

#define REACHABLE_RANGE 800.0

// defines for waypoint flags field (32 bits are available)
#define W_FL_TEAM ((1 << 0) + (1 << 1)) /* allow for 4 teams (0-3) */
#define W_FL_TEAM_SPECIFIC (1 << 2)     /* waypoint only for specified team */
#define W_FL_CROUCH (1 << 3)            /* must crouch to reach this waypoint */
#define W_FL_LADDER (1 << 4)            /* waypoint is on a ladder */
#define W_FL_LIFT (1 << 5)              /* wait for lift to be down before approaching this waypoint */
#define W_FL_WALK (1 << 6)              /* walk towards this waypoint */
#define W_FL_HEALTH (1 << 7)            /* health kit (or wall mounted) location */
#define W_FL_ARMOR (1 << 8)             /* armor (or HEV) location */
#define W_FL_AMMO (1 << 9)              /* ammo location */
#define W_FL_SNIPER (1 << 10)           /* sniper waypoint (a good sniper spot) */

#define W_FL_TFC_FLAG (1 << 11)      /* flag position (or hostage or president) */
#define W_FL_TFC_FLAG_GOAL (1 << 12) /* flag return position (or rescue zone) */

#define W_FL_TFC_SENTRY (1 << 13)     /* sentry gun */
#define W_FL_TFC_SENTRY_180 (1 << 28) /* 180 SG point!! */
#define W_FL_AIMING (1 << 14)         /* aiming waypoint */

#define W_FL_JUMP (1 << 15) /* standard jump waypoint */

#define W_FL_TFC_PIPETRAP (1 << 24)      /* demoman pipetrap */
#define W_FL_TFC_DETPACK_CLEAR (1 << 25) /* demoman detpack(blow passageway open) */
#define W_FL_TFC_DETPACK_SEAL (1 << 16)  /* demoman detpack(blow passageway closed) */

#define W_FL_TFC_JUMP (1 << 27) /* rocket/concussion jump */

#define W_FL_TFC_TELEPORTER_ENTRANCE (1 << 26)
#define W_FL_TFC_TELEPORTER_EXIT (1 << 30)

#define W_FL_TFC_PL_DEFEND (1 << 29) /* bot defense point!! */

#define W_FL_PATHCHECK (1 << 17) /* bots should check if waypoint path is blocked */

#define W_FL_DELETED (1 << 31) /* used by waypoint allocation code */

// script number flags 1 - 8
#define S_FL_POINT1 (1 << 0) /* point1 */
#define S_FL_POINT2 (1 << 1) /* point2 */
#define S_FL_POINT3 (1 << 2) /* point3 */
#define S_FL_POINT4 (1 << 3) /* point4 */
#define S_FL_POINT5 (1 << 4) /* point5 */
#define S_FL_POINT6 (1 << 5) /* point6 */
#define S_FL_POINT7 (1 << 6) /* point7 */
#define S_FL_POINT8 (1 << 7) /* point8 */

#define WAYPOINT_VERSION 5

// define the waypoint file header structure...
typedef struct {
    char filetype[8]; // should be "FoXBot\0"
    WPT_INT32 waypoint_file_version;
    WPT_INT32 waypoint_file_flags; // not currently used
    WPT_INT32 number_of_waypoints;
    char mapname[32]; // name of map for these waypoints
} WAYPOINT_HDR;

// define the structure for waypoints...
typedef struct {
    WPT_INT32 flags;       // button, lift, flag, health, ammo, etc.
    WPT_INT8 script_flags; // script numbers 1 - 8
    Vector origin;         // map location
} WAYPOINT;

#define WAYPOINT_UNREACHABLE UINT_MAX
#define WAYPOINT_MAX_DISTANCE (UINT_MAX - 1)

#define MAX_PATH_INDEX 4

// define the structure for waypoint paths (paths are connections between
// two waypoint nodes that indicates the bot can get from point A to point B.
// note that paths DON'T have to be two-way.  sometimes they are just one-way
// connections between two points.  There is an array called "paths" that
// contains head pointers to these structures for each waypoint index.
typedef struct path {
    WPT_INT16 index[MAX_PATH_INDEX]; // indexes of waypoints (-1 means not used)
    struct path* next;               // link to next structure
} PATH;

#define A_FL_1 (1 << 0)
#define A_FL_2 (1 << 1)
#define A_FL_3 (1 << 2)
#define A_FL_4 (1 << 3)

typedef struct area {
    Vector a;       // location
    Vector b;       // location
    Vector c;       // location
    Vector d;       // location
    char namea[64]; // team1's name
    char nameb[64]; // team2's name
    char namec[64]; // team3's name
    char named[64]; // team4's name
    WPT_INT32 flags;
} AREA;

#define AREA_VERSION 1

// define the area file header structure...
typedef struct {
    char filetype[8]; // should be "FoXBot\0"
    WPT_INT32 area_file_version;
    WPT_INT32 number_of_areas;
    char mapname[32]; // name of map for these areas
} AREA_HDR;

// waypoint function prototypes...
void WaypointInit(void);

int WaypointFindPath(PATH** pPath, int* path_index, int waypoint_index, int team);

int WaypointFindNearest_E(edict_t* pEntity, const float range, const int team);

int WaypointFindNearest_V(Vector v_src, const float range, const int team);

int WaypointFindNearest_S(Vector v_src,
    edict_t* pEntity,
    const float range,
    const int team,
    const WPT_INT32 ignore_flags);

int WaypointFindInRange(Vector v_src,
    const float min_range,
    const float max_range,
    const int team,
    const bool chooseRandom);

int WaypointFindNearestGoal(const int srcWP, const int team, int range, const WPT_INT32 flags);

int WaypointFindRandomGoal(const int source_WP, const int team, const WPT_INT32 flags);

int WaypointFindRandomGoal_D(const int source_WP, const int team, const int range, const WPT_INT32 flags);

int WaypointFindRandomGoal_R(Vector v_src,
    const bool checkVisibility,
    const float range,
    const int team,
    const WPT_INT32 flags);

int WaypointFindDetpackGoal(const int srcWP, const int team);

bool DetpackClearIsBlocked(const int index);

bool DetpackSealIsClear(const int index);

int WaypointFindNearestAiming(const Vector& r_v_origin);

void WaypointAdd(edict_t* pEntity);

void WaypointAddAiming(edict_t* pEntity);

void WaypointDelete(edict_t* pEntity);

void WaypointCreatePath(edict_t* pEntity, int cmd);

void WaypointRemovePath(edict_t* pEntity, int cmd);

bool WaypointTypeExists(const WPT_INT32 flags, const int team);

bool WaypointLoad(edict_t* pEntity);

void WaypointSave(void);

bool WaypointReachable(Vector v_srv, Vector v_dest, edict_t* pEntity);

bool WaypointDirectPathCheck(const int srcWP, const int destWP);

void WaypointPrintInfo(edict_t* pEntity);

void WaypointThink(edict_t* pEntity);

int WaypointRouteFromTo(int src, int dest, int team);

int WaypointDistanceFromTo(int src, int dest, int team);

void WaypointDrawBeam(edict_t* pEntity,
    Vector start,
    Vector end,
    int width,
    int noise,
    int red,
    int green,
    int blue,
    int brightness,
    int speed);

bool WaypointAvailable(const int index, const int team);

void WaypointRunOneWay(edict_t* pEntity);

void WaypointRunTwoWay(edict_t* pEntity);

void WaypointAutoBuild(edict_t* pEntity);

void AreaDefCreate(edict_t* pEntity);

int AreaDefPointFindNearest(edict_t* pEntity, float range, int flags);

void AreaDefDelete(edict_t* pEntity);

void AreaDefSave(void);

bool AreaDefLoad(edict_t* pEntity);

void AreaDefPrintInfo(edict_t* pEntity);

bool AreaInside(edict_t* pEntity, int i);

int AreaInsideClosest(edict_t* pEntity);

void AreaAutoBuild1(void);

void AreaAutoMerge(void);

void ProcessCommanderList(void);
#endif // WAYPOINT_H
