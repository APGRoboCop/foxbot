//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// botcam.h
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

#ifndef BOTCAM_H
#define BOTCAM_H

void CreateCamera(edict_t* pPlayer, edict_t* pEntity);
void KillCamera(edict_t* pPlayer);

/*class CBotCam : public CBaseEntity
{
   public:

      CBasePlayer *m_pPlayer;
      CBasePlayer *m_pBot;

      static CBotCam *Create( CBasePlayer *pPlayer, CBasePlayer *pBot );
      void Spawn( void );
          void EXPORT IThink( void );
      void Disconnect( void );
};*/

#endif
