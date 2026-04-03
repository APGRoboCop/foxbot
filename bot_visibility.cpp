// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_visibility.cpp
//
// Waypoint visibility table - adapted from RCBot1 by Paul Murphy 'Cheeseh'
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
#include "bot_visibility.h"
#include "bot_neuralnet.h"
#include "bot_compress.h"

extern int mod_id;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;

// Global visibility table instance
CWaypointVisibilityTable g_WaypointVisibility;

// Compute the visibility table by doing a TraceLine between every pair of waypoints.
void CWaypointVisibilityTable::WorkOutVisibilityTable(const int iNumWaypoints)
{
	if (!AllocateTable(iNumWaypoints))
	{
		ALERT(at_console, "FoXBot: Failed to allocate visibility table!\n");
		return;
	}

	ClearVisibilityTable();

	TraceResult tr;

	for (int i = 0; i < iNumWaypoints; i++)
	{
		if (waypoints[i].flags & W_FL_DELETED)
			continue;

		for (int j = 0; j < iNumWaypoints; j++)
		{
			if (i == j)
			{
				SetVisibilityFromTo(i, j, true);
				continue;
			}

			if (waypoints[j].flags & W_FL_DELETED)
				continue;

			// trace from waypoint i at head height to waypoint j at head height
			UTIL_TraceLine(waypoints[i].origin, waypoints[j].origin + Vector(0, 0, 16.0f),
				ignore_monsters, nullptr, &tr);

			SetVisibilityFromTo(i, j, tr.flFraction >= 1.0f);
		}
	}

	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Visibility table computed for %d waypoints\n", iNumWaypoints);
}

// Save the visibility table to a compressed binary file.
bool CWaypointVisibilityTable::SaveToFile() const
{
	if (m_VisTable == nullptr || m_iTableNumWaypoints <= 0)
		return false;

	FoxEnsureDirectory("visibility");

	char mapname[64];
	char filename[256];

	std::strcpy(mapname, STRING(gpGlobals->mapname));
	std::strcat(mapname, ".fvt"); // FoXBot Visibility Table

	UTIL_BuildFileName(filename, 255, "visibility", mapname);

	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Saving visibility table: %s\n", filename);

	std::FILE* bfp = std::fopen(filename, "wb");
	if (bfp == nullptr)
	{
		ALERT(at_console, "FoXBot: Can't open visibility table for writing!\n");
		return false;
	}

	// build uncompressed payload: waypoint count + bit table
	const int iDesiredSize = static_cast<int>((static_cast<long long>(m_iTableNumWaypoints) * m_iTableNumWaypoints + 7) / 8);
	const unsigned int payloadSize = sizeof(int) + iDesiredSize;

   unsigned char *payload = static_cast<unsigned char *>(std::malloc(payloadSize));
	if (payload == nullptr)
	{
		std::fclose(bfp);
		return false;
	}

	std::memcpy(payload, &m_iTableNumWaypoints, sizeof(int));
	std::memcpy(payload + sizeof(int), m_VisTable, iDesiredSize);

	const bool result = FoxCompressedWrite(bfp, payload, payloadSize);
	std::free(payload);

	std::fclose(bfp);

	if (result && IS_DEDICATED_SERVER())
		std::printf("FoXBot: Visibility table saved (compressed %u bytes)\n", payloadSize);

	return result;
}

// Load the visibility table from a binary file (compressed or uncompressed).
bool CWaypointVisibilityTable::ReadFromFile(const int iNumWaypoints)
{
	if (iNumWaypoints <= 0)
		return false;

	char mapname[64];
	char filename[256];

	std::strcpy(mapname, STRING(gpGlobals->mapname));
	std::strcat(mapname, ".fvt");

	UTIL_BuildFileName(filename, 255, "visibility", mapname);

	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Loading visibility table: %s\n", filename);

	std::FILE* bfp = std::fopen(filename, "rb");
	if (bfp == nullptr)
		return false;

	const int iDesiredSize = static_cast<int>((static_cast<long long>(iNumWaypoints) * iNumWaypoints + 7) / 8);
	const unsigned int expectedPayload = sizeof(int) + iDesiredSize;

	// try reading as compressed file (also handles uncompressed via fallback)
	unsigned int dataSize = 0;
	unsigned char* data = FoxCompressedRead(bfp, &dataSize);
	std::fclose(bfp);

	if (data == nullptr || dataSize != expectedPayload)
	{
		std::free(data);
		return false;
	}

	// verify the stored waypoint count
	int storedNumWaypoints = 0;
	std::memcpy(&storedNumWaypoints, data, sizeof(int));
	if (storedNumWaypoints != iNumWaypoints)
	{
		std::free(data);
		return false;
	}

	// allocate the table
	if (!AllocateTable(iNumWaypoints))
	{
		std::free(data);
		return false;
	}

	// copy the bit table from the decompressed data
	std::memcpy(m_VisTable, data + sizeof(int), iDesiredSize);
	std::free(data);

	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Visibility table loaded (%d waypoints)\n", iNumWaypoints);

	return true;
}

// Initialize the visibility table after waypoints are loaded.
// Tries to load from file first; if that fails, computes it and saves.
void WaypointVisibilityInit()
{
	FoxEnsureDirectory("visibility");

	g_WaypointVisibility.FreeVisibilityTable();

	if (num_waypoints <= 0)
		return;

	// try to load a cached visibility table first
	if (g_WaypointVisibility.ReadFromFile(num_waypoints))
		return;

	// no cached table found or it was invalid - compute a new one
	if (IS_DEDICATED_SERVER())
		std::printf("FoXBot: Computing visibility table for %d waypoints...\n", num_waypoints);
	else
		ALERT(at_console, "FoXBot: Computing visibility table...\n");

	g_WaypointVisibility.WorkOutVisibilityTable(num_waypoints);

   if (!g_WaypointVisibility.SaveToFile()) {
      ALERT(at_console, "FoXBot: Failed to save visibility table.\n");
   }
}

// Convenience function: O(1) waypoint-to-waypoint visibility lookup.
// Falls back to a TraceLine if the visibility table is unavailable.
bool WaypointVisibleFromTo(const int iFrom, const int iTo)
{
	if (iFrom < 0 || iTo < 0 || iFrom >= num_waypoints || iTo >= num_waypoints)
		return false;

	if (iFrom == iTo)
		return true;

	// use the pre-computed table if available
	if (g_WaypointVisibility.IsAllocated())
		return g_WaypointVisibility.GetVisibilityFromTo(iFrom, iTo) != 0;

	// fallback: do a runtime TraceLine
	TraceResult tr;
	UTIL_TraceLine(waypoints[iFrom].origin, waypoints[iTo].origin + Vector(0, 0, 16.0f),
		ignore_monsters, nullptr, &tr);
	return tr.flFraction >= 1.0f;
}
