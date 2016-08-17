//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_combat.cpp
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

#define NADEVELOCITY 650 // DrEvil #define

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_job_think.h"
#include "bot_weapons.h"
#include "waypoint.h"
#include "bot_navigate.h"

extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints; // number of waypoints currently in map

// area defs...
extern AREA areas[MAX_WAYPOINTS];
extern int num_areas;

extern int mod_id;
extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern bool b_observer_mode;
extern edict_t* pent_info_ctfdetect;
extern float is_team_play;
extern bool checked_teamplay;

// bot settings //////////////////
extern int bot_use_grenades;
extern bool offensive_chatter;
extern int bot_skill_1_aim;   // accuracy for skill 1 bots
extern int bot_aim_per_skill; // accuracy modifier for bots from skill 1 downwards

// accuracy levels for each bot skill level
static float bot_max_inaccuracy[5] = { 20.0, 30.0, 40.0, 50.0, 60.0 };
static float bot_snipe_max_inaccuracy[5] = { 10.0, 20.0, 30.0, 40.0, 50.0 };

extern bool is_team[4];
extern int team_allies[4];

extern bot_t bots[32];

// list of which player indices carry a flag(updated each frame)
// You should call PlayerHasFlag() rather than use this array directly, because of
// Half-Life engine complications(client list starts with index of 1 instead of 0).
static bool playerHasFlag[32];

// FUNCTION PROTOTYPES ///////////////
static void BotPipeBombCheck(bot_t* pBot);
static edict_t* BotFindEnemy(bot_t* pBot);
static bool BotSpyDetectCheck(bot_t* pBot, edict_t* Spy);
static void BotSGSpotted(bot_t* pBot, edict_t* sg);
static bool BotPrimeGrenade(bot_t* pBot, const int slot, const char nadeType, const unsigned short reserve);
static bool BotClearShotCheck(bot_t* pBot);
static Vector BotBodyTarget(edict_t* pBotEnemy, bot_t* pBot);
static int BotLeadTarget(edict_t* pBotEnemy, bot_t* pBot, const int projSpeed, float& d_x, float& d_y, float& d_z);

typedef struct {
    int iId;                      // the weapon ID value
    char weapon_name[64];         // name of the weapon when selecting it
    int skill_level;              // bot skill must be less than or equal to this value
    float primary_min_distance;   // 0 = no minimum
    float primary_max_distance;   // 9999 = no maximum
    float secondary_min_distance; // 0 = no minimum
    float secondary_max_distance; // 9999 = no maximum
    int use_percent;              // times out of 100 to use this weapon when available
    bool can_use_underwater;      // can use this weapon underwater
    int primary_fire_percent;     // times out of 100 to use primary fire
    int min_primary_ammo;         // minimum ammout of primary ammo needed to fire
    int min_secondary_ammo;       // minimum ammout of secondary ammo needed to fire
    bool primary_fire_hold;       // hold down primary fire button to use?
    bool secondary_fire_hold;     // hold down secondary fire button to use?
    bool primary_fire_charge;     // charge weapon using primary fire?
    bool secondary_fire_charge;   // charge weapon using secondary fire?
    float primary_charge_delay;   // time to charge weapon
    float secondary_charge_delay; // time to charge weapon
} bot_weapon_select_t;

typedef struct {
    int iId;
    float primary_base_delay;
    float primary_min_delay[5];
    float primary_max_delay[5];
    float secondary_base_delay;
    float secondary_min_delay[5];
    float secondary_max_delay[5];
} bot_fire_delay_t;

// This holds the multigun names we will check using a repeat loop
#define NumNTFGuns 8
char* ntfTargetChecks[] = {
    "ntf_teslacoil", "ntf_grenlauncher", "ntf_rocklauncher", "ntf_lrlauncher", "ntf_flamegun", "ntf_crowbar",
    "ntf_displacer", "ntf_biocannon",
};

// weapons are stored in priority order, most desired weapon should be at
// the start of the array and least desired should be at the end
static bot_weapon_select_t tfc_weapon_select[] = { { TF_WEAPON_KNIFE, "tf_weapon_knife", 5, 0.0, 90.0, 0.0, 0.0, 100,
                                                       TRUE, 100, 0, 0, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0 },
    { TF_WEAPON_SPANNER, "tf_weapon_spanner", 5, 0.0, 90.0, 0.0, 0.0, 100, TRUE, 100, 0, 0, FALSE, FALSE, FALSE, FALSE,
        0.0, 0.0 },
    { TF_WEAPON_MEDIKIT, "tf_weapon_medikit", 5, 0.0, 90.0, 0.0, 0.0, 100, TRUE, 100, 0, 0, FALSE, FALSE, FALSE, FALSE,
        0.0, 0.0 },
    { TF_WEAPON_SNIPERRIFLE, "tf_weapon_sniperrifle", 5, 300.0, 4000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE,
        TRUE, FALSE, 2.0, 0.0 },
    { TF_WEAPON_FLAMETHROWER, "tf_weapon_flamethrower", 5, 0.0, 400.0, 0.0, 0.0, 100, FALSE, 100, 1, 0, TRUE, FALSE,
        FALSE, FALSE, 0.0, 0.0 },
    { TF_WEAPON_AC, "tf_weapon_ac", 5, 0.0, 2500.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, TRUE, FALSE, FALSE, FALSE, 0.0,
        0.0 },
    { TF_WEAPON_RPG, "tf_weapon_rpg", 5, 180.0, 3000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0,
        0.0 },
    { TF_WEAPON_IC, "tf_weapon_ic", 5, 300.0, 2000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0,
        0.0 },
    { TF_WEAPON_SUPERSHOTGUN, "tf_weapon_supershotgun", 5, 0.0, 2000.0, 0.0, 0.0, 100, TRUE, 100, 2, 0, FALSE, FALSE,
        FALSE, FALSE, 0.0, 0.0 },
    { TF_WEAPON_SUPERNAILGUN, "tf_weapon_superng", 5, 40.0, 3000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, TRUE, FALSE, FALSE,
        FALSE, 0.0, 0.0 },
    { TF_WEAPON_TRANQ, "tf_weapon_tranq", 5, 40.0, 1000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE,
        0.0, 0.0 },
    { TF_WEAPON_AUTORIFLE, "tf_weapon_autorifle", 5, 0.0, 1000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE,
        FALSE, 0.0, 0.0 },
    { TF_WEAPON_AXE, "tf_weapon_axe", 5, 0.0, 90.0, 0.0, 0.0, 100, TRUE, 100, 0, 0, FALSE, FALSE, FALSE, FALSE, 0.0,
        0.0 },
    { TF_WEAPON_PL, "tf_weapon_pl", 5, 400.0, 900.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0,
        0.0 },
    { TF_WEAPON_GL, "tf_weapon_gl", 5, 180.0, 900.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE, 0.0,
        0.0 },
    { TF_WEAPON_SHOTGUN, "tf_weapon_shotgun", 5, 0.0, 4000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE,
        FALSE, 0.0, 0.0 },
    { TF_WEAPON_NAILGUN, "tf_weapon_ng", 5, 0.0, 3000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE, FALSE,
        0.0, 0.0 },
    { TF_WEAPON_RAILGUN, "tf_weapon_railgun", 5, 40.0, 4000.0, 0.0, 0.0, 100, TRUE, 100, 1, 0, FALSE, FALSE, FALSE,
        FALSE, 0.0, 0.0 },
    /* terminator */
    { 0, "", 0, 0.0, 0.0, 0.0, 0.0, 0, TRUE, 0, 1, 1, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0 } };

static bot_fire_delay_t tfc_fire_delay[] = {
    { TF_WEAPON_KNIFE, 0.3, { 0.0, 0.2, 0.3, 0.4, 0.6 }, { 0.1, 0.3, 0.5, 0.7, 1.0 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_SPANNER, 0.3, { 0.0, 0.2, 0.3, 0.4, 0.6 }, { 0.1, 0.3, 0.5, 0.7, 1.0 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_MEDIKIT, 0.3, { 0.0, 0.2, 0.3, 0.4, 0.6 }, { 0.1, 0.3, 0.5, 0.7, 1.0 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_SNIPERRIFLE, 0.5, { 0.0, 0.2, 0.6, 0.8, 1.0 }, { 0.3, 0.5, 0.7, 0.9, 1.1 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_FLAMETHROWER, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_AC, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_RPG, 0.6, { 0.0, 0.1, 0.3, 0.6, 1.0 }, { 0.1, 0.2, 0.7, 1.0, 2.0 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_IC, 0.6, { 1.0, 2.0, 3.0, 4.0, 5.0 }, { 3.0, 4.0, 5.0, 6.0, 7.0 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_SUPERSHOTGUN, 0.6, { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.25, 0.4, 0.7, 1.0, 1.3 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_SUPERNAILGUN, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_TRANQ, 1.5, { 1.0, 2.0, 3.0, 4.0, 5.0 }, { 3.0, 4.0, 5.0, 6.0, 7.0 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_AUTORIFLE, 0.1, { 0.0, 0.1, 0.2, 0.4, 0.6 }, { 0.1, 0.2, 0.5, 0.7, 1.0 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_AXE, 0.3, { 0.0, 0.2, 0.3, 0.4, 0.6 }, { 0.1, 0.3, 0.5, 0.7, 1.0 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_PL, 0.6, { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.25, 0.4, 0.7, 1.0, 1.3 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_GL, 0.6, { 0.0, 0.2, 0.5, 0.8, 1.0 }, { 0.25, 0.4, 0.7, 1.0, 1.3 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_SHOTGUN, 0.5, { 0.0, 0.2, 0.4, 0.6, 0.8 }, { 0.25, 0.5, 0.8, 1.2, 2.0 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_NAILGUN, 0.1, { 0.0, 0.1, 0.2, 0.4, 0.6 }, { 0.1, 0.2, 0.5, 0.7, 1.0 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    { TF_WEAPON_RAILGUN, 0.4, { 0.0, 0.1, 0.2, 0.3, 0.4 }, { 0.1, 0.2, 0.3, 0.4, 0.5 }, 0.0,
        { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 } },
    /* terminator */
    { 0, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0, 0.0 }, 0.0, { 0.0, 0.0, 0.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0 } }
};

void BotCheckTeamplay(void)
{
    // is this TFC or Counter-Strike or OpFor teamplay or FrontLineForce?
    if(mod_id == TFC_DLL && pent_info_ctfdetect != NULL)
        is_team_play = 1.0;
    else
        is_team_play = CVAR_GET_FLOAT("mp_teamplay"); // teamplay enabled?

    checked_teamplay = TRUE;
}

// This function can be used to report the number of players using the specified class
// on the specified players team, or a team allied to them.
// You'll be cheating if you use this function on an enemy team.  >:-O
int FriendlyClassTotal(edict_t* pEdict, const int specifiedClass, const bool ignoreSelf)
{
    if(!checked_teamplay) // check for team play...
        BotCheckTeamplay();

    // report failure if there is no team play
    if(is_team_play <= 0 || mod_id != TFC_DLL)
        return FALSE;

    const int my_team = UTIL_GetTeam(pEdict);
    edict_t* pPlayer;
    int player_team;
    int classTotal = 0;

    // search the world for players...
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        pPlayer = INDEXENT(i);

        // skip invalid players
        if(pPlayer && !pPlayer->free) {
            // check the specified player if instructed to do so
            if(pPlayer == pEdict) {
                if(pPlayer->v.playerclass == specifiedClass && !ignoreSelf)
                    ++classTotal;
            } else if(IsAlive(pPlayer) && pPlayer->v.playerclass == specifiedClass) {
                player_team = UTIL_GetTeam(pPlayer);

                // add another if the player is a teammate or ally
                if(my_team == player_team || team_allies[my_team] & (1 << player_team))
                    ++classTotal;
            }
        }
    }

    return classTotal;
}

// This function should be called whenever bot_skill_1_aim or bot_aim_per_skill
// are changed, because it updates the bot inaccuracy levels based on those two
// config settings.
void BotUpdateSkillInaccuracy(void)
{
    const float f_aim_per_skill = static_cast<float>(bot_aim_per_skill);

    bot_max_inaccuracy[0] = static_cast<float>(bot_skill_1_aim);
    bot_max_inaccuracy[1] = bot_max_inaccuracy[0] + f_aim_per_skill;
    bot_max_inaccuracy[2] = bot_max_inaccuracy[1] + f_aim_per_skill;
    bot_max_inaccuracy[3] =
        bot_max_inaccuracy[2] + f_aim_per_skill; // Foxbot >0.77 Version had a typo? [APG]RoboCop[CL]
    bot_max_inaccuracy[4] = bot_max_inaccuracy[3] + f_aim_per_skill;

    // sniper rifle inaccuracy starts out a bit lower than with other weapons
    // but then scales up in the same way
    bot_snipe_max_inaccuracy[0] = bot_max_inaccuracy[0] * 0.75;
    bot_snipe_max_inaccuracy[1] = bot_snipe_max_inaccuracy[0] + f_aim_per_skill;
    bot_snipe_max_inaccuracy[2] = bot_snipe_max_inaccuracy[1] + f_aim_per_skill;
    bot_snipe_max_inaccuracy[3] = bot_snipe_max_inaccuracy[2] + f_aim_per_skill;
    bot_snipe_max_inaccuracy[4] = bot_snipe_max_inaccuracy[3] + f_aim_per_skill;

    //	UTIL_BotLogPrintf("New bot accuracy levels %f %f %f %f %f\n",
    //		bot_max_inaccuracy[0], bot_max_inaccuracy[1], bot_max_inaccuracy[2],
    //		bot_max_inaccuracy[3], bot_max_inaccuracy[4]);
}

// Set pipebombs off if they are near to the bots enemy.
static void BotPipeBombCheck(bot_t* pBot)
{
    edict_t* pent = NULL;
    while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "tf_gl_pipebomb")) != NULL && !FNullEnt(pent)) {
        if(pBot->pEdict == pent->v.owner && VectorsNearerThan(pBot->enemy.ptr->v.origin, pent->v.origin, 90.0)) {
            FakeClientCommand(pBot->pEdict, "detpipe", NULL, NULL);
            return;
        }
    }
}

// This function was created for use by feigning Spies.
// It will search for an enemy player who is near to the Spy and facing
// away from them.  i.e. backstabbing victims!
static void BotFeigningEnemyCheck(bot_t* pBot)
{
    pBot->lastEnemySentryGun = NULL; // ignore all sentry guns, the bot is feigning
    pBot->visAllyCount = 1;          // assume the bot can't see much because it has to stay still

    // see if there are any enemies near enough to this location
    edict_t* pPlayer;
    int player_team;
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if(pPlayer && !pPlayer->free && pPlayer != pBot->pEdict && IsAlive(pPlayer)) {
            // ignore humans in observer mode
            if(b_observer_mode && !(pPlayer->v.flags & FL_FAKECLIENT))
                continue;

            // ignore allied players
            player_team = UTIL_GetTeam(pPlayer);
            if(player_team > -1 &&
                (player_team == pBot->current_team || team_allies[pBot->current_team] & (1 << player_team)))
                continue;

            // is this enemy near and facing away from the bot?
            if(VectorsNearerThan(pPlayer->v.origin, pBot->pEdict->v.origin, 400.0f) &&
                !FInViewCone(pBot->pEdict->v.origin, pPlayer) && FVisible(pPlayer->v.origin, pBot->pEdict)) {
                // is this a new sighting?
                if(pBot->enemy.ptr != pPlayer)
                    pBot->enemy.f_firstSeen = pBot->f_think_time;

                pBot->enemy.ptr = pPlayer;
                pBot->enemy.f_lastSeen = pBot->f_think_time;
                pBot->enemy.lastLocation = pPlayer->v.origin;
                pBot->visEnemyCount = 1;
                return;
            }
        }
    }

    // no target found, clear the Spies knowledge of enemies
    pBot->enemy.ptr = NULL;
    pBot->visEnemyCount = 0;
}

// This function will first check that the bots current enemy is still a
// valid target for attacking.
// Then it will check if the bot can see a new(perhaps more important) target.
void BotEnemyCheck(bot_t* pBot)
{
    // let feigning spys target enemies in their own way
    if(pBot->pEdict->v.playerclass == TFC_CLASS_SPY && pBot->pEdict->v.deadflag == 5) {
        BotFeigningEnemyCheck(pBot);
        return;
    }

    // stupidity check... dont target yourself moron.
    if(pBot->enemy.ptr == pBot->pEdict)
        pBot->enemy.ptr = NULL;

    // handle sniper firing delays(when they have a target)
    if(pBot->f_snipe_time > pBot->f_think_time) {
        pBot->pEdict->v.button |= IN_ATTACK;
    }

    // test our last SG to see if its still valid
    // a crash could occur if using a pointer to a non-existant SG
    if(!FNullEnt(pBot->lastEnemySentryGun)) {
        // Is it a sentry gun or an NTF multigun?
        if((strcmp(STRING(pBot->lastEnemySentryGun->v.classname), "building_sentrygun") == 0 ||
               strncmp(STRING(pBot->lastEnemySentryGun->v.classname), "ntf_", 4) == 0) &&
            (BotTeamColorCheck(pBot->lastEnemySentryGun) != pBot->current_team)) {
            // get the bots to notice the destruction of this sentry gun
            if(!IsAlive(pBot->lastEnemySentryGun) && pBot->lastEnemySentryGun->v.origin != Vector(0, 0, 0)) {
                edict_t* deadSG = pBot->lastEnemySentryGun;

                int area = AreaInsideClosest(deadSG);
                if(area != -1) {
                    char msg[MAX_CHAT_LENGTH];
                    if(pBot->current_team == 0)
                        _snprintf(msg, MAX_CHAT_LENGTH, "Sentry Down %s", areas[area].namea);
                    else if(pBot->current_team == 1)
                        _snprintf(msg, MAX_CHAT_LENGTH, "Sentry Down %s", areas[area].nameb);
                    else if(pBot->current_team == 2)
                        _snprintf(msg, MAX_CHAT_LENGTH, "Sentry Down %s", areas[area].namec);
                    else if(pBot->current_team == 3)
                        _snprintf(msg, MAX_CHAT_LENGTH, "Sentry Down %s", areas[area].named);
                    msg[MAX_CHAT_LENGTH - 1] = '\0';

                    bool destruction_reported = FALSE;

                    for(int i = 0; i < 32; i++) {
                        if(bots[i].is_used && bots[i].lastEnemySentryGun == deadSG) {
                            // get one bot that saw the sentry get destroyed to report it
                            if(!destruction_reported && bots[i].current_team == pBot->current_team &&
                                !BufferContainsJobType(&bots[i], JOB_REPORT) &&
                                VectorsNearerThan(deadSG->v.origin, bots[i].pEdict->v.origin, 1400.0f) &&
                                FVisible(deadSG->v.origin + deadSG->v.view_ofs, bots[i].pEdict)) {
                                job_struct* newJob = InitialiseNewJob(&bots[i], JOB_REPORT);
                                if(newJob != NULL) {
                                    strncpy(newJob->message, msg, MAX_CHAT_LENGTH);
                                    newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
                                    SubmitNewJob(&bots[i], JOB_REPORT, newJob);

                                    destruction_reported = TRUE;
                                }
                            }

                            bots[i].lastEnemySentryGun = NULL;
                        }
                    }
                }
            }
        } else // sentry pointer is pointing at something which isn't a sentry gun
        {
            if(pBot->enemy.ptr == pBot->lastEnemySentryGun)
                pBot->enemy.ptr = NULL;
            pBot->lastEnemySentryGun = NULL;
        }
    } else {
        if(pBot->enemy.ptr == pBot->lastEnemySentryGun)
            pBot->enemy.ptr = NULL;
        pBot->lastEnemySentryGun = NULL;
    }

    // clear this, it's tested below
    pBot->enemy.seenWithFlag = FALSE;

    // if the bot has a current enemy, check if it's still valid
    if(pBot->enemy.ptr != NULL) {
        Vector vecEnd = pBot->enemy.ptr->v.origin + pBot->enemy.ptr->v.view_ofs;

        // anti friendly fire
        if(pBot->pEdict->v.playerclass != TFC_CLASS_MEDIC && pBot->pEdict->v.playerclass != TFC_CLASS_ENGINEER &&
            (pBot->pEdict->v.team == pBot->enemy.ptr->v.team ||
                BotTeamColorCheck(pBot->enemy.ptr) == pBot->current_team)) {
            if(pBot->enemy.ptr->v.playerclass != TFC_CLASS_MEDIC)
                pBot->enemy.ptr = NULL;
        } else if(pBot->enemy.ptr->v.flags & FL_KILLME) {
            // the enemies edict has been flagged for deletion
            // so null out the pointer to them
            pBot->enemy.ptr = NULL;
        }
        // is the enemy dead? assume bot killed it
        else if(!IsAlive(pBot->enemy.ptr) &&
            !(pBot->enemy.ptr->v.deadflag == 5 && random_long(0, (pBot->bot_skill + 1) * 1000) < 900)) {
            // the enemy is dead, jump for joy about 20% of the time
            if(random_long(1, 100) <= 20)
                pBot->pEdict->v.button |= IN_JUMP;
            if(random_long(1, 100) <= 20)
                BotSprayLogo(pBot->pEdict, TRUE);

            // don't have an enemy anymore so null out the pointer...
            pBot->enemy.ptr = NULL;
        } else if(!FInViewCone(vecEnd, pBot->pEdict) || !FVisible(vecEnd, pBot->pEdict)) {
            pBot->enemy.ptr = NULL; // forget the enemy
        } else                      // enemy is visible
        {
            // keep this up to date
            pBot->enemy.lastLocation = pBot->enemy.ptr->v.origin;

            // keep track of when the bot last saw an enemy
            pBot->enemy.f_lastSeen = pBot->f_think_time;

            // update the bots known distance from it's enemy
            pBot->enemy.f_seenDistance = (pBot->pEdict->v.origin - pBot->enemy.ptr->v.origin).Length();

            // check and remember if the bots enemy has a flag
            // so that we can avoid repeating this test elsewhere
            if(PlayerHasFlag(pBot->enemy.ptr))
                pBot->enemy.seenWithFlag = TRUE;

            // this code segment sets up a snipers sniper rifle firing delay
            if(pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER && pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE) {
                float timeVal = (pBot->bot_skill + 1) * 0.4;

                // if f_snipe_time is too high or too low reset it
                if((pBot->f_snipe_time + 0.3) < pBot->f_think_time ||
                    pBot->f_snipe_time > pBot->f_think_time + timeVal) {
                    // set pBot->f_snipe_time for a short delay
                    pBot->f_snipe_time = pBot->f_think_time + random_float(0.0, timeVal);

                    pBot->pEdict->v.button |= IN_ATTACK;
                    pBot->f_pause_time = pBot->f_snipe_time;
                }
            }
        }
    }

    // is the enemy near one of the bot's pipebombs?
    if(pBot->pEdict->v.playerclass == TFC_CLASS_DEMOMAN && pBot->enemy.ptr != NULL)
        BotPipeBombCheck(pBot);

    // optimization - don't check for new enemies too often
    if(pBot->f_enemy_check_time > pBot->f_think_time)
        return;
    else
        pBot->f_enemy_check_time = pBot->f_think_time + 0.2f;

    // now scan the visible area around the bot for new enemies
    edict_t* new_enemy = BotFindEnemy(pBot);
    if(new_enemy != NULL && new_enemy != pBot->enemy.ptr) {
        pBot->enemy.ptr = new_enemy;
        pBot->enemy.f_firstSeen = pBot->f_think_time;
        pBot->enemy.f_lastSeen = pBot->f_think_time;
        pBot->enemy.lastLocation = new_enemy->v.origin;
    }
}

// This function is responsible for checking if the bot can see a new
// target worth attacking.
static edict_t* BotFindEnemy(bot_t* pBot)
{
    edict_t* pEdict = pBot->pEdict;

    int i;
    bool return_null = FALSE;
    edict_t* pNewEnemy = NULL;
    edict_t* pent;
    Vector vecEnd;
    float nearestDistance;

    // some basic TFC stuff
    if(mod_id == TFC_DLL) {
        // if bot is a disguised spy consider not targetting anyone
        if(pEdict->v.playerclass == TFC_CLASS_SPY && pBot->enemy.ptr == NULL &&
            pBot->disguise_state == DISGUISE_COMPLETE && (pBot->f_injured_time + 0.5f) < pBot->f_think_time)
            return_null = TRUE;

        // if injured whilst carrying a flag always shoot back
        if((pBot->f_injured_time + 0.3f) > pBot->f_think_time && pBot->bot_has_flag)
            return_null = FALSE;
    }

    // if the bot currently has an enemy
    if(pBot->enemy.ptr != NULL) {
        vecEnd = pBot->enemy.ptr->v.origin + pBot->enemy.ptr->v.view_ofs;

        if(FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
            // check for closer enemy, or enemy with flag here!
            // and sentry guns!!!!!!
            pent = NULL;
            nearestDistance = 2000.0;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_sentrygun")) != NULL && (!FNullEnt(pent))) {
                int sentry_team = BotTeamColorCheck(pent);

                // don't target your own team's sentry guns...
                // don't target allied sentry guns either...
                if(pBot->current_team == sentry_team || team_allies[pBot->current_team] & (1 << sentry_team))
                    continue;

                vecEnd = pent->v.origin + pent->v.view_ofs;

                // is this the closest visible sentry gun?
                float distance = (pent->v.origin - pEdict->v.origin).Length();
                if(distance < nearestDistance && FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
                    nearestDistance = distance;
                    pNewEnemy = pent;

                    return_null = FALSE;
                    BotSGSpotted(pBot, pent);
                }
            }

            // This checks for uncaptured multiguns.
            pent = NULL;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "ntf_multigun")) != NULL && (!FNullEnt(pent))) {
                if((pent->v.flags & FL_KILLME) == FL_KILLME)
                    continue;
                int sentry_team = pent->v.team - 1;
                // don't target friendly sentry guns...
                if(pBot->current_team == sentry_team || team_allies[pBot->current_team] & (1 << sentry_team))
                    continue;

                // ntf_capture_mg 1 = ignore, we can cap it
                char* cvar_ntf_capture_mg = (char*)CVAR_GET_STRING("ntf_capture_mg");

                if(strcmp(cvar_ntf_capture_mg, "1") == 0)
                    continue;

                // is this the closest visible sentry gun?
                vecEnd = pent->v.origin + pent->v.view_ofs;
                float distance = (pent->v.origin - pEdict->v.origin).Length();
                if(distance < nearestDistance && FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
                    nearestDistance = distance;
                    pNewEnemy = pent;

                    return_null = FALSE;
                    BotSGSpotted(pBot, pent);
                }
            }

            // This function loops through the multiguns
            // looking for a better target.
            BotCheckForMultiguns(pBot, nearestDistance, pNewEnemy, return_null);

            if(pNewEnemy)
                return pNewEnemy;

            // only return enemy we got if they're close..
            // and we're not sniping
            // and they're not the player we're tracking
            if(pBot->enemy.ptr != NULL) {
                // put random in, so hopefully cpu usage is a bit less
                if(pBot->f_snipe_time < pBot->f_think_time &&
                    (VectorsNearerThan(pBot->enemy.ptr->v.origin, pEdict->v.origin, 250.0f) ||
                        random_long(1, 1000) < 700))
                    return (pBot->enemy.ptr);
            } else
                return NULL;
        } else {
            // if the bot can't see it's targetted Sentry Gun fuggedaboutit
            if(pBot->lastEnemySentryGun != NULL && pBot->enemy.ptr == pBot->lastEnemySentryGun &&
                !FNullEnt(pBot->lastEnemySentryGun))
                pBot->enemy.ptr = NULL;

            pBot->f_shoot_time = pBot->f_think_time + 1.0f; // normally +2
        }
    }
    pNewEnemy = NULL;
    nearestDistance = 1000.0;

    if(mod_id == TFC_DLL) {
        // get medics and engineers to heal/repair teammates
        if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC ||
            (pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER &&
                pBot->m_rgAmmo[weapon_defs[TF_WEAPON_SPANNER].iAmmo1] > 90)) {
            nearestDistance = 1000.0;
            edict_t* pPlayer;
            int player_team;

            // search the world for players...
            for(i = 1; i <= gpGlobals->maxClients; i++) {
                pPlayer = INDEXENT(i);

                // skip invalid players and skip self (i.e. this bot)
                if(pPlayer && !pPlayer->free && pPlayer != pEdict) {
                    // skip this player if they're not alive
                    if(!IsAlive(pPlayer))
                        continue;

                    // skip human players in observer mode
                    if(b_observer_mode && !(pPlayer->v.flags & FL_FAKECLIENT))
                        continue;

                    player_team = UTIL_GetTeamColor(pPlayer);

                    // ignore all enemies...
                    if((pBot->current_team != player_team) && !(team_allies[pBot->current_team] & (1 << player_team)))
                        continue;

                    // check if the player needs to be healed
                    if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC &&
                        (pPlayer->v.health / pPlayer->v.max_health) > 0.80 &&
                        !PlayerIsInfected(pPlayer)) // scores a point, even to selfish medics
                        continue;                   // health greater than 70% so ignore

                    // check if the player needs to be armored...
                    if(pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER &&
                        (PlayerIsInfected(pPlayer) || PlayerArmorPercent(pPlayer) > 50))
                        continue; // armor greater than 50% so ignore

                    // see if bot can see the player...
                    float distance = (pPlayer->v.origin - pEdict->v.origin).Length();
                    vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;
                    if(distance < nearestDistance && FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
                        nearestDistance = distance;

                        // set up a job to handle the healing/repairing
                        job_struct* newJob = InitialiseNewJob(pBot, JOB_BUFF_ALLY);
                        if(newJob != NULL) {
                            newJob->player = pPlayer;
                            newJob->origin = pPlayer->v.origin; // remember where the player was seen
                            SubmitNewJob(pBot, JOB_BUFF_ALLY, newJob);
                        }
                        break;
                    }
                }
            }
        }

        if(!pNewEnemy) {
            // check for sniper laser dots
            pent = NULL;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "laser_spot")) != NULL && (!FNullEnt(pent))) {
                // ignore your own sniper spot
                if(pent->v.owner == pEdict)
                    continue;

                int sniper_team = UTIL_GetTeam(pent->v.owner);

                // ignore sniper spots from your team
                // and ignore sniper spots from your allies
                if(sniper_team == pBot->current_team || team_allies[pBot->current_team] & (1 << sniper_team))
                    continue;

                // ok... check distance to sniper spot and see if its nearish
                // to the bot... if it is and they can see the sniper...
                // make them the enemy!  not very human like..but fun!
                if(VectorsNearerThan(pent->v.origin, pEdict->v.origin, 500.0f)) {
                    vecEnd = pent->v.owner->v.origin + pent->v.owner->v.view_ofs;
                    if(FVisible(vecEnd, pEdict)) {
                        // DrEvil
                        // Lets add some error into this. annoying when the
                        // bots always snapshot as soon as they see your dot.
                        if(pBot->pEdict->v.playerclass == TFC_CLASS_CIVILIAN ||
                            (pBot->bot_skill * 10 + random_long(0, 50)) < 60)
                            pNewEnemy = pent->v.owner;
                    }
                }
            }

            // check for sentry guns
            pent = NULL;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "building_sentrygun")) != NULL && (!FNullEnt(pent))) {
                int sentry_team = BotTeamColorCheck(pent);

                // don't target your own team's sentry guns...
                // don't target allied sentry guns either...
                vecEnd = pent->v.origin + pent->v.view_ofs; // + Vector(0,0,16);
                if(pBot->current_team == sentry_team || team_allies[pBot->current_team] & (1 << sentry_team)) {
                    if(VectorsNearerThan(pent->v.origin, pEdict->v.origin, 300.0f) &&
                        pEdict->v.playerclass != TFC_CLASS_ENGINEER && FInViewCone(vecEnd, pEdict) &&
                        FVisible(vecEnd, pEdict)) {
                        // ntf_feature_antigren
                        char* cvar_ntf_feature_antigren = (char*)CVAR_GET_STRING("ntf_feature_antigren");
                        if(pEdict->v.playerclass == TFC_CLASS_MEDIC && strcmp(cvar_ntf_feature_antigren, "1") == 0) {
                            FakeClientCommand(pEdict, "_special2", NULL, NULL);
                        }

                        // discard uneeded ammo when near friendly SG's
                        FakeClientCommand(pEdict, "discard", NULL, NULL);
                        BlacklistJob(pBot, JOB_SPOT_STIMULUS, 2.0); // ignore the pack as it drops
                        if(pBot->ammoStatus != AMMO_UNNEEDED)
                            BlacklistJob(pBot, JOB_PICKUP_ITEM, 2.0); // don't pick up your pack
                    }
                    continue;
                }

                // is this the closest visible sentry gun?
                const float distance = (pent->v.origin - pEdict->v.origin).Length();
                if(distance < nearestDistance && FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
                    nearestDistance = distance;
                    pNewEnemy = pent;
                    return_null = FALSE;

                    BotSGSpotted(pBot, pent);
                }
            }

            // neotf guns
            pent = NULL;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "ntf_multigun")) != NULL && (!FNullEnt(pent))) {
                if((pent->v.flags & FL_KILLME) == FL_KILLME)
                    continue;

                int sentry_team = pent->v.team - 1;

                // don't target friendly sentry guns...
                if(pBot->current_team == sentry_team || team_allies[pBot->current_team] & (1 << sentry_team))
                    continue;

                // ntf_capture_mg 1 = ignore, we can cap it
                char* cvar_ntf_capture_mg = (char*)CVAR_GET_STRING("ntf_capture_mg");
                if(strcmp(cvar_ntf_capture_mg, "1") == 0)
                    continue;

                // is this the closest visible sentry gun?
                const float distance = (pent->v.origin - pEdict->v.origin).Length();
                vecEnd = pent->v.origin + pent->v.view_ofs;
                if(distance < nearestDistance && FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
                    nearestDistance = distance;
                    pNewEnemy = pent;
                    return_null = FALSE;
                }
            }
            // This function loops through the multiguns
            // looking for a better target.
            BotCheckForMultiguns(pBot, nearestDistance, pNewEnemy, return_null);
        }
    } // end tfc mod if

    if(!pNewEnemy) {
        edict_t* pPlayer;
        int player_team;
        bool player_is_ally;

        pBot->visEnemyCount = 0; // reset this, so that it can be recalculated
        pBot->visAllyCount = 1;  // reset this, so that it can be recalculated

        nearestDistance = 3000;

        if(!checked_teamplay) // check for team play...
            BotCheckTeamplay();

        // track whether or not the bot is willing to escort an ally
        bool canEscort = FALSE;
        if(!pBot->bot_has_flag && !BufferContainsJobType(pBot, JOB_ESCORT_ALLY))
            canEscort = TRUE;

        // search the world for players...
        for(i = 1; i <= gpGlobals->maxClients; i++) {
            pPlayer = INDEXENT(i);

            // skip invalid players and skip self (i.e. this bot)
            if((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict)) {
                // skip this player if not alive (i.e. dead or dying)
                if(!IsAlive(pPlayer) &&
                    !(pPlayer->v.deadflag == 5 // a feign check
                        && random_long(0, (pBot->bot_skill + 1) * 1000) < 900))
                    continue;

                // skip human players in observer mode
                if(b_observer_mode && !(pPlayer->v.flags & FL_FAKECLIENT))
                    continue;

                player_is_ally = FALSE;

                // is team play enabled?
                if(is_team_play > 0.0) {
                    player_team = UTIL_GetTeam(pPlayer);

                    // don't target your teammates...
                    if(pBot->current_team == player_team)
                        player_is_ally = TRUE;

                    if(mod_id == TFC_DLL) {
                        // don't target your allies either...
                        if(team_allies[pBot->current_team] & (1 << player_team))
                            player_is_ally = TRUE;

                        // so disguised spys wont attack other disguised spys
                        if(pEdict->v.playerclass == TFC_CLASS_SPY && (pPlayer->v.playerclass == TFC_CLASS_SPY) &&
                            pBot->current_team == UTIL_GetTeamColor(pPlayer))
                            continue;
                    }
                }

                // check for allied players
                if(player_is_ally) {
                    vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;
                    if(VectorsNearerThan(pPlayer->v.origin, pEdict->v.origin, 1000.0f) && FVisible(vecEnd, pEdict)) {
                        // keep tags on the allies the bot can see
                        ++pBot->visAllyCount;
                        pBot->lastAllyVector = pPlayer->v.origin;
                        pBot->f_lastAllySeenTime = pBot->f_think_time;

                        // seen a friendly Medic?
                        if(pPlayer->v.playerclass == TFC_CLASS_MEDIC)
                            pBot->f_alliedMedicSeenTime = pBot->f_think_time;

                        // try and set up an escort job if the ally has a flag
                        if(canEscort && PlayerHasFlag(pPlayer)) {
                            job_struct* newJob = InitialiseNewJob(pBot, JOB_ESCORT_ALLY);
                            if(newJob != NULL) {
                                newJob->player = pPlayer;
                                newJob->origin = pPlayer->v.origin; // remember where
                                if(SubmitNewJob(pBot, JOB_ESCORT_ALLY, newJob) == TRUE)
                                    canEscort = FALSE;
                            }
                        }
                    }

                    continue;
                }

                vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

                // see if the bot can see the player...
                if(FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict)) {
                    const float distance = (pPlayer->v.origin - pEdict->v.origin).Length();

                    // count the number of visible enemies nearby
                    if(distance < 1200.0f)
                        ++pBot->visEnemyCount;

                    if(distance < nearestDistance) {
                        nearestDistance = distance;
                        pNewEnemy = pPlayer;
                    }
                }
            }
        }
    }

    if(pNewEnemy && mod_id == TFC_DLL) {
        const float distance = (pNewEnemy->v.origin - pEdict->v.origin).Length();
        vecEnd = pEdict->v.origin + pEdict->v.view_ofs;

        // if the enemy can't see this bot and this bot is a Spy
        if(pBot->pEdict->v.playerclass == TFC_CLASS_SPY && pBot->bot_real_health > 25 && !pBot->enemy.ptr &&
            pBot->disguise_state == DISGUISE_COMPLETE && distance < 400.0 &&
            !(FInViewCone(vecEnd, pNewEnemy) && FVisible(vecEnd, pNewEnemy))) {
            // backstab routine...might work :)
            pBot->strafe_mod = STRAFE_MOD_STAB;
            return_null = FALSE;
        }
        // turn off backstab routine if too far away
        else if(pBot->pEdict->v.playerclass == TFC_CLASS_SPY && distance >= 400.0)
            pBot->strafe_mod = STRAFE_MOD_NORMAL;

        // return null(if return_null == true)
        // only if the enemy hasn't got your flag
        if(return_null && pNewEnemy != pEdict) {
            bool enemy_has_flag = FALSE;

            // is the enemy carrying the flag/card/ball ?
            edict_t* pent = NULL;
            while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "item_tfgoal")) != NULL && !FNullEnt(pent)) {
                if(pent->v.owner == pNewEnemy) {
                    enemy_has_flag = TRUE;
                    break; // break out of while loop
                }
            }

            if(enemy_has_flag == FALSE && (pBot->enemy.f_lastSeen + 2.0) <= pBot->f_think_time)
                return pBot->enemy.ptr; // if the enemy doesn't have the flag
        }
    }

    // check if the bot should continue to target a disguised/feigning Spy
    if(return_null == FALSE && BotSpyDetectCheck(pBot, pNewEnemy) == FALSE)
        return (pBot->enemy.ptr);

    if(pNewEnemy) {
        // face the new enemy
        Vector v_enemy = pNewEnemy->v.origin - pEdict->v.origin;
        Vector bot_angles = UTIL_VecToAngles(v_enemy);

        pEdict->v.ideal_yaw = bot_angles.y;
        pEdict->v.idealpitch = bot_angles.x;

        if(pEdict->v.playerclass == TFC_CLASS_SNIPER && pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE &&
            pBot->bot_skill > 0 && mod_id == TFC_DLL) {
            // BotChangeYaw(pBot->pEdict,1);
        } else
            BotFixIdealYaw(pEdict);
    }

    // has the bot NOT seen an enemy for a few seconds? (time to reload)
    if((pBot->enemy.f_lastSeen + 0.5f) <= pBot->f_think_time && (pBot->enemy.f_lastSeen + 1.5f) > pBot->f_think_time) {
        // press reload button
        pEdict->v.button |= IN_RELOAD;

        return (pNewEnemy);
    }

    if(!pNewEnemy) {
        pEdict->v.button |= IN_RELOAD; // press reload button

        // if not a disguised spy return the bots current enemy
        if(!(return_null && pEdict->v.playerclass == TFC_CLASS_SPY && pBot->disguise_state == DISGUISE_COMPLETE))
            return pBot->enemy.ptr;
    }

    return (pNewEnemy);
}

// You can call this function when the bot spots a disguised or feigning Spy.
// Spies will be given a random amount of seconds before they are attacked.
// It returns true if the Spy should be attacked, false otherwise.
static bool BotSpyDetectCheck(bot_t* pBot, edict_t* pNewEnemy)
{
    // the bot has encountered a new enemy
    if(pNewEnemy != NULL && pNewEnemy != pBot->suspectedSpy) {
        // if the enemy is not disguised/feigning forget the last Spy
        if(pBot->current_team != UTIL_GetTeamColor(pNewEnemy) && pNewEnemy->v.deadflag != 5) {
            pBot->suspectedSpy = NULL;
            pBot->f_suspectSpyTime = 0;
            return TRUE;
        }

        const float distance = (pNewEnemy->v.origin - pBot->pEdict->v.origin).Length();

        // only consider disguised/feigning Spies who are near enough
        if(distance < 1200.0f) {
            // a cool laser effect to show who has just spotted a Spy
            // (for debugging purposes)
            //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
            //		pNewEnemy->v.origin, 10, 2, 250, 50, 250, 200, 10);

            pBot->suspectedSpy = pNewEnemy;

            // moderate the bots suspicion based on what role it's filling
            if(pBot->mission == ROLE_DEFENDER) {
                pBot->f_suspectSpyTime = (pBot->f_think_time + random_float(2.0f, 5.0f)) + (pBot->bot_skill * 0.5f);

                // maybe track the spy about for a bit whilst suspicious
                if(pBot->bot_class != TFC_CLASS_CIVILIAN && random_long(0, 1000) < 250) {
                    job_struct* newJob = InitialiseNewJob(pBot, JOB_PURSUE_ENEMY);
                    if(newJob != NULL) {
                        newJob->player = pBot->suspectedSpy;
                        newJob->origin = pBot->suspectedSpy->v.origin; // remember where
                        SubmitNewJob(pBot, JOB_PURSUE_ENEMY, newJob);
                    }

                    pBot->f_suspectSpyTime += 2.0f;
                }
            } else // attackers
            {
                pBot->f_suspectSpyTime = (pBot->f_think_time + random_float(2.0f, 5.0f)) + (pBot->bot_skill * 0.5f);

                // maybe track the spy about for a bit whilst suspicious
                if(pBot->bot_class != TFC_CLASS_CIVILIAN && random_long(0, 1000) < 100) {
                    job_struct* newJob = InitialiseNewJob(pBot, JOB_PURSUE_ENEMY);
                    if(newJob != NULL) {
                        newJob->player = pBot->suspectedSpy;
                        newJob->origin = pBot->suspectedSpy->v.origin; // remember where
                        SubmitNewJob(pBot, JOB_PURSUE_ENEMY, newJob);
                    }

                    pBot->f_suspectSpyTime += 2.0f;
                }
            }
        }

        return FALSE;
    }

    // the bot sees nobody
    if(pNewEnemy == NULL) {
        // forget the last Spy if the bot hasn't seen him for a while
        if((pBot->f_suspectSpyTime + 5.0f) < pBot->f_think_time)
            pBot->suspectedSpy = NULL;

        return TRUE;
    }
    // the bot has decided to target it's suspected Spy
    else if((pNewEnemy == pBot->suspectedSpy && pBot->f_suspectSpyTime < pBot->f_think_time) ||
        (pBot->current_team != UTIL_GetTeamColor(pNewEnemy) && pNewEnemy->v.deadflag != 5)) {
        // keep the memory of this experience fresh
        pBot->f_suspectSpyTime = pBot->f_think_time - 0.5f;

        return TRUE;
    }

    return FALSE;
}

// BotSGSpotted - This function handles the storing, and communicating
// of sentry/mg positions to teammates.
// When a bot see's a sentry, this function is called.
// It will be up to this function to determine if he already knows about it.
// If none of the bots on the team know about it, he will communicate it
// to the team using area names if available.
// Either way, the location is transferred to other bots memory if
// a) They dont have a sg in memory b) They are better than skill 5
static void BotSGSpotted(bot_t* pBot, edict_t* sg)
{
    // Double check this isn't our own teams SG.
    if(BotTeamColorCheck(sg) == pBot->current_team)
        return;

    // Always set as our last SG
    pBot->lastEnemySentryGun = sg;

    // allowed to report this now?
    // also check if bot is smart enough
    if(offensive_chatter == FALSE || pBot->bot_skill > 3)
        return;

    // abort if the bot can't report something right now
    job_struct* newJob = InitialiseNewJob(pBot, JOB_REPORT);
    if(newJob == NULL)
        return;

    // Communicate!
    const int area = AreaInsideClosest(sg);
    if(area != -1) {
        switch(pBot->current_team) {
        case 0:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry Spotted %s", areas[area].namea);
            break;
        case 1:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry Spotted %s", areas[area].nameb);
            break;
        case 2:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry Spotted %s", areas[area].namec);
            break;
        case 3:
            _snprintf(newJob->message, MAX_CHAT_LENGTH, "Sentry Spotted %s", areas[area].named);
            break;
        }

        newJob->message[MAX_CHAT_LENGTH - 1] = '\0';
        if(SubmitNewJob(pBot, JOB_REPORT, newJob) == TRUE) {
            // get the other bots to know about this sg if they dont know of one already.
            for(int i = 0; i < MAX_BOTS; i++) {
                if(bots[i].is_used && bots[i].current_team == pBot->current_team &&
                    bots[i].lastEnemySentryGun == NULL && bots[i].bot_skill < 3) // skill 5's are too dumb
                {
                    // everyone else has 1 in 10 chance of not "getting it"
                    if(random_long(1, 1000) < 901)
                        bots[i].lastEnemySentryGun = pBot->lastEnemySentryGun;
                }
            }
        }
    }
}

// This function is fairly intensive because it attempts to guess a waypoint
// that a player may be found at.
//  It returns the waypoint chosen or -1 on failure.
int BotGuessPlayerPosition(bot_t* const pBot, const Vector& r_playerOrigin)
{
    if(pBot->current_wp == -1)
        return -1;

    // get the waypoint nearest to the player
    int playerWP = WaypointFindNearest_S(r_playerOrigin, NULL, 700.0, pBot->current_team, W_FL_DELETED);
    if(playerWP == -1)
        return -1;

    int playerRouteDistance;
    int botRouteDistance;

    // try to pick waypoints more randomly
    int index = random_long(0, num_waypoints);

    // time to find the right sort of waypoint
    // we want one with a shorter route from the specified player than from the bot
    // i.e. a waypoint on the other side of the specified player from the bot
    for(int i = 0; i < num_waypoints; i++, index++) {
        if(index >= num_waypoints)
            index = 0; // wrap the search if necessary

        // skip deleted waypoints
        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
            continue;

        // Skip unavailable waypoints.
        if(!WaypointAvailable(index, pBot->current_team))
            continue;

        playerRouteDistance = WaypointDistanceFromTo(playerWP, index, pBot->current_team);

        if(playerRouteDistance > 100 && playerRouteDistance < 2000) {
            botRouteDistance = WaypointDistanceFromTo(pBot->current_wp, index, pBot->current_team);

            // ideally the resulting waypoint should be physically nearer to the
            // specified player than to the bot too
            // but this will do for now
            if(playerRouteDistance < botRouteDistance && botRouteDistance > 100)
                return index; // success!
        }
    }

    return -1;
}

// This function is useful for throwing grenades at an enemy who has just dissappeared
// behind a bit of scenery(e.g. a wall).  It will search for and return a waypoint that
// is near enough to the enemy to splash them, and visible to both the enemy and the bot.
// Returns -1 on failure to find a suitable waypoint.
int BotFindGrenadePoint(bot_t* const pBot, const Vector& r_vecOrigin)
{
    TraceResult tr;

    for(int index = 0; index < num_waypoints; index++) {
        // skip any deleted or aiming waypoints
        if(waypoints[index].flags & W_FL_DELETED || waypoints[index].flags & W_FL_AIMING)
            continue;

        // if the waypoint is near enough to the enemy check for visibility
        if(VectorsNearerThan(waypoints[index].origin, r_vecOrigin, 500.0)) {
            UTIL_TraceLine(r_vecOrigin, waypoints[index].origin, ignore_monsters, NULL, &tr);

            // debug stuff
            //	WaypointDrawBeam(INDEXENT(1), r_vecOrigin,
            //		waypoints[index].origin, 10, 2, 250, 50, 50, 200, 10);

            // is this waypoint visible to the enemy?
            if(tr.flFraction >= 1.0) {
                UTIL_TraceLine(pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs, waypoints[index].origin,
                    ignore_monsters, NULL, &tr);

                // debug stuff
                //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin,
                //		waypoints[index].origin, 10, 2, 250, 50, 50, 200, 10);

                // is this waypoint visible to the bot also?
                // if so we've found a suitable waypoint for throwing at
                if(tr.flFraction >= 1.0) {
                    //	UTIL_BotLogPrintf("BotFindGrenadePoint success\n");
                    return index;
                }
            }
        }
    }

    //	UTIL_BotLogPrintf("BotFindGrenadePoint failure\n");
    return -1; // oops, failure!
}

// This function is used to trace a line in front of the bot a couple
// of meters or so, to see if there is an obstacle blocking the bots
// next shot.  It returns TRUE if there is.
static bool BotClearShotCheck(bot_t* pBot)
{
    // Return false if this function returned false less than a few seconds
    // ago.  This is done to stop the bot constantly swapping back and
    // forth between weapons in complex areas.
    if(pBot->f_safeWeaponTime > pBot->f_think_time)
        return FALSE;

    Vector endpoint;
    TraceResult tr;

    UTIL_MakeVectors(pBot->pEdict->v.v_angle);

    // trace a line out directly in front of the bot (it would probably
    // help if this matched the minimum safe range of the RPG launcher)
    endpoint = pBot->pEdict->v.origin + gpGlobals->v_forward * 180;

    // un-comment the WaypointDrawBeam call below if you want to see this
    // function in action
    //	WaypointDrawBeam(INDEXENT(1), pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
    //		endpoint, 10, 2, 250, 150, 150, 200, 10);

    UTIL_TraceLine(pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs, endpoint, dont_ignore_monsters,
        pBot->pEdict->v.pContainingEntity, &tr);

    // report FALSE if the trace has hit something, and remember when
    // it happened
    if(tr.flFraction < 1.0) {
        // don't swap back to this weapon for a short period of time
        pBot->f_safeWeaponTime = pBot->f_think_time + 2.0f;
        return FALSE;
    }

    return TRUE; // the shot is clear
}

// This function initiates the process of firing the bots weapon at an enemy.
void BotShootAtEnemy(bot_t* pBot)
{
    // aim for the head and/or body
    Vector v_enemy = BotBodyTarget(pBot->enemy.ptr, pBot) - GetGunPosition(pBot->pEdict);

    // how far away is the enemy scum?
    const float f_distance = v_enemy.Length();

    Vector vv = UTIL_VecToAngles(v_enemy);
    pBot->idle_angle = vv.y;
    pBot->pEdict->v.idealpitch = vv.x;
    pBot->pEdict->v.ideal_yaw = pBot->idle_angle;
    v_enemy.z = 0; // ignore z component (up & down)

    // if the bot is concussed or burning etc. then mess with it's view angles
    // even if that means running into walls (for added realism)
    if(pBot->disturbedViewAmount > 0) {
        // periodically update the bots disturbed view
        if(pBot->f_periodicAlert1 < pBot->f_think_time) {
            pBot->f_disturbedViewYaw = random_float(-pBot->disturbedViewAmount, pBot->disturbedViewAmount);

            pBot->f_disturbedViewPitch = random_float(-pBot->disturbedViewAmount, pBot->disturbedViewAmount);
        }

        pBot->pEdict->v.ideal_yaw += pBot->f_disturbedViewYaw;
        pBot->pEdict->v.idealpitch += pBot->f_disturbedViewPitch;
        BotFixIdealYaw(pBot->pEdict);
        BotFixIdealPitch(pBot->pEdict);
        // UTIL_HostSay(pEdict, 0, "I'm concussed");
    }

    // if the bot is a disguised Spy targetting a Sentry Gun
    // and is near enough to throw frag grenades at it
    if(bot_use_grenades && pBot->pEdict->v.playerclass == TFC_CLASS_SPY &&
        pBot->enemy.ptr == pBot->lastEnemySentryGun && pBot->disguise_state == DISGUISE_COMPLETE &&
        !FNullEnt(pBot->lastEnemySentryGun) && pBot->grenades[PrimaryGrenade] > 0 && f_distance < 1200.0f) {
        // don't shoot until the Spy has no frag grenades left
        pBot->f_shoot_time = pBot->f_think_time + 3.0f;

        //	char msg[80];
        //	sprintf(msg, "<Attacking SG, nades left: %d>", (int)pBot->grenades[0]);
        //	UTIL_HostSay(pBot->pEdict, 0, msg);  //DebugMessageOfDoom!
    }

    // if the bots aim is currently too bad, postpone pulling the trigger
    if(f_distance > 200.0f) {
        const float aim_diff = BotViewAngleDiff(pBot->enemy.ptr->v.origin, pBot->pEdict);
        if(aim_diff < 0.95f) {
            //	char msg[80]; //DebugMessageOfDoom!
            //	sprintf(msg, "<my aim diff: %f>", aim_diff);
            //	UTIL_HostSay(pBot->pEdict, 0, msg);
            pBot->f_shoot_time = pBot->f_think_time + 0.08f;
        }
    }

    // return if the spy isn't in range to backstab... muwhahaha!
    if(pBot->strafe_mod == STRAFE_MOD_STAB && f_distance > 50.0)
        return;

    // if on the same team(follow) don't shoot!
    // is team play enabled?
    if(is_team_play > 0.0) {
        if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC || pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER) {
            // try to get medic to heal only.
            // is it time to shoot yet?
            if(pBot->f_shoot_time <= pBot->f_think_time) {
                pBot->f_move_speed = pBot->f_max_speed;
                pBot->strafe_mod = STRAFE_MOD_HEAL;
                // select the best weapon to use at this distance and fire...
                BotFireWeapon(v_enemy, pBot, 0);
                if(UTIL_GetTeamColor(pBot->enemy.ptr) == pBot->current_team)
                    return;
            }
        }
        const int player_team = UTIL_GetTeam(pBot->enemy.ptr);

        // don't target your teammates.
        // and don't target your allies either...
        if(pBot->current_team == player_team || team_allies[pBot->current_team] & (1 << player_team)) {
            pBot->strafe_mod = STRAFE_MOD_HEAL;
            return;
        }
    }

    // is it time to shoot yet?
    if(pBot->f_shoot_time <= pBot->f_think_time) {
        // select the best weapon to use at this distance and fire...
        BotFireWeapon(v_enemy, pBot, 0);
    }
}

// This function handles the aiming of whatever weapon a bot is holding.
static Vector BotBodyTarget(edict_t* pBotEnemy, bot_t* pBot)
{
    Vector target;
    const float f_distance = pBot->enemy.f_seenDistance;
    float f_scale; // for scaling accuracy based on distance
    float d_x, d_y, d_z;

    if(f_distance > 1000.0)
        f_scale = 1.0;
    else
        f_scale = f_distance / 1000.0;

    // aim at the enemies feet if the bot is using a rocket launcher
    // and the enemy is not much higher up than the bot
    if((pBot->current_weapon.iId == TF_WEAPON_RPG || pBot->current_weapon.iId == TF_WEAPON_IC) // pyro RPG
        && pBot->enemy.ptr->v.origin.z < (pBot->pEdict->v.origin.z + 35.0)) {
        target.x = pBotEnemy->v.origin.x;
        target.y = pBotEnemy->v.origin.y;
        target.z = pBotEnemy->v.absmin.z;
    } else {
        if(pBot->bot_skill < 2)
            target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs; // aim for the head
        else
            target = pBotEnemy->v.origin; // aim for the body
    }

    // time to reassess the bots inaccuracy?
    if(pBot->f_periodicAlert1 < pBot->f_think_time) {
        // is the bot sniping?
        if(mod_id == TFC_DLL && pBot->pEdict->v.playerclass == TFC_CLASS_SNIPER &&
            pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE) {
            // Make the bot less accurate based on it's targets speed.
            // ignore the bots own speed - it's sniping!
            const float enemy_velocity = pBot->enemy.ptr->v.velocity.Length();
            float aim_error = random_float(0.0, enemy_velocity) * 0.05;

            // Make the bot less accurate if the enemy was just seen
            if((pBot->enemy.f_firstSeen + 2.0) > pBot->f_think_time)
                aim_error += (pBot->bot_skill + 1) * random_float(5.0, 20.0);

            float aim_offset = bot_snipe_max_inaccuracy[pBot->bot_skill] + aim_error;
            switch(pBot->bot_skill) {
            case 0:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            case 1:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            case 2:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            case 3:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            default: // covers bot_skill 4 (and silences a compiler warning)
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            }
        } else // the bot is not sniping
        {
            // Make the bot less accurate based on it's own speed.
            const float my_velocity = pBot->pEdict->v.velocity.Length();
            float aim_error = random_float(0.0, my_velocity) * 0.05;

            // Make the bot less accurate based on it's targets speed.
            const float enemy_velocity = pBot->enemy.ptr->v.velocity.Length();
            aim_error += random_float(0.0, enemy_velocity) * 0.05;

            // Make the bot less accurate if the enemy was just seen
            if((pBot->enemy.f_firstSeen + 2.0) > pBot->f_think_time)
                aim_error += (pBot->bot_skill + 1) * random_float(5.0, 20.0);

            float aim_offset = bot_max_inaccuracy[pBot->bot_skill] + aim_error;
            switch(pBot->bot_skill) {
            case 0:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            case 1:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            case 2:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            case 3:
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            default: // covers bot_skill 4 (and silences a compiler warning)
                pBot->aimDrift.x = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.y = random_float(-aim_offset, aim_offset) * f_scale;
                pBot->aimDrift.z = random_float(-aim_offset, aim_offset) * f_scale;
                break;
            }
        }
    }

    // use aimDrift as the starting point for further aim calculations
    d_x = pBot->aimDrift.x;
    d_y = pBot->aimDrift.y;
    d_z = pBot->aimDrift.z;

    // make healers aim for the head
    if(mod_id == TFC_DLL &&
        (pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC || pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER) &&
        ((pBot->current_weapon.iId == TF_WEAPON_MEDIKIT) || (pBot->current_weapon.iId == TF_WEAPON_SPANNER)))
        target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;

    /*	float aim_tolerance;

            if(mod_id == TFC_DLL
                    && pBot->pEdict->v.playerclass == TFC_CLASS_HWGUY
                    && pBot->current_weapon.iId == TF_WEAPON_AC)
                    aim_tolerance = 25.0f + (static_cast<float>(pBot->bot_skill) * 5.0f);
            else aim_tolerance = 5.0f + (static_cast<float>(pBot->bot_skill) * 2.0f);

            // if the bots aim is pretty bad right now, make it cease firing
            // for a fraction of a second(wait for the aim to stabilize)
            if(pBot->f_shoot_time <= pBot->f_think_time
            //	&& random_long(0, 1000) > 200
                    && (d_x < -aim_tolerance || d_x > aim_tolerance
                            || d_y < -aim_tolerance || d_y > aim_tolerance) )
                    pBot->f_shoot_time = pBot->f_think_time + 0.1f;*/

    // projectile prediction targetting & grenade arcing
    int zDiff = static_cast<int>(pBot->enemy.ptr->v.origin.z - pBot->pEdict->v.origin.z);
    float dist2d = (pBotEnemy->v.origin - pBot->pEdict->v.origin).Length2D();

    // Add some error into the distance calculation based on skill level.
    // This should add some lead error.
    dist2d += random_float(-dist2d * 0.95f, dist2d * 0.95f);
    dist2d += static_cast<float>(random_long(-pBot->bot_skill, pBot->bot_skill) * 10);

    // lead for rocket and grenade launcher type weapons
    if(!pBot->tossNade && (pBot->current_weapon.iId == TF_WEAPON_RPG || pBot->current_weapon.iId == TF_WEAPON_IC ||
                              pBot->current_weapon.iId == TF_WEAPON_GL) ||
        pBot->current_weapon.iId == TF_WEAPON_PL) {
        BotLeadTarget(pBotEnemy, pBot, 725, d_x, d_y, d_z);

        // arc grenade/pipe launcher
        if(pBot->current_weapon.iId == TF_WEAPON_GL || pBot->current_weapon.iId == TF_WEAPON_PL) {
            if(zDiff > 100 || dist2d > 650)
                d_z += dist2d / 3;
        }
    }
    // Lead for grenades.
    else if(pBot->tossNade) {
        // Need to arc up more if they are high or far away.
        if(zDiff > 70 || dist2d > 500) {
            d_z += dist2d;
            // UTIL_HostSay(pBot->pEdict, 0, "arcing up");
        }
        pBot->tossNade = 0;
    }

    // if the enemy is a feigning spy - aim lower(soldiers aim lower anyway)
    if(pBotEnemy->v.deadflag == 5 && pBot->pEdict->v.playerclass != TFC_CLASS_SOLDIER) {
        d_z -= 32.0;
    }

    target = target + Vector(d_x, d_y, d_z);
    return target;
}

// specifing a weapon_choice allows you to choose the weapon the bot
// will use (assuming enough ammo exists for that weapon)
// BotFireWeapon will return TRUE if weapon was fired, FALSE otherwise
bool BotFireWeapon(Vector v_enemy, bot_t* pBot, int weapon_choice)
{
    bot_weapon_select_t* pSelect = NULL;
    bot_fire_delay_t* pDelay = NULL;
    int iId;

    edict_t* pEdict = pBot->pEdict;
    float f_distance = v_enemy.Length(); // how far away is the enemy?

    // NeoTF pyro stuff
    if(pBot->pEdict->v.playerclass == TFC_CLASS_PYRO && pBot->FreezeDelay < pBot->f_think_time && f_distance < 200.0 &&
        random_long(1, 10) > 2 + pBot->bot_skill) {
        pBot->FreezeDelay = pBot->f_think_time + 1.0;
        FakeClientCommand(pEdict, "freeze", "102", NULL);
    } else
        pBot->FreezeDelay = pBot->f_think_time + 0.2f;

    // nobody documented this bit of code ;-(
    if(pBot->current_weapon.iId == TF_WEAPON_RPG || pBot->current_weapon.iId == TF_WEAPON_IC ||
        pBot->current_weapon.iId == TF_WEAPON_GL) {
        double diff = pEdict->v.v_angle.y; //-pEdict->v.ideal_yaw;

        if(diff < -180)
            diff += 360;
        if(diff > 180)
            diff -= 360;

        diff -= pEdict->v.ideal_yaw;
        if(diff < -180)
            diff += 360;
        if(diff > 180)
            diff -= 360;

        double ang = (((32 * pBot->bot_skill) + 16) / f_distance);
        ang = tan(ang);
        ang = ang * 180;

        if(diff < 0)
            diff = -diff;

        /*char msg[512];
           sprintf(msg,"%f  %f", ang, diff);
           UTIL_HostSay(pEdict,0,msg);*/
        if(ang < diff) //|| (FInViewCone( v_enemy, pEdict ) && FVisible( v_enemy, pEdict )))
            return FALSE;
    }

    //////////////////
    int distance = static_cast<int>(f_distance);

    if(pEdict->v.playerclass == TFC_CLASS_SNIPER && mod_id == TFC_DLL &&
        (pEdict->v.waterlevel >= WL_WAIST_IN_WATER || pEdict->v.movetype == MOVETYPE_FLY) &&
        (!((pEdict->v.button & IN_ATTACK) == IN_ATTACK && pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE) ||
            pEdict->v.waterlevel >= WL_WAIST_IN_WATER)) {
        distance = 100; // use autorifle when running
    } else {
        if(pEdict->v.playerclass == TFC_CLASS_SNIPER && pBot->current_weapon.iId == TF_WEAPON_SNIPERRIFLE &&
            pBot->enemy.ptr != NULL && mod_id == TFC_DLL && distance > 300)
            return TRUE;
    }

    // demoman grenade launcher height restriction :D
    if(pEdict->v.playerclass == TFC_CLASS_DEMOMAN && mod_id == TFC_DLL && distance < 900) {
        if(pBot->enemy.ptr != NULL) {
            int z = static_cast<int>(pBot->enemy.ptr->v.origin.z - pEdict->v.origin.z);
            if(z > 0 && z < 300 && distance < 901)
                distance = static_cast<int>((pBot->enemy.ptr->v.origin - pEdict->v.origin).Length2D()) + (z * 3);
            else if(z > 300 && distance < 901)
                distance = 901;
        }
    }

    if(mod_id == TFC_DLL) {
        pSelect = &tfc_weapon_select[0];
        pDelay = &tfc_fire_delay[0];
    }

    if(pSelect) {
        int select_index;
        bool use_primary, use_secondary;
        int use_percent, primary_percent;

        // are we charging the primary fire?
        if(pBot->f_primary_charging > 0) {
            iId = pBot->charging_weapon_id;
            if(mod_id == TFC_DLL) {
                // don't move while using sniper rifle
                if(iId == TF_WEAPON_SNIPERRIFLE)
                    pBot->f_move_speed = 0;
            }

            // is it time to fire the charged weapon?
            if(pBot->f_primary_charging <= pBot->f_think_time) {
                // we DON'T set pEdict->v.button here to release the
                // fire button which will fire the charged weapon

                pBot->f_primary_charging = -1; // -1 means not charging
                // find the correct fire delay for this weapon
                select_index = 0;

                while((pSelect[select_index].iId) && (pSelect[select_index].iId != iId))
                    select_index++;

                // set next time to shoot
                float base_delay, min_delay, max_delay;

                base_delay = pDelay[select_index].primary_base_delay;
                min_delay = pDelay[select_index].primary_min_delay[pBot->bot_skill];
                max_delay = pDelay[select_index].primary_max_delay[pBot->bot_skill];

                // pBot->f_shoot_time = pBot->f_think_time + base_delay +
                //   random_float(min_delay, max_delay);
                return TRUE;
            } else {
                pEdict->v.button |= IN_ATTACK;           // charge the weapon
                pBot->f_shoot_time = pBot->f_think_time; // keep charging
                return TRUE;
            }
        }

        // are we charging the secondary fire?
        if(pBot->f_secondary_charging > 0) {
            iId = pBot->charging_weapon_id;
            // is it time to fire the charged weapon?
            if(pBot->f_secondary_charging <= pBot->f_think_time) {
                // we DON'T set pEdict->v.button here to release the
                // fire button which will fire the charged weapon

                pBot->f_secondary_charging = -1; // -1 means not charging
                // find the correct fire delay for this weapon
                select_index = 0;

                while((pSelect[select_index].iId) && (pSelect[select_index].iId != iId))
                    select_index++;

                // set next time to shoot
                float base_delay, min_delay, max_delay;

                base_delay = pDelay[select_index].secondary_base_delay;
                min_delay = pDelay[select_index].secondary_min_delay[pBot->bot_skill];
                max_delay = pDelay[select_index].secondary_max_delay[pBot->bot_skill];
                return TRUE;
            } else {
                pEdict->v.button |= IN_ATTACK2;          // charge the weapon
                pBot->f_shoot_time = pBot->f_think_time; // keep charging
                return TRUE;
            }
        }

        select_index = 0;
        use_primary = FALSE;
        use_secondary = FALSE;

        // loop through all the weapons until terminator is found...
        while(pSelect[select_index].iId && !use_primary && !use_secondary) {
            // was a weapon choice specified?
            // (and if so do they NOT match?)
            if((weapon_choice != 0) && (weapon_choice != pSelect[select_index].iId)) {
                select_index++; // skip to next weapon
                continue;
            }

            // is the bot NOT carrying this weapon?
            if(!(pBot->bot_weapons & (1 << pSelect[select_index].iId))) {
                select_index++; // skip to next weapon
                continue;
            }

            // is the bot NOT skilled enough to use this weapon?
            if((pBot->bot_skill + 1) > pSelect[select_index].skill_level) {
                select_index++; // skip to next weapon
                continue;
            }

            // is the bot underwater and does this weapon
            // NOT work under water?
            if((pEdict->v.waterlevel == WL_HEAD_IN_WATER) && !(pSelect[select_index].can_use_underwater)) {
                select_index++; // skip to next weapon
                continue;
            }

            use_percent = 0;

            // is use percent greater than weapon use percent?
            if(use_percent > pSelect[select_index].use_percent) {
                select_index++; // skip to next weapon
                continue;
            }

            /* check if the bots too close to an obstacle with an explosive
                    weapon such as the RPG launcher */
            if((pSelect[select_index].iId == TF_WEAPON_RPG
                   //|| pSelect[select_index].iId == TF_WEAPON_IC
                   || pSelect[select_index].iId == TF_WEAPON_GL) &&
                BotClearShotCheck(pBot) == false) {
                select_index++; // skip to next weapon
                continue;
            }

            iId = pSelect[select_index].iId;
            use_primary = use_secondary = FALSE;
            primary_percent = 0;

            // check if this weapon uses ammo and is running low
            if(pBot->m_rgAmmo[weapon_defs[iId].iAmmo1] < pSelect[select_index].min_primary_ammo)
                pBot->ammoStatus = AMMO_LOW;

            // is primary percent less than weapon primary percent AND
            // no ammo required for this weapon OR
            // enough ammo available to fire AND
            // the bot is far enough away to use primary fire AND
            // the bot is close enough to the enemy to use primary fire

            if((primary_percent <= pSelect[select_index].primary_fire_percent) &&
                ((weapon_defs[iId].iAmmo1 == -1) ||
                    (pBot->m_rgAmmo[weapon_defs[iId].iAmmo1] >= pSelect[select_index].min_primary_ammo)) &&
                (distance >= pSelect[select_index].primary_min_distance) &&
                (distance <= pSelect[select_index].primary_max_distance)) {
                use_primary = TRUE;
            }

            // otherwise see if there is enough secondary ammo AND
            // the bot is far enough away to use secondary fire AND
            // the bot is close enough to the enemy to use secondary fire
            else if(((weapon_defs[iId].iAmmo2 == -1) ||
                        (pBot->m_rgAmmo[weapon_defs[iId].iAmmo2] >= pSelect[select_index].min_secondary_ammo)) &&
                (distance >= pSelect[select_index].secondary_min_distance) &&
                (distance <= pSelect[select_index].secondary_max_distance)) {
                use_secondary = TRUE;
            }

            // see if there wasn't enough ammo to fire the weapon...
            if((use_primary == FALSE) && (use_secondary == FALSE)) {
                select_index++; // skip to next weapon
                continue;
            }

            // select this weapon if it isn't already selected
            if(pBot->current_weapon.iId != iId)
                UTIL_SelectItem(pEdict, pSelect[select_index].weapon_name);

            if(pDelay[select_index].iId != iId) {
                char msg[80];
                sprintf(msg, "fire_delay mismatch for weapon id=%d\n", iId);
                ALERT(at_console, msg);
                return FALSE;
            }

            if(mod_id == TFC_DLL) {
                if(iId == TF_WEAPON_SNIPERRIFLE) {
                    // don't move while using a sniper rifle
                    pBot->f_move_speed = 0;

                    // don't press attack key until velocity is < 50
                    if(pBot->pEdict->v.velocity.Length() > 50)
                        return FALSE;
                }

                if(pBot->pEdict->v.playerclass == TFC_CLASS_MEDIC ||
                    pBot->pEdict->v.playerclass == TFC_CLASS_ENGINEER) {
                    const int player_team = UTIL_GetTeam(pBot->enemy.ptr);

                    // only heal your teammates or allies...
                    if((pBot->current_team == player_team || team_allies[pBot->current_team] & (1 << player_team)) &&
                        (iId != TF_WEAPON_MEDIKIT && iId != TF_WEAPON_SPANNER)) {
                        pBot->strafe_mod = STRAFE_MOD_HEAL;
                        // return FALSE;  // don't "fire" unless weapon is medikit
                        use_primary = FALSE;
                    }
                }
            }

            if(use_primary) {
                pEdict->v.button |= IN_ATTACK; // use primary attack

                if(pSelect[select_index].primary_fire_charge) {
                    pBot->charging_weapon_id = iId;
                    // release primary fire after the appropriate delay...
                    pBot->f_primary_charging = pBot->f_think_time + pSelect[select_index].primary_charge_delay;
                    pBot->f_shoot_time = pBot->f_think_time; // keep charging
                } else {
                    // set next time to shoot
                    if(pSelect[select_index].primary_fire_hold)
                        pBot->f_shoot_time = pBot->f_think_time; // don't let button up
                    else {
                        /*	float base_delay = pDelay[select_index].primary_base_delay;
                                float min_delay = pDelay[select_index].primary_min_delay[pBot->bot_skill];
                                float max_delay = pDelay[select_index].primary_max_delay[pBot->bot_skill];

                                pBot->f_shoot_time = pBot->f_think_time + base_delay +
                                   random_float(min_delay, max_delay);*/
                        pBot->f_shoot_time = pBot->f_think_time + 0.05f;
                    }
                }
            } else // MUST be use_secondary...
            {
                pEdict->v.button |= IN_ATTACK2; // use secondary attack
                if(pSelect[select_index].secondary_fire_charge) {
                    pBot->charging_weapon_id = iId;

                    // release secondary fire after the appropriate delay...
                    pBot->f_secondary_charging = pBot->f_think_time + pSelect[select_index].secondary_charge_delay;
                    pBot->f_shoot_time = pBot->f_think_time; // keep charging
                } else {
                    // set next time to shoot
                    if(pSelect[select_index].secondary_fire_hold)
                        pBot->f_shoot_time = pBot->f_think_time; // don't let button up
                    else {
                        /*	float base_delay = pDelay[select_index].secondary_base_delay;
                                float min_delay = pDelay[select_index].secondary_min_delay[pBot->bot_skill];
                                float max_delay = pDelay[select_index].secondary_max_delay[pBot->bot_skill];
                                pBot->f_shoot_time = pBot->f_think_time + base_delay +
                                   random_float(min_delay, max_delay);*/
                        pBot->f_shoot_time = pBot->f_think_time + 0.05;
                    }
                }
            }
            return TRUE; // weapon was fired
        }
    }
    // didn't have any available weapons or ammo, return FALSE
    return FALSE;
}

// BotNadeHandler - This function handles all grenade related actions,
// except for concussion grenade primings when used for conc jumps.
// A call to this function would pass in the bot, the grenade type,
// and whether or not this call is intended to be timed or not.
// If there is already a grenade primed when a new call is made,
// the new call is ignored and the function exits after the evaluations
// on whether it is time to throw or not.
// If a call to throw a stationary grenade type(usually at sentry guns) is
// recieved, the bot will toss any priming grenades at the target immediately.
// This function also handles looking ahead to the next waypoint and
// checking whether or not the sentry they have in memory is viewable
// from there. If so they will anticipate contact and prime a grenade,
// and acquire the target early just before contact.
int BotNadeHandler(bot_t* pBot, bool timed, char newNadeType)
{
    // Lets try putting discard code in here. (dont let the engineer discard)
    if(pBot->f_discard_time < pBot->f_think_time && pBot->pEdict->v.playerclass != TFC_CLASS_ENGINEER) {
        FakeClientCommand(pBot->pEdict, "discard", NULL, NULL);
        BlacklistJob(pBot, JOB_SPOT_STIMULUS, 2.0); // ignore the pack as it drops
        if(pBot->ammoStatus != AMMO_UNNEEDED)
            BlacklistJob(pBot, JOB_PICKUP_ITEM, 2.0); // don't pick up your own pack

        if(pBot->mission == ROLE_ATTACKER)
            pBot->f_discard_time = pBot->f_think_time + 7.0f;
        else if(pBot->mission == ROLE_DEFENDER)
            pBot->f_discard_time = pBot->f_think_time + 15.0f;
        else
            pBot->f_discard_time = pBot->f_think_time + 15.0f;
    }

    if(bot_use_grenades == 0)
        return 0;

    int rtnValue = 0; // Our return value.
    edict_t* pEdict = pBot->pEdict;

    // Do we need to toss?  No for now...
    bool toss = FALSE;

    // Time left to detonation
    const float timeToDet = 4.0f - (pBot->f_think_time - pBot->primeTime);
    float zDiff = 0;

    // if the bot has no target to throw at try to find a place to
    // dispose of the live grenade(anti-suicide code)
    if(pBot->nadePrimed == TRUE && pBot->enemy.ptr == NULL && timeToDet <= 2.0f) {
        job_struct* newJob = InitialiseNewJob(pBot, JOB_BIN_GRENADE);
        if(newJob != NULL)
            SubmitNewJob(pBot, JOB_BIN_GRENADE, newJob);
    }

    // Go ahead and throw if its about to explode.
    if(pBot->nadePrimed) {
        const int lost_health_percent = 100 - PlayerHealthPercent(pEdict);
        float release_time = 0.8f;

        if(pBot->nadeType == GRENADE_MIRV)
            release_time = 1.0f;

        // factor in the bots state of health
        if(lost_health_percent > 20)
            release_time += static_cast<float>(lost_health_percent) * 0.01f;

        if(timeToDet <= release_time) {
            //	char msg[96];
            //	sprintf(msg, "Tossing. release_time:%f  lost_health_percent:%d",
            //		release_time, lost_health_percent);
            //	UTIL_HostSay(pBot->pEdict, 0, msg);//DebugMessageOfDoom!
            toss = TRUE;
        }
    }

    // Elevation check, try to throw up to ledges better.
    if(pBot->enemy.ptr) {
        zDiff = pBot->enemy.ptr->v.origin.z - pEdict->v.origin.z;
        if(zDiff > 100)
            timed = TRUE;
    }

    // If we're targetting an enemy with a held grenade.
    if(pBot->enemy.ptr != NULL && pBot->nadePrimed) {
        // Check the distance against where the bot thinks the nade
        // will blow if thrown now.
        const float distanceThrown = NADEVELOCITY * timeToDet;

        // Throw it if we got a good chance at landing good splash damage.
        if(fabs(distanceThrown - pBot->enemy.f_seenDistance) < 20.0f ||
            (distanceThrown - pBot->enemy.f_seenDistance) < -200.0f) {
            toss = TRUE;
            pBot->tossNade = 1;
            // UTIL_HostSay(pEdict, 0, "Die Bitch!!");
        }

        // If target is rapidly(ish) moving away from me, go ahead and throw.
        // Uses a timer delay between checks
        if(pBot->lastDistanceCheck <= pBot->f_think_time &&
            (pBot->lastDistance > (pBot->enemy.f_seenDistance + 375) && (pBot->enemy.f_seenDistance > 500))) {
            toss = TRUE;
            pBot->tossNade = 1;
            pBot->lastDistanceCheck = pBot->f_think_time + 1;
            // UTIL_HostSay(pEdict, 0, "Get over here!!");
        }

        // go ahead and toss if enemy is our team
        if(pBot->current_team == UTIL_GetTeam(pBot->enemy.ptr)) {
            toss = TRUE;
            pBot->tossNade = 2;
        }

        // Update last distance to target for comparison purposes
        // with above check.
        pBot->lastDistance = pBot->enemy.f_seenDistance;
    }

    // Time to throw?
    if(toss || (pEdict->v.waterlevel == WL_HEAD_IN_WATER)) {
        // Throw the mofos!
        FakeClientCommand(pEdict, "-gren1", "102", NULL);
        FakeClientCommand(pEdict, "-gren2", "101", NULL);
        rtnValue = 1;
        pBot->nadePrimed = FALSE;
        pBot->nadeType = 0;
    }

    /*	// this code allows bots to prime grenades early when
            // approaching a known Sentry Gun
            if(pBot->current_wp != pBot->lastWPIndexChecked)
            {
                    pBot->lastWPIndexChecked = pBot->current_wp;

                    // If we have a last seen sg, lets try checking the visibility
                    // from the next waypoint we are heading to.
                    if(pBot->lastEnemySentryGun != NULL
                            && !FNullEnt(pBot->lastEnemySentryGun)
                            && pBot->enemy.ptr != pBot->lastEnemySentryGun
                            && VectorsNearerThan(waypoints[pBot->current_wp].origin, pBot->lastEnemySentryGun->v.origin,
       800))
                    {
                            zDiff = pBot->lastEnemySentryGun->v.origin.z - waypoints[pBot->current_wp].origin.z;
                            rtnValue = static_cast<int>(zDiff);
                            const float dist
                                    = (pBot->pEdict->v.origin - waypoints[pBot->current_wp].origin).Length();

                            TraceResult result;
                            UTIL_TraceLine( waypoints[pBot->current_wp].origin,
                                    pBot->lastEnemySentryGun->v.origin,
                                    ignore_monsters,
                                    pEdict->v.pContainingEntity,
                                    &result );

                            // will we be close enough to attack?
                            if((result.flFraction >= 1.0)
                                    && (zDiff < 500.0f))
                            {
                                    if(dist < 600.0f)
                                    {
                                            // Lets see if the scout can use the NTF ring of shadows
                                            if(pEdict->v.playerclass == TFC_CLASS_SCOUT)
                                                    FakeClientCommand(pEdict, "_special2", "102", NULL);

                                            char buffer[150];
                                                    sprintf(buffer, "SG Early Prime P: %d S: %d",
                                                            pBot->grenades[PrimaryGrenade],
                                                            pBot->grenades[SecondaryGrenade]);
                                                    UTIL_HostSay(pEdict, 0, buffer);

                                            // prime a grenade in anticipation
                                            newNadeType = GRENADE_DAMAGE;
                                            timed = TRUE;
                                    }

                                    // try not to target the Sentry Gun until it's in view
                                    if(dist < 100.0f)
                                            pBot->enemy.ptr = pBot->lastEnemySentryGun;
                            }
                    }
            }*/

    // Get out if: nothing NEW to throw, or underwater
    if(!newNadeType || pEdict->v.waterlevel == WL_HEAD_IN_WATER)
        return rtnValue;

    if(pBot->enemy.ptr != NULL) {
        const int player_team = UTIL_GetTeam(pBot->enemy.ptr);

        // don't throw at teammates
        if(player_team == pBot->current_team)
            return rtnValue;

        // prime a grenade if we don't currently have one primed
        if(!pBot->nadePrimed && zDiff <= 400.0f && pBot->enemy.f_seenDistance < 1200.0f) {
            switch(pEdict->v.playerclass) {
            case TFC_CLASS_SCOUT:
                if(newNadeType == GRENADE_RANDOM || newNadeType == GRENADE_DAMAGE) {
                    if(pBot->enemy.f_seenDistance < 400.0f)
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_CALTROP, 0);
                    else
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_CONCUSSION, 1);
                }
                break;
            case TFC_CLASS_SNIPER:
                if(newNadeType == GRENADE_STATIONARY || newNadeType == GRENADE_DAMAGE)
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                else if(newNadeType == GRENADE_RANDOM)
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 1);
                break;
            case TFC_CLASS_SOLDIER:
                if(newNadeType == GRENADE_STATIONARY) {
                    if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_NAIL, 0))
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_DAMAGE) {
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_RANDOM) {
                    if(pBot->visEnemyCount > pBot->visAllyCount) {
                        if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_NAIL, 0))
                            BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                    } else if(!BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0))
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_NAIL, 1);
                }
                break;
            case TFC_CLASS_DEMOMAN:
                if(newNadeType == GRENADE_STATIONARY) {
                    if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_MIRV, 0))
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_DAMAGE) {
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_RANDOM) {
                    if(pBot->enemy.ptr->v.playerclass == TFC_CLASS_HWGUY || pBot->visEnemyCount > pBot->visAllyCount) {
                        if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_MIRV, 0))
                            BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                    } else if(!BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0))
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_MIRV, 1);
                }
                break;
            case TFC_CLASS_MEDIC:
                if((newNadeType == GRENADE_DAMAGE)) {
                    if(random_long(0, 100) < 20)
                        FakeClientCommand(pEdict, "snark", NULL, NULL); // NeoTF specific?
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_STATIONARY) {
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                    FakeClientCommand(pEdict, "snark", NULL, NULL);
                    FakeClientCommand(pEdict, "snark", NULL, NULL);
                } else if(newNadeType == GRENADE_RANDOM) {
                    if(!BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 1))
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_CONCUSSION, 1);
                }
                break;
            case TFC_CLASS_HWGUY:
                if(newNadeType == GRENADE_STATIONARY) {
                    if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_MIRV, 0))
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_DAMAGE) {
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_RANDOM) {
                    if(pBot->enemy.ptr->v.playerclass == TFC_CLASS_HWGUY || pBot->visEnemyCount > pBot->visAllyCount) {
                        if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_MIRV, 0))
                            BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                    } else if(!BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0))
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_MIRV, 1);
                }
                break;
            case TFC_CLASS_PYRO:
                if(newNadeType == GRENADE_STATIONARY) {
                    if(!BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0))
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_FLAME, 0);
                } else if(newNadeType == GRENADE_DAMAGE)
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                else if(newNadeType == GRENADE_RANDOM) {
                    if(random_long(0, 1000) < 500)
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_FLAME, 0);
                    else
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                }
                break;
            case TFC_CLASS_SPY:
                if(newNadeType == GRENADE_STATIONARY) {
                    if(!BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0))
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_SPY_GAS, 1);
                } else if(newNadeType == GRENADE_DAMAGE || newNadeType == GRENADE_RANDOM) {
                    if(pBot->visEnemyCount > pBot->visAllyCount)
                        BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_SPY_GAS, 0);
                    else
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 1);
                }
                break;
            case TFC_CLASS_ENGINEER:
                if(newNadeType == GRENADE_STATIONARY) {
                    if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_EMP, 0))
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_DAMAGE) {
                    BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                } else if(newNadeType == GRENADE_RANDOM) {
                    if(pBot->enemy.ptr->v.playerclass == TFC_CLASS_HWGUY ||
                        pBot->enemy.ptr->v.playerclass == TFC_CLASS_DEMOMAN ||
                        pBot->enemy.ptr->v.playerclass == TFC_CLASS_SOLDIER ||
                        pBot->enemy.ptr->v.playerclass == TFC_CLASS_PYRO) {
                        if(!BotPrimeGrenade(pBot, SecondaryGrenade, GRENADE_EMP, 0))
                            BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                    } else
                        BotPrimeGrenade(pBot, PrimaryGrenade, GRENADE_FRAGMENTATION, 0);
                }
                break;
            }
        }
    }

    // Go ahead and toss em if they aren't meant to be timed.
    if(!timed) {
        FakeClientCommand(pEdict, "-gren1", "102", NULL);
        FakeClientCommand(pEdict, "-gren2", "101", NULL);
        pBot->tossNade = 1;
        rtnValue = 1;
        pBot->nadePrimed = FALSE;
    }

    return rtnValue;
}

// A simple function you can call if you want a bot to prime a grenade.
// The reserve parameter lets you specify how many grenades you want the bot
// to keep in reserve(for other occasions).
// Returns true if a grenade was primed, false otherwise.
static bool BotPrimeGrenade(bot_t* pBot, const int slot, const char nadeType, const unsigned short reserve)
{
    // abort if the bot has run out of the requested grenade type
    if(slot == PrimaryGrenade && pBot->grenades[PrimaryGrenade] > static_cast<int>(reserve))
        FakeClientCommand(pBot->pEdict, "+gren1", "102", NULL);
    else if(slot == SecondaryGrenade && pBot->grenades[SecondaryGrenade] > static_cast<int>(reserve))
        FakeClientCommand(pBot->pEdict, "+gren2", "101", NULL);
    else
        return FALSE;

    // Skill level based primed grenade hold time.
    const float primeError = -random_float(0.0, static_cast<float>(pBot->bot_skill)) * 0.1f;

    pBot->nadeType = nadeType;
    pBot->nadePrimed = TRUE;
    pBot->primeTime = pBot->f_think_time + primeError;

    return TRUE;
}

// Assess the threat level of the target using their class & my class.
// 0-100 scale higher is more dangerous
// returns -1 if error or no target.
int BotAssessThreatLevel(bot_t* pBot)
{
    // BotAssessThreatLevel - This function evaluates the threat level
    // of a target based on the following.
    // 1) This bots class
    // 2) The enemy bots class
    // 3) Distance to the target (modifier by class, since some are
    //    deadlier at different distances)
    // 4) This bots health.
    // Returns a 0 - 100 integer that is the result of the threat assessment.
    // Code can compare against certain values ( threat > 50... ) to trigger various events.
    // This forms the backbone of pre-emptive grenade tossing.
    // No more completely random or when he is low health.

    // Error checking. Do we even have an enemy?
    if(pBot->enemy.ptr == NULL)
        return -1;

    // This will keep track of the threat level.
    int Threat = 0;

    // Set base threat from MY class
    switch(pBot->pEdict->v.playerclass) {
    case TFC_CLASS_CIVILIAN:
        Threat = 20;
        break;
    case TFC_CLASS_SCOUT:
        Threat = 20;
        break;
    case TFC_CLASS_SNIPER:
        Threat = 17;
        break;
    case TFC_CLASS_SOLDIER:
        Threat = 8;
        break;
    case TFC_CLASS_DEMOMAN:
        Threat = 12;
        break;
    case TFC_CLASS_MEDIC:
        Threat = 12;
        break;
    case TFC_CLASS_HWGUY:
        Threat = 0;
        break;
    case TFC_CLASS_PYRO:
        Threat = 15;
        break;
    case TFC_CLASS_SPY:
        Threat = 15;
        break;
    case TFC_CLASS_ENGINEER:
        Threat = 17;
        break;
    }

    // Then add based on targets class.
    switch(pBot->enemy.ptr->v.playerclass) {
    case TFC_CLASS_SCOUT:
        Threat += 5;
        break;
    case TFC_CLASS_SNIPER:
        // added by rix(snipers are more scary when further away though)
        if(pBot->enemy.f_seenDistance < 800.0f)
            Threat += 10;
        break;
    case TFC_CLASS_SOLDIER:
        Threat += 15;
        if(pBot->enemy.f_seenDistance < 400.0f)
            Threat += 15;
        break;
    case TFC_CLASS_DEMOMAN:
        Threat += 13;
        if(pBot->enemy.f_seenDistance < 400.0f)
            Threat += 13;
        break;
    case TFC_CLASS_MEDIC:
        Threat += 10;
        if(pBot->enemy.f_seenDistance < 100.0f)
            Threat += 25;
        break;
    case TFC_CLASS_HWGUY:
        Threat += 15;
        if(pBot->enemy.f_seenDistance < 600.0f)
            Threat += 35;
        break;
    case TFC_CLASS_PYRO:
        Threat += 10;
        if(pBot->enemy.f_seenDistance < 200.0f)
            Threat += 10;
        break;
    case TFC_CLASS_SPY:
        Threat += 15;
        if(pBot->enemy.f_seenDistance < 80.0f)
            Threat += 35;
        break;
    case TFC_CLASS_ENGINEER:
        Threat += 15;
        if(pBot->enemy.f_seenDistance < 300.0f)
            Threat += 10;
        break;
    }

    // ramp up the threat level if the bot is outnumbered
    if(pBot->visEnemyCount > pBot->visAllyCount)
        Threat += (pBot->visEnemyCount - pBot->visAllyCount) * 15;
    else {
        // if the enemy appears outnumbered, reduce the threat level
        Threat -= ((pBot->visAllyCount) - pBot->visEnemyCount) * 10;

        // experimental - don't prime grenades when too near to enemy
        // (probably not the best place to do this)
        if(pBot->enemy.ptr->v.playerclass != TFC_CLASS_HWGUY && pBot->enemy.f_seenDistance < 200.0f)
            Threat -= 50;
    }

    // The less our health, the higher threat the enemy is.
    Threat += (100 - PlayerHealthPercent(pBot->pEdict));

    return (Threat <= 100) ? Threat : 100;
}

// BotTeamColorCheck -  Use this for getting sentry gun teams.
// Checks team based on the colormap.
int BotTeamColorCheck(const edict_t* pent)
{
    if(pent->v.colormap == 0xA096)
        return 0; // blue team's sentry
    if(pent->v.colormap == 0x04FA)
        return 1; // red team's sentry
    if(pent->v.colormap == 0x372D)
        return 2; // yellow team's sentry
    if(pent->v.colormap == 0x6E64)
        return 3; // green team's sentry

    // Unrecognised colormap
    return -1;
}

// This function will return the index number of a random enemy team
// or -1 if no enemy players were found.
// Make sure you specify the bots team, and check the result isn't -1.
int PickRandomEnemyTeam(const int my_team)
{
    int teamList[4]; // there wont ever be 4 hostile teams, but hey
    int total = 0;

    // count and index the hostile teams
    for(int index = 0; index < 4; index++) {
        if(is_team[index] == TRUE && my_team != index && !(team_allies[my_team] & (1 << index))) {
            teamList[total] = index;
            ++total;
        }
    }

    if(total == 1) // a common scenario
        return teamList[0];
    else if(total > 1) // hello hunted!
        return teamList[random_long(0, total - 1)];

    // failure shouldn't happen but let's keep an eye out for it anyway
    static short failureReported = 0;

    if(failureReported == 0) {
        UTIL_BotLogPrintf("Couldn't pick a hostile team for team %d on %s\n", my_team, STRING(gpGlobals->mapname));
        failureReported = 1; // no log spam!
    }

    // no hostile teams found
    return -1;
}

static int BotLeadTarget(edict_t* pBotEnemy, bot_t* pBot, const int projSpeed, float& d_x, float& d_y, float& d_z)
{
    // BotLeadTarget - I moved the leading into a function, so that
    // I could use it for grenades as well without duplicating code.
    // This leads a target based on a passed projectile speed.

    float f_distance = (pBotEnemy->v.origin - pBot->pEdict->v.origin).Length();

    // Lets try some error in the projectile prediction based on skill level
    f_distance += static_cast<float>(random_long(-pBot->bot_skill, pBot->bot_skill) * 5);

    if(f_distance > 1000.0f)
        f_distance = 1000.0f;

    d_x += (pBot->enemy.ptr->v.velocity.x * (f_distance / projSpeed));
    d_y += (pBot->enemy.ptr->v.velocity.y * (f_distance / projSpeed));

    if(pBot->enemy.ptr->v.velocity.z < -30) // falling
        d_z += (pBot->enemy.ptr->v.velocity.z * (f_distance / 2000.0f));
    else
        d_z += (pBot->enemy.ptr->v.velocity.z * (f_distance / 1000.0f));

    return static_cast<int>(pBot->enemy.ptr->v.origin.z - pBot->pEdict->v.origin.z);
}

// BotCheckForMultiguns - Loops through all the multigun types
// looking for the closest type of sentry.
void BotCheckForMultiguns(bot_t* pBot, float nearestdistance, edict_t* pNewEnemy, bool& rtn)
{
    // This 40ish lines of code replaces 500 used previously.
    // Also, adding new mg types to check for is as
    // simple as adding another string to the ntfTargetChecks array.

    // Skip this shit if neotf isnt present
    char* cvar_ntf = (char*)CVAR_GET_STRING("neotf");
    if(strcmp(cvar_ntf, "1")) // No neotf
        return;

    int sentry_team;

    Vector vecEnd;
    float distance;

    // Loop through all the multigun types, checking for a closer target
    // and storing it in nearestdistance, and pNewEnemy
    for(int i = 0; i < NumNTFGuns; i++) {
        edict_t* pent = NULL;
        while((pent = FIND_ENTITY_BY_CLASSNAME(pent, ntfTargetChecks[i])) != NULL && (!FNullEnt(pent))) {
            sentry_team = pent->v.team - 1;

            // flagged for deletion by the engine?
            if((pent->v.flags & FL_KILLME) == FL_KILLME)
                continue;

            // don't target friendly sentry guns...
            if(pBot->current_team == sentry_team || team_allies[pBot->current_team] & (1 << sentry_team))
                continue;

            // is this the closest visible sentry gun?
            distance = (pent->v.origin - pBot->pEdict->v.origin).Length();
            vecEnd = pent->v.origin + pent->v.view_ofs;
            if(distance < nearestdistance && FInViewCone(vecEnd, pBot->pEdict) && FVisible(vecEnd, pBot->pEdict)) {
                nearestdistance = distance;
                pNewEnemy = pent;
                rtn = FALSE;

                BotSGSpotted(pBot, pent);
            }
        }
    }
}

// This function should be called once each frame so that it can maintain
// an up to date list of which players are currently carrying a flag.
void UpdateFlagCarrierList(void)
{
    int i;

    // reset the list before updating it
    for(i = 0; i < 32; i++) {
        playerHasFlag[i] = FALSE;
    }

    // search for visible friendly flag carriers and track them.
    edict_t* pent = NULL;
    edict_t* pPlayer;
    while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "item_tfgoal")) != NULL && (!FNullEnt(pent))) {
        // search the world for players...
        for(int i = 1; i <= gpGlobals->maxClients; i++) {
            pPlayer = INDEXENT(i);

            // remember if this player index owns(carries) this flag
            if(pPlayer && !pPlayer->free && pent->v.owner == pPlayer && IsAlive(pPlayer)) {
                playerHasFlag[i - 1] = TRUE;

                const int botIndex = i - 2; // -2 definitely not -1!

                // if the player is a bot set it's flag impulse to match the flag
                // so the bot knows which flag it has
                if(bots[botIndex].is_used) {
                    bots[botIndex].flag_impulse = pent->v.impulse;
                }
            }
        }
    }
}

// This function will return TRUE if the specified player is carrying a flag,
// FALSE otherwise.
bool PlayerHasFlag(edict_t* Player)
{
    // search the world for players to find the one with the matching index
    for(int i = 1; i <= gpGlobals->maxClients; i++) {
        if(INDEXENT(i) == Player && playerHasFlag[i - 1])
            return TRUE;
    }

    return FALSE;
}

// This function returns TRUE if the specified player is infected,
// FALSE otherwise.
bool PlayerIsInfected(const edict_t* pEntity)
{
    edict_t* pent = NULL;

    while((pent = FIND_ENTITY_BY_CLASSNAME(pent, "timer")) != NULL && !FNullEnt(pent)) {
        // UTIL_SavePent(pent);
        if(pent->v.owner == pEntity && pent->v.enemy) {
            // make sure the enemy who put this infection timer on the player is
            // a TFC class(used to check they were Medics, but that's no good if
            // the medic changes class after infecting someone)
            if(pent->v.enemy->v.playerclass >= TFC_CLASS_SCOUT && pent->v.enemy->v.playerclass <= TFC_CLASS_ENGINEER)
                return TRUE;
        }
    }

    return FALSE;
}
