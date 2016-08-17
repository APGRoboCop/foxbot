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
#include <util.h>
//#include <client.h>
#include "cbase.h"
//#include "player.h"

#include "botcam.h"
#include "bot.h"

void CreateCamera(edict_t* pPlayer, edict_t* pEntity)
{
    if(pPlayer != NULL && pEntity != NULL) {
        edict_t* pCamera;

        pCamera = CREATE_NAMED_ENTITY(MAKE_STRING("info_target"));
        DispatchSpawn(pCamera);
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
        pCamera->v.renderamt = 0;
        // pCamera->v.rendermode = kRenderTransColor;
        // pCamera->v.renderfx = kRenderFxNone;
        SET_MODEL(pCamera, "models/mechgibs.mdl");
        // SET_MODEL(pCamera, "models/nail.mdl");
        SET_VIEW(pPlayer, pCamera);
    }
}

void KillCamera(edict_t* pPlayer)
{
    if(pPlayer != NULL) {
        edict_t* pCCamera;
        pCCamera = NULL;
        while((pCCamera = FIND_ENTITY_BY_CLASSNAME(pCCamera, "entity_botcam")) != NULL && (!FNullEnt(pCCamera))) {
            if(pCCamera->v.owner == pPlayer)
                pCCamera->v.flags |= FL_KILLME;
        }
        SET_VIEW(pPlayer, pPlayer);
    }
}

/*
   LINK_ENTITY_TO_CLASS(entity_botcam, CBotCam);


   CBotCam *CBotCam::Create( CBasePlayer *pPlayer, CBasePlayer *pBot )
   {
            edict_t *pCamera;

    pCamera = CREATE_NAMED_ENTITY(MAKE_STRING("info_target"));
    DispatchSpawn(pCamera);

   CBotCam *pBotCam = GetClassPtr( (CBotCam *)NULL );
        //CBotCam *pBotCam = GetClassPtr( (CBotCam *)CREATE_NAMED_ENTITY(MAKE_STRING("info_target")));
   pBotCam->m_pPlayer = pPlayer;
   pBotCam->m_pBot = pBot;
   {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"a %x %x %x\n",pBotCam,pBotCam->m_pBot,pBotCam->m_pPlayer); fclose(fp); }
   PRECACHE_MODEL( "models/mechgibs.mdl" );

   pPlayer->pev->effects |= EF_NODRAW;
   pPlayer->pev->solid = SOLID_NOT;
   pPlayer->pev->takedamage = DAMAGE_NO;
   pPlayer->m_iHideHUD |= HIDEHUD_ALL;

   pBotCam->Spawn();

   return pBotCam;
   }


   void CBotCam::Spawn( void )
   {
    pCamera->v.origin = pEntity->v.origin;
    if(FStrEq(STRING(pEntity->v.classname), "building_sentrygun"))
        pCamera->v.origin = pCamera->v.origin + Vector(0, 0, 16);
    pCamera->v.angles = pEntity->v.angles;
    pCamera->v.velocity = pEntity->v.velocity;
    pCamera->v.takedamage = DAMAGE_NO;
    pCamera->v.solid = SOLID_NOT;
    pCamera->v.owner = pPlayer;
    pCamera->v.euser1 = pEntity;
    pCamera->v.movetype = MOVETYPE_NOCLIP;
    pCamera->v.classname = MAKE_STRING("ntf_camera");
    pCamera->v.nextthink = gpGlobals->time;
    pCamera->v.renderamt = 0;
    pCamera->v.rendermode = kRenderTransColor;
    pCamera->v.renderfx = kRenderFxNone;
        SET_MODEL(pCamera, "models/nail.mdl");
    SET_VIEW(pPlayer, pCamera);
        {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"spawn %x %x %x\n",this,m_pBot,m_pPlayer); fclose(fp); }
        if(m_pBot!=NULL && m_pPlayer!=NULL && this!=NULL
                && !FNullEnt(m_pBot->edict()) && !FNullEnt(m_pPlayer->edict())
                && !FNullEnt(this->edict()))
        {
           pev->classname = MAKE_STRING("entity_botcam");

           UTIL_MakeVectors(m_pBot->pev->v_angle);

           TraceResult tr;
           UTIL_TraceLine(m_pBot->pev->origin + m_pBot->pev->view_ofs,
                                          m_pBot->pev->origin + m_pBot->pev->view_ofs + gpGlobals->v_forward * -16 +
   gpGlobals->v_up * 10,
                                          dont_ignore_monsters, m_pBot->edict(), &tr );

           //UTIL_SetOrigin(pev, tr.vecEndPos);
           SET_ORIGIN(edict(), m_pBot->pev->origin + m_pBot->pev->view_ofs);
           this->pev->origin=m_pBot->pev->origin + m_pBot->pev->view_ofs;

           pev->angles = m_pBot->pev->v_angle;

           pev->fixangle = TRUE;

           SET_VIEW (m_pPlayer->edict(), edict());

           // mechgibs seems to be an "invisible" model.  Other players won't see
           // anything when this model is used as the botcam...

           SET_MODEL(ENT(pev), "models/mechgibs.mdl");

           pev->movetype = MOVETYPE_FLY;
           pev->solid = SOLID_NOT;
           pev->takedamage = DAMAGE_NO;

           pev->renderamt = 0;

           m_pPlayer->EnableControl(FALSE);

           SetTouch( NULL );

           //SetThink( IThink );
                pev->origin = m_pBot->pev->origin;
                pev->angles = m_pBot->pev->angles;
                pev->velocity = m_pBot->pev->velocity;
                pev->takedamage = DAMAGE_NO;
                pev->solid = SOLID_NOT;
                pev->owner = m_pPlayer->edict();
                pev->movetype = MOVETYPE_NOCLIP;
                pev->renderamt = 0;
                SET_MODEL(ENT(pev), "models/mechgibs.mdl");
                SET_VIEW (m_pPlayer->edict(), edict());
                m_pPlayer->EnableControl(FALSE);
           pev->nextthink = gpGlobals->time + 0.1;
        }
   }


   void CBotCam :: IThink( void )
   {
   // make sure bot is still in the game...

   if (m_pBot->pev->takedamage != DAMAGE_NO)  // not "kicked"
   {
           {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"other %x %x %x\n",this,m_pBot,m_pPlayer); fclose(fp); }
      UTIL_MakeVectors(m_pBot->pev->v_angle);

      TraceResult tr;
      UTIL_TraceLine(m_pBot->pev->origin + m_pBot->pev->view_ofs,
                     m_pBot->pev->origin + m_pBot->pev->view_ofs + gpGlobals->v_forward * -16 + gpGlobals->v_up * 10,
                     dont_ignore_monsters, m_pBot->edict(), &tr );

      //UTIL_SetOrigin(pev, tr.vecEndPos);
          SET_ORIGIN(edict(), m_pBot->pev->origin + m_pBot->pev->view_ofs);

          this->pev->origin=m_pBot->pev->origin + m_pBot->pev->view_ofs;
      pev->angles = m_pBot->pev->v_angle;

      pev->fixangle = TRUE;

      SET_VIEW (m_pPlayer->edict(), edict());//
                pev->origin = m_pBot->pev->origin;
                pev->angles = m_pBot->pev->angles;
                pev->velocity = m_pBot->pev->velocity;

      pev->nextthink = gpGlobals->time + 0.1; //was without 0.1
   }
   else
   {
           {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"remove %x %x %x\n",this,m_pBot,m_pPlayer); fclose(fp); }
      SET_VIEW (m_pPlayer->edict(), m_pPlayer->edict());

      m_pPlayer->pev->effects &= ~EF_NODRAW;
      m_pPlayer->pev->solid = SOLID_SLIDEBOX;
      m_pPlayer->pev->takedamage = DAMAGE_AIM;
      m_pPlayer->m_iHideHUD &= ~HIDEHUD_ALL;

      m_pPlayer->EnableControl(TRUE);

      m_pPlayer->pBotCam = NULL;  // player's botcam is no longer valid

      m_pBot = NULL;
      m_pPlayer = NULL;
   {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"disc..a"); fclose(fp); }
      REMOVE_ENTITY( ENT(pev) );
          {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"disc..b"); fclose(fp); }
   }
   }


   void CBotCam::Disconnect( void )
   {
        {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"disconnect %x %x %x\n",this,m_pBot,m_pPlayer); fclose(fp); }
   SET_VIEW (m_pPlayer->edict(), m_pPlayer->edict());

   m_pPlayer->pev->effects &= ~EF_NODRAW;
   m_pPlayer->pev->solid = SOLID_SLIDEBOX;
   m_pPlayer->pev->takedamage = DAMAGE_AIM;
   m_pPlayer->m_iHideHUD &= ~HIDEHUD_ALL;

   m_pPlayer->EnableControl(TRUE);

   m_pPlayer->pBotCam = NULL;  // player's botcam is no longer valid

   m_pBot = NULL;
   m_pPlayer = NULL;
   {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"disc..a"); fclose(fp); }
   REMOVE_ENTITY( ENT(pev) );
   {FILE *fp=fopen("bot.txt","a"); fprintf(fp,"disc..b"); fclose(fp); }
   }

 */
