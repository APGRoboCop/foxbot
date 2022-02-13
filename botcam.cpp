//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// botcam.cpp
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

#include "extdll.h"
#include <enginecallback.h>

// meta mod includes
#include <dllapi.h>
#include <meta_api.h>

void CreateCamera(edict_t *pPlayer, edict_t *pEntity) {
   if (pPlayer != nullptr && pEntity != nullptr) {
      edict_t *pCamera = CREATE_NAMED_ENTITY(MAKE_STRING("info_target"));
      MDLL_Spawn(pCamera);
      pCamera->v.origin = pEntity->v.origin + pEntity->v.view_ofs;
      pCamera->v.angles = pEntity->v.v_angle;
      pCamera->v.velocity = pEntity->v.velocity;
      pCamera->v.takedamage = DAMAGE_NO;
      pCamera->v.solid = SOLID_NOT;
      pCamera->v.owner = pPlayer;
      pCamera->v.euser1 = pEntity;
      pCamera->v.movetype = MOVETYPE_FLY; // noclip
      pCamera->v.classname = MAKE_STRING("entity_botcam");
      pCamera->v.nextthink = gpGlobals->time;
      pCamera->v.renderamt = 0.0f;
      // pCamera->v.rendermode = kRenderTransColor;
      // pCamera->v.renderfx = kRenderFxNone;
      SET_MODEL(pCamera, "models/mechgibs.mdl");
      // SET_MODEL(pCamera, "models/nail.mdl");
      SET_VIEW(pPlayer, pCamera);
   }
}

void KillCamera(edict_t *pPlayer) {
   if (pPlayer != nullptr) {
      edict_t *pCCamera = nullptr;
      while ((pCCamera = FIND_ENTITY_BY_CLASSNAME(pCCamera, "entity_botcam")) != nullptr && !FNullEnt(pCCamera)) {
         if (pCCamera->v.owner == pPlayer)
            pCCamera->v.flags |= FL_KILLME;
      }
      SET_VIEW(pPlayer, pPlayer);
   }
}