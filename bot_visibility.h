//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_visibility.h
//
// Waypoint visibility table - adapted from RCBot1 by Paul Murphy 'Cheeseh'
// (http://www.rcbot.net)
//
// Original RCBot code is free software under the GNU General Public License v2+
// This adaptation maintains the same license.
//
// The visibility table pre-computes line-of-sight between all waypoint pairs
// and stores the results in a compact bit matrix for O(1) lookups at runtime,
// replacing expensive per-frame UTIL_TraceLine calls.
//

#ifndef BOT_VISIBILITY_H
#define BOT_VISIBILITY_H

#include <cstdio>
#include <cstring>
#include <cstdlib>

class CWaypointVisibilityTable
{
public:
	CWaypointVisibilityTable()
		: m_VisTable(nullptr), m_iTableNumWaypoints(0), m_iMaxBytes(0)
	{
	}

	~CWaypointVisibilityTable()
	{
		FreeVisibilityTable();
	}

	// Allocate the table for the given number of waypoints.
	// Returns false on allocation failure.
	bool AllocateTable(const int iNumWaypoints)
	{
		FreeVisibilityTable();

		if (iNumWaypoints <= 0)
			return false;

		m_iTableNumWaypoints = iNumWaypoints;
		m_iMaxBytes = static_cast<int>((static_cast<long long>(iNumWaypoints) * iNumWaypoints + 7) / 8);

		m_VisTable = static_cast<unsigned char*>(std::calloc(m_iMaxBytes, 1));
		return m_VisTable != nullptr;
	}

	// Returns 1 if waypoint iFrom can see waypoint iTo, 0 otherwise.
	int GetVisibilityFromTo(const int iFrom, const int iTo) const
	{
		if (m_VisTable == nullptr || iFrom < 0 || iTo < 0
			|| iFrom >= m_iTableNumWaypoints || iTo >= m_iTableNumWaypoints)
			return 0;

		const int iPosition = iFrom * m_iTableNumWaypoints + iTo;
		const int iByte = iPosition / 8;
		const int iBit = iPosition % 8;

		if (iByte < m_iMaxBytes)
			return (m_VisTable[iByte] & (1 << iBit)) > 0 ? 1 : 0;

		return 0;
	}

	// Set the visibility between two waypoints.
	void SetVisibilityFromTo(const int iFrom, const int iTo, const bool bVisible) const
	{
		if (m_VisTable == nullptr || iFrom < 0 || iTo < 0
			|| iFrom >= m_iTableNumWaypoints || iTo >= m_iTableNumWaypoints)
			return;

		const int iPosition = iFrom * m_iTableNumWaypoints + iTo;
		const int iByte = iPosition / 8;
		const int iBit = iPosition % 8;

		if (iByte < m_iMaxBytes)
		{
			if (bVisible)
				m_VisTable[iByte] |= (1 << iBit);
			else
				m_VisTable[iByte] &= ~(1 << iBit);
		}
	}

	void ClearVisibilityTable() const
	{
		if (m_VisTable != nullptr)
			std::memset(m_VisTable, 0, m_iMaxBytes);
	}

	void FreeVisibilityTable()
	{
		if (m_VisTable != nullptr)
		{
			std::free(m_VisTable);
			m_VisTable = nullptr;
		}
		m_iTableNumWaypoints = 0;
		m_iMaxBytes = 0;
	}

	// Compute the entire visibility table using TraceLine for all waypoint pairs.
	void WorkOutVisibilityTable(int iNumWaypoints);

	// Save the visibility table to a .fvt file in the waypoints directory.
	bool SaveToFile() const;

	// Load the visibility table from a .fvt file in the waypoints directory.
	// Returns false if file doesn't exist or is the wrong size.
	bool ReadFromFile(int iNumWaypoints);

	bool IsAllocated() const { return m_VisTable != nullptr; }

	int GetNumWaypoints() const { return m_iTableNumWaypoints; }

private:
	unsigned char* m_VisTable;
	int m_iTableNumWaypoints; // the number of waypoints when the table was built
	int m_iMaxBytes;          // total byte size of the bit table
};

// Global visibility table instance
extern CWaypointVisibilityTable g_WaypointVisibility;

// Call after waypoints are loaded to set up the visibility table.
void WaypointVisibilityInit();

// Quick O(1) waypoint-to-waypoint visibility check using the pre-computed table.
// Returns true if the two waypoints can see each other, or falls back to
// a TraceLine if the table is not available.
bool WaypointVisibleFromTo(int iFrom, int iTo);

#endif // BOT_VISIBILITY_H
