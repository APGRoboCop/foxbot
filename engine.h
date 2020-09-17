//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// engine.h
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

#ifndef ENGINE_H
#define ENGINE_H

void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
void pfnMessageEnd(void);
void pfnWriteByte(int iValue);
void pfnWriteChar(int iValue);
void pfnWriteShort(int iValue);
void pfnWriteLong(int iValue);
void pfnWriteAngle(float flValue);
void pfnWriteCoord(float flValue);
void pfnWriteString(const char *sz);
void pfnWriteEntity(int iValue);

#endif // ENGINE_H
