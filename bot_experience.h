//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_experience.h
//
// Per-map experience data storage - inspired by RCBot1 by Paul Murphy 'Cheeseh'
// (http://www.rcbot.net)
//
// Stores per-map learned data such as sentry gun death locations so bots
// can remember dangerous waypoints and approach them more cautiously.
//
// Original RCBot code is free software under the GNU General Public License v2+
// This adaptation maintains the same license.
//

#ifndef BOT_EXPERIENCE_H
#define BOT_EXPERIENCE_H

#include <cstdio>
#include <cstring>

// Maximum number of sentry-death locations tracked per team
constexpr int MAX_SG_DEATH_ENTRIES = 64;

// How dangerous a single SG kill makes a waypoint (additive, clamped)
constexpr short SG_DANGER_INCREMENT = 25;
constexpr short SG_DANGER_MAX = 100;

// Minimum danger value to consider a waypoint "known dangerous"
constexpr short SG_DANGER_THRESHOLD = 15;

// Per-team record of a waypoint where a teammate was killed by a sentry gun
struct SGDeathEntry {
	int waypoint;        // waypoint index where the death occurred
	short dangerLevel;   // accumulated danger rating (0-100)
	float lastSeenTime;  // last time a sentry was confirmed at/near this location
};

// Per-map experience data, stored to and loaded from the /experience directory.
// Tracks dangerous sentry gun locations per team so bots can plan routes
// that avoid or cautiously approach known SG placements.
class FoxMapExperience
{
public:
	FoxMapExperience();

	void clear();

	// Record that a bot on the given team was killed by a sentry gun
	// near the specified waypoint.
	void recordSGDeath(int team, int waypoint);

	// Confirm that an enemy SG was seen near a waypoint (refreshes timestamp).
	void confirmSGActive(int team, int waypoint, float currentTime);

	// Mark an SG location as destroyed / no longer dangerous.
	void clearSGDanger(int team, int waypoint);

	// Query the danger level of a specific waypoint for a team.
	// Returns 0 if no known danger, else 1-100.
	short getSGDanger(int team, int waypoint) const;

	// Find the nearest dangerous waypoint to a given waypoint within a range of
	// waypoint indices.  Returns -1 if none found above the threshold.
	int findNearestDangerousWP(int team, int nearWP, float maxDist) const;

	// Save/load to a per-map file in /experience as <mapname>.fxp
	bool saveForMap(const char* mapname) const;
	bool loadForMap(const char* mapname);

	// Save/load using the current map name.
	bool save() const;
	bool load();

private:
	int findOrCreateEntry(int team, int waypoint);
	int findEntry(int team, int waypoint) const;

	SGDeathEntry m_entries[4][MAX_SG_DEATH_ENTRIES]; // 4 teams
	int m_entryCount[4];
};

// Global per-map experience instance
extern FoxMapExperience g_MapExperience;

// Initialise experience data (call once at map load).
void FoxExperienceInit();

// Save experience data (call on map change / server shutdown).
bool FoxExperienceSave();
bool FoxExperienceSaveForMap(const char* mapname);

#endif // BOT_EXPERIENCE_H
