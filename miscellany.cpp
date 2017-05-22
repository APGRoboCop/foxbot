//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
// (http://foxbot.net)
//
// miscellany.cpp
//
// Copyright (C) 2003 - Tom "Redfox" Simpson
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
#include "miscellany.hpp"


// This function tries to find out where the Foxbot directory is and
// attempts to piece together the path and name of the specified file
// and/or directory.
void botpath_class::UTIL_BuildFileName(char *filename, const int max_fn_length,
	char *arg1, char *arg2)
{
	filename[0] = '\0';

	UTIL_FindFoxbotPath();

	// only support TFC, as it's what Foxbot does best
	// also make sure the Foxbot pathname is valid
	if(mod_id != TFC_DLL
		|| foxbot_path.empty())
		return;

	stringFullname = foxbot_path;

	// add on the directory and or filename
	if((arg1) && (*arg1) && (arg2) && (*arg2))
	{
		stringFullname += arg1;

#ifndef __linux__
		stringFullname += "\\";
#else
		stringFullname += "/";
#endif

		stringFullname += arg2;
	}
	else if((arg1) && (*arg1))
		stringFullname += arg1;

	// copy the full filename into the waiting filename
	if(stringFullname.size() < max_fn_length)
		strncpy(filename, stringFullname.c_str(), max_fn_length);
}


// This function tries to find the Foxbot directory path by testing for
// the existence of foxbot.cfg in expected relative locations.
// e.g. /tfc/addons/  or the main Half-Life directory.
static void botpath_class::UTIL_FindFoxbotPath(void)
{
	// only do this test once
	static bool dir_path_checked = FALSE;
	if(dir_path_checked)
		return;
	else dir_path_checked = TRUE;

	// find out where the foxbot directory is, by trying to open and
	// close the foxbot.cfg file just once
#ifndef __linux__  // must be a Windows machine
	if(foxbot_path.empty())
	{
		// try the addons directory first(for Foxbot 0.76 and newer)
		FILE *fptr = fopen("tfc\\addons\\foxbot\\tfc\\foxbot.cfg", "r");
		if(fptr != NULL)
		{
			foxbot_path = "tfc\\addons\\foxbot\\tfc\\";
			foxbot_logname = "tfc\\addons\\foxbot\\foxbot.log";
			fclose(fptr);
			fptr = NULL;
		}
		else  // try the older directory location(Foxbot 0.75 and older)
		{
			fptr = fopen("foxbot\\tfc\\foxbot.cfg", "r");
			if(fptr != NULL)
			{
				foxbot_path = "foxbot\\tfc\\";
				foxbot_logname = "foxbot\\foxbot.log";
				fclose(fptr);
				fptr = NULL;
			}
		}
	}
#else  // must be a Linux machine
	if(foxbot_path.empty())
	{
		// try the addons directory first(for Foxbot 0.76 and newer)
		FILE *fptr = fopen("tfc/addons/foxbot/tfc/foxbot.cfg", "r");
		if(fptr != NULL)
		{
			foxbot_path = "tfc/addons/foxbot/tfc/";
			foxbot_logname = "tfc/addons/foxbot/foxbot.log";
			fclose(fptr);
			fptr = NULL;
		}
		else  // try the older directory location(Foxbot 0.75 and older)
		{
			fptr = fopen("foxbot/tfc/foxbot.cfg", "r");
			if(fptr != NULL)
			{
				foxbot_path = "foxbot/tfc/";
				foxbot_logname = "foxbot/foxbot.log";
				fclose(fptr);
				fptr = NULL;
			}
		}
	}
#endif

	// report a problem if the Foxbot directory wasn't found
	if(foxbot_path.empty())
	{
		if(IS_DEDICATED_SERVER())
		{
			printf("\nfoxbot.cfg should be in the \\foxbot\\tfc\\ directory\n");
			printf("--Check your Foxbot installation is correct--\n\n");
		}
		else
		{
			ALERT( at_console, "\nfoxbot.cfg should be in the \\foxbot\\tfc\\ directory\n");
			ALERT( at_console, "--Check your Foxbot installation is correct--\n\n");
		}
	}
}


#if 0  // not ready yet
// This is a long version of rand().
// This function seems to give good variation even when asked for a
// smaller spread of numbers.
long random_long(const long lowval, const long highval)
{
	const long spread = highval - lowval;

	// make sure a sensible number was requested
	if(spread < 1)
		return lowval;

	lseed = (lseed * 1103515245 + 12345) % 2147483647;

	return ((lseed / 3) % spread) + lowval;
}


// This function is a quick simple way of testing the distance between two
// vectors against some distance value.
// This should be faster than using the Vector classes .Length() function
// because it requires no function call to sqrt().
bool VectorsNearerThan(const Vector &r_vOne, const Vector &r_vTwo,
	double value)
{
	value = value * value;

	Vector distance = r_vOne - r_vTwo;
	double temp
		= (distance.x * distance.x)
		+ (distance.y * distance.y);

	// perform an early 2 dimensional check, because most maps
	// are wider/longer than they are taller
	if(temp > value)
		return FALSE;
	else temp += (distance.z * distance.z);

	// final check(3 dimensional)
	if(temp < value)
		return TRUE;

	return FALSE;
}


// This function can be used to report the number of players using the specified class
// on the specified players team, or a team allied to them.
// You'll be cheating if you use this function on an enemy team.  >:-O
int FriendlyClassTotal(edict_t *pEdict, const int specifiedClass, const bool ignoreSelf)
{
	if(!checked_teamplay)  // check for team play...
		BotCheckTeamplay();

	// report failure if there is no team play
	if(is_team_play <= 0
		|| mod_id != TFC_DLL)
		return FALSE;

	const int my_team = UTIL_GetTeam(pEdict);
	edict_t *pPlayer;
	int player_team;
	int classTotal = 0;

	// search the world for players...
	for(int i = 1; i <= gpGlobals->maxClients; i++)
	{
		pPlayer = INDEXENT(i);

		// skip invalid players
		if(pPlayer
			&& !pPlayer->free)
		{
			// check the specified player if instructed to do so
			if(pPlayer == pEdict)
			{
				if(pPlayer->v.playerclass == specifiedClass
					&& !ignoreSelf)
					++classTotal;
			}
			else if(IsAlive(pPlayer)
				&& pPlayer->v.playerclass == specifiedClass)
			{
				player_team = UTIL_GetTeam(pPlayer);

				// add another if the player is a teammate or ally
				if(my_team == player_team
					|| team_allies[my_team] & (1 << player_team) )
					++classTotal;
			}
		}
	}

	return classTotal;
}


// This function is a sort of wrapper around fgets().
// Unlike fgets() it always attempts to find the end of the current line
// in the file.
// It also makes sure that the string is null terminated on success.
// It returns false if fgets() returned NULL.
bool UTIL_ReadFileLine(char *string, const unsigned int max_length,
	FILE *file_ptr)
{
	unsigned int a;
	bool line_end_found = FALSE;

	if(fgets(string, 80, file_ptr) == NULL)
		return FALSE;

	// check if the string read contains a line terminator of some sort
	for(a = 0; a < max_length; a++)
	{
		if(string[a] == '\n'
			|| string[a] == '\r')
			line_end_found = TRUE;
	}

	// if the end of the current line in the file was not found,
	// go look for it
	if(!line_end_found)
	{
		//printf("finding line end\n");
		int c;
		do {
			c = fgetc(file_ptr);
		} while(c != '\n' && c != '\r' && c != EOF);
	}
	else  /* make sure the string is null terminated */
		string[max_length - 1] = '\0';

	return TRUE;
}


// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
CBaseEntity	*UTIL_PlayerByIndex( int playerIndex )
{
	CBaseEntity *pPlayer = NULL;

	if( playerIndex > 0
		&& playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = INDEXENT( playerIndex );

		if( pPlayerEdict && !pPlayerEdict->free )
		{
			pPlayer = CBaseEntity::Instance( pPlayerEdict );
		}
	}

	return pPlayer;
}


// This function counts the number of clients currently in the game.
// It's basically a minor variant of the one posted by Leon Hartwig
// in the HLCoders list, and should(in theory) be able to cope with players
// who have only partially joined the server.
int UTIL_CountClientsInGame(void)
{
	int total = 0;

	for(int index = 1; index <= gpGlobals->maxClients; index++)
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex(index);

		// is this a valid connected player?
		if(pPlayer == NULL
			|| FNullEnt(pPlayer->pev)
			|| FStrEq(STRING(pPlayer->pev->netname), ""))
			continue;

		total++;
	}

	return total;
}
#endif
