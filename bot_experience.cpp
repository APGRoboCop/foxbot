// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_experience.cpp
//
// Per-map experience data storage - inspired by RCBot1 by Paul Murphy 'Cheeseh'
// (http://www.rcbot.net)
//
// Original RCBot code is free software under the GNU General Public License v2+
// This adaptation maintains the same license.
//

#include "extdll.h"
#include "util.h"
#include <enginecallback.h>

#include "bot.h"
#include "waypoint.h"
#include "bot_experience.h"
#include "bot_neuralnet.h" // for FoxEnsureDirectory
#include "bot_compress.h"

#include <algorithm>
#include <cmath>

extern int mod_id;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;

// Global per-map experience instance
FoxMapExperience g_MapExperience;

FoxMapExperience::FoxMapExperience()
{
	clear();
}

void FoxMapExperience::clear()
{
	for (int t = 0; t < 4; t++)
	{
		m_entryCount[t] = 0;
		for (int i = 0; i < MAX_SG_DEATH_ENTRIES; i++)
		{
			m_entries[t][i].waypoint = -1;
			m_entries[t][i].dangerLevel = 0;
			m_entries[t][i].lastSeenTime = 0.0f;
		}
	}
}

int FoxMapExperience::findEntry(const int team, const int waypoint) const
{
	if (team < 0 || team >= 4 || waypoint < 0)
		return -1;

	for (int i = 0; i < m_entryCount[team]; i++)
	{
		if (m_entries[team][i].waypoint == waypoint)
			return i;
	}
	return -1;
}

int FoxMapExperience::findOrCreateEntry(const int team, const int waypoint)
{
	if (team < 0 || team >= 4 || waypoint < 0)
		return -1;

	// look for existing entry
	const int existing = findEntry(team, waypoint);
	if (existing >= 0)
		return existing;

	// create new entry if space available
	if (m_entryCount[team] < MAX_SG_DEATH_ENTRIES)
	{
		const int idx = m_entryCount[team]++;
		m_entries[team][idx].waypoint = waypoint;
		m_entries[team][idx].dangerLevel = 0;
		m_entries[team][idx].lastSeenTime = 0.0f;
		return idx;
	}

	// table full - overwrite the entry with the lowest danger level
	int lowestIdx = 0;
	short lowestDanger = m_entries[team][0].dangerLevel;
	for (int i = 1; i < MAX_SG_DEATH_ENTRIES; i++)
	{
		if (m_entries[team][i].dangerLevel < lowestDanger)
		{
			lowestDanger = m_entries[team][i].dangerLevel;
			lowestIdx = i;
		}
	}

	m_entries[team][lowestIdx].waypoint = waypoint;
	m_entries[team][lowestIdx].dangerLevel = 0;
	m_entries[team][lowestIdx].lastSeenTime = 0.0f;
	return lowestIdx;
}

void FoxMapExperience::recordSGDeath(const int team, const int waypoint)
{
	const int idx = findOrCreateEntry(team, waypoint);
	if (idx < 0)
		return;

	m_entries[team][idx].dangerLevel =
		std::min(static_cast<short>(m_entries[team][idx].dangerLevel + SG_DANGER_INCREMENT), SG_DANGER_MAX);
}

void FoxMapExperience::confirmSGActive(const int team, const int waypoint, const float currentTime)
{
	const int idx = findOrCreateEntry(team, waypoint);
	if (idx < 0)
		return;

	m_entries[team][idx].lastSeenTime = currentTime;
	// bump danger slightly when confirmed
   m_entries[team][idx].dangerLevel =
	   std::max(m_entries[team][idx].dangerLevel, SG_DANGER_THRESHOLD);
}

void FoxMapExperience::clearSGDanger(const int team, const int waypoint)
{
	const int idx = findEntry(team, waypoint);
	if (idx < 0)
		return;

	// halve the danger instead of zeroing, so it decays over time
	m_entries[team][idx].dangerLevel /= 2;
}

short FoxMapExperience::getSGDanger(const int team, const int waypoint) const
{
	const int idx = findEntry(team, waypoint);
	if (idx < 0)
		return 0;
	return m_entries[team][idx].dangerLevel;
}

int FoxMapExperience::findNearestDangerousWP(const int team, const int nearWP, const float maxDist) const
{
	if (team < 0 || team >= 4 || nearWP < 0 || nearWP >= num_waypoints)
		return -1;

	int bestWP = -1;
	float bestDist = maxDist;

	for (int i = 0; i < m_entryCount[team]; i++)
	{
		if (m_entries[team][i].dangerLevel < SG_DANGER_THRESHOLD)
			continue;

		const int wp = m_entries[team][i].waypoint;
		if (wp < 0 || wp >= num_waypoints)
			continue;

		const float dist = (waypoints[wp].origin - waypoints[nearWP].origin).Length();
		if (dist < bestDist)
		{
			bestDist = dist;
			bestWP = wp;
		}
	}

	return bestWP;
}

// File format:
//   int: version (1)
//   For each of 4 teams:
//     int: entryCount
//     entryCount * { int waypoint, short dangerLevel }

constexpr int FXP_VERSION = 1;

bool FoxMapExperience::saveForMap(const char* mapname) const
{
	if (mapname == nullptr || mapname[0] == '\0')
		return false;

	FoxEnsureDirectory("experience");

	char fname[64];
	char filename[256];

	std::strncpy(fname, mapname, 55);
	fname[55] = '\0';
	std::strcat(fname, ".fxp"); // FoXBot eXPerience

	UTIL_BuildFileName(filename, 255, "experience", fname);

	std::FILE* bfp = std::fopen(filename, "wb");
	if (bfp == nullptr)
		return false;

	// serialize to a memory buffer first
	// max size: version(4) + 4 teams * (count(4) + 64 entries * (waypoint(4) + danger(2)))
	constexpr unsigned int maxBufSize = sizeof(int) + 4 * (sizeof(int) + MAX_SG_DEATH_ENTRIES * (sizeof(int) + sizeof(short)));
	unsigned char buf[maxBufSize];
	unsigned int offset = 0;

	const int version = FXP_VERSION;
	std::memcpy(buf + offset, &version, sizeof(int));
	offset += sizeof(int);

	for (int t = 0; t < 4; t++)
	{
		std::memcpy(buf + offset, &m_entryCount[t], sizeof(int));
		offset += sizeof(int);
		for (int i = 0; i < m_entryCount[t]; i++)
		{
			std::memcpy(buf + offset, &m_entries[t][i].waypoint, sizeof(int));
			offset += sizeof(int);
			std::memcpy(buf + offset, &m_entries[t][i].dangerLevel, sizeof(short));
			offset += sizeof(short);
		}
	}

	const bool result = FoxCompressedWrite(bfp, buf, offset);
	std::fclose(bfp);
	return result;
}

bool FoxMapExperience::loadForMap(const char* mapname)
{
	if (mapname == nullptr || mapname[0] == '\0')
		return false;

	char fname[64];
	char filename[256];

	std::strncpy(fname, mapname, 55);
	fname[55] = '\0';
	std::strcat(fname, ".fxp");

	UTIL_BuildFileName(filename, 255, "experience", fname);

	std::FILE* bfp = std::fopen(filename, "rb");
	if (bfp == nullptr)
		return false;

	// read the file (handles both compressed and uncompressed)
	unsigned int dataSize = 0;
	unsigned char* data = FoxCompressedRead(bfp, &dataSize);
	std::fclose(bfp);

	if (data == nullptr || dataSize < sizeof(int))
	{
		std::free(data);
		return false;
	}

	unsigned int offset = 0;

	int version = 0;
	std::memcpy(&version, data + offset, sizeof(int));
	offset += sizeof(int);

	if (version != FXP_VERSION)
	{
		std::free(data);
		return false;
	}

	clear();

	for (int t = 0; t < 4; t++)
	{
		if (offset + sizeof(int) > dataSize)
		{
			std::free(data);
			return false;
		}

		int count = 0;
		std::memcpy(&count, data + offset, sizeof(int));
		offset += sizeof(int);

		if (count < 0 || count > MAX_SG_DEATH_ENTRIES)
			count = 0;

		m_entryCount[t] = count;
		for (int i = 0; i < count; i++)
		{
			if (offset + sizeof(int) + sizeof(short) > dataSize)
			{
				std::free(data);
				return false;
			}

			std::memcpy(&m_entries[t][i].waypoint, data + offset, sizeof(int));
			offset += sizeof(int);
			std::memcpy(&m_entries[t][i].dangerLevel, data + offset, sizeof(short));
			offset += sizeof(short);
			m_entries[t][i].lastSeenTime = 0.0f;
		}
	}

	std::free(data);
	return true;
}

bool FoxMapExperience::save() const
{
	return saveForMap(STRING(gpGlobals->mapname));
}

bool FoxMapExperience::load()
{
	return loadForMap(STRING(gpGlobals->mapname));
}

// ==================== Global functions ====================

void FoxExperienceInit()
{
	FoxEnsureDirectory("experience");

	if (g_MapExperience.load())
	{
		if (IS_DEDICATED_SERVER())
			std::printf("FoXBot: Map experience data loaded\n");
		return;
	}

	g_MapExperience.clear();

	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Map experience data initialized (no saved data)\n");
}

bool FoxExperienceSave()
{
	return FoxExperienceSaveForMap(STRING(gpGlobals->mapname));
}

bool FoxExperienceSaveForMap(const char* mapname)
{
	return g_MapExperience.saveForMap(mapname);
}
