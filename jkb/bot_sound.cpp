//
// JK_Botti - be more human!
//
// bot_sound.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <malloc.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "waypoint.h"
#include "bot_sound.h"
#include "player.h"

static CSoundEnt sSoundEnt;
CSoundEnt *pSoundEnt = &sSoundEnt;

//
void SaveSound(edict_t * pEdict, const Vector & origin, int volume, int channel, float flDuration) 
{
   // save player sounds to player list
   int idx = ENTINDEX(pEdict) - 1;
   if(idx >= 0 && idx < gpGlobals->maxClients)
   {
      CSoundEnt::InsertSound(pEdict, channel, origin, volume, flDuration, UTIL_GetBotIndex(pEdict));
      
      return;
   }
      
   float min_distance = 64.0f;
   int bot_index = -1;
   
   // check if bot is close to sound and mark as owner if it is
   for (int i = 0; i < 32; i++)
   {
      if(bots[i].is_used)
      {
         // is owner of object?
         if(pEdict->v.owner == bots[i].pEdict)
         {
            bot_index = i;
            break;
         }
         
         float distance = (bots[i].pEdict->v.origin - origin).Length();
         
         if(distance < min_distance)
         {
            min_distance = distance;
            bot_index = i;
         }
      }
   }
   
   if(min_distance < 64.0f)
   {
      pEdict = bots[bot_index].pEdict;
      channel |= 0x1000;
   }
   
   if(min_distance < 64.0f || bot_index != -1)
   {
      //UTIL_ConsolePrintf("[%s] InsertSound(): min_distance: %.1f, bot_index: %d", bots[bot_index].name, min_distance, bot_index);
   }
   
   CSoundEnt::InsertSound(pEdict, channel, origin, volume, flDuration, bot_index);
}

//=========================================================
// CSound - Clear - zeros all fields for a sound
//=========================================================
void CSound :: Clear ( void )
{
   m_vecOrigin = Vector(0, 0, 0);
   m_iVolume = 0;
   m_flExpireTime = 0;
   m_iNext = SOUNDLIST_EMPTY;
   m_iNextAudible = 0;
   m_pEdict = NULL;
   m_iBotOwner = -1;
   m_iChannel = 0;
}

//=========================================================
// Reset - clears the volume, origin, and type for a sound,
// but doesn't expire or unlink it. 
//=========================================================
void CSound :: Reset ( void )
{
   m_vecOrigin = Vector(0, 0, 0);
   m_iVolume = 0;
   m_iNext = SOUNDLIST_EMPTY;
}

//=========================================================
// Spawn 
//=========================================================
void CSoundEnt :: Spawn( void )
{
   Initialize();

   m_nextthink = gpGlobals->time + 1; 
}

//=========================================================
// Think - at interval, the entire active sound list is checked
// for sounds that have ExpireTimes less than or equal
// to the current world time, and these sounds are deallocated.
//=========================================================
void CSoundEnt :: Think ( void )
{
   int iSound;
   int iPreviousSound;
   float add_time = 1.0f/15.0f;
   
   m_nextthink = gpGlobals->time + add_time;// how often to check the sound list.

   iPreviousSound = SOUNDLIST_EMPTY;
   iSound = m_iActiveSound; 

   while ( iSound != SOUNDLIST_EMPTY )
   {
      if ( m_SoundPool[ iSound ].m_flExpireTime <= gpGlobals->time )
      {
         int iNext = m_SoundPool[ iSound ].m_iNext;

         // move this sound back into the free list
         FreeSound( iSound, iPreviousSound );

         iSound = iNext;
      }
      else
      {
         iPreviousSound = iSound;
                  
         if(!FNullEnt(m_SoundPool[ iSound ].m_pEdict))
         {
            // update origin of moving sound
            if(FClassnameIs(m_SoundPool[ iSound ].m_pEdict, "rpg_rocket"))
               m_SoundPool[ iSound ].m_vecOrigin = m_SoundPool[ iSound ].m_pEdict->v.origin;
         }
         else
            m_SoundPool[ iSound ].m_pEdict = NULL;
         
         iSound = m_SoundPool[ iSound ].m_iNext;
      }
   }

   if ( m_fShowReport )
   {
      UTIL_ConsolePrintf( "Soundlist: %d / %d  (%d)\n", ISoundsInList( SOUNDLISTTYPE_ACTIVE ), ISoundsInList( SOUNDLISTTYPE_FREE ), ISoundsInList( SOUNDLISTTYPE_ACTIVE ) - m_cLastActiveSounds );
      m_cLastActiveSounds = ISoundsInList ( SOUNDLISTTYPE_ACTIVE );
   }

   //
   if( m_bDebug == 2 )
   {
      CSound *pCurrentSound;
      
      for(iSound = ActiveList(); iSound != SOUNDLIST_EMPTY; iSound = pCurrentSound->m_iNext)
      {
         pCurrentSound = CSoundEnt::SoundPointerForIndex( iSound );
      
         if(pCurrentSound->m_iVolume <= 0)
            continue;
         
         int idx = ENTINDEX(m_SoundPool[ iSound ].m_pEdict) - 1;
         qboolean is_player = (idx >= 0 && idx < gpGlobals->maxClients);
            
         UTIL_ParticleEffect ( pCurrentSound->m_vecOrigin, Vector(0, 0, 0), (is_player) ? 150 : 250, 25 ); 
      }
   }
}

//=========================================================
// FreeSound - clears the passed active sound and moves it 
// to the top of the free list. TAKE CARE to only call this
// function for sounds in the Active list!!
//=========================================================
void CSoundEnt :: FreeSound ( int iSound, int iPrevious )
{
   if ( !pSoundEnt )
   {
      // no sound ent!
      return;
   }

   if ( iPrevious != SOUNDLIST_EMPTY )
   {
      // iSound is not the head of the active list, so
      // must fix the index for the Previous sound
//      pSoundEnt->m_SoundPool[ iPrevious ].m_iNext = m_SoundPool[ iSound ].m_iNext;
      pSoundEnt->m_SoundPool[ iPrevious ].m_iNext = pSoundEnt->m_SoundPool[ iSound ].m_iNext;
   }
   else 
   {
      // the sound we're freeing IS the head of the active list.
      pSoundEnt->m_iActiveSound = pSoundEnt->m_SoundPool [ iSound ].m_iNext;
   }

   // make iSound the head of the Free list.
   pSoundEnt->m_SoundPool[ iSound ].m_iNext = pSoundEnt->m_iFreeSound;
   pSoundEnt->m_iFreeSound = iSound;
}

//=========================================================
// IAllocSound - moves a sound from the Free list to the 
// Active list returns the index of the alloc'd sound
//=========================================================
int CSoundEnt :: IAllocSound( void )
{
   int iNewSound;
   
   if ( m_iFreeSound == SOUNDLIST_EMPTY )
   {
      // no free sound!
      if(m_bDebug)
         UTIL_ConsolePrintf( "Free Sound List is full!\n" );
      return SOUNDLIST_EMPTY;
   }

   // there is at least one sound available, so move it to the
   // Active sound list, and return its SoundPool index.
   
   iNewSound = m_iFreeSound;// copy the index of the next free sound

   m_iFreeSound = m_SoundPool[ m_iFreeSound ].m_iNext;// move the index down into the free list. 

   m_SoundPool[ iNewSound ].m_iNext = m_iActiveSound;// point the new sound at the top of the active list.

   m_iActiveSound = iNewSound;// now make the new sound the top of the active list. You're done.

   return iNewSound;
}

//=========================================================
// InsertSound - Allocates a free sound and fills it with 
// sound info.
//=========================================================
void CSoundEnt :: InsertSound ( edict_t* pEdict, int channel, const Vector &vecOrigin, int iVolume, float flDuration, int iBotOwner )
{
   int iThisSound;

   if ( !pSoundEnt )
   {
      // no sound ent!
      return;
   }
   
   CSound *pSound;
   
   if(pEdict)
   {
      pSound = GetEdictChannelSound( pEdict, channel );
   }
   else
   {
      iThisSound = pSoundEnt->IAllocSound();

      if ( iThisSound == SOUNDLIST_EMPTY )
      {
         if(pSoundEnt->m_bDebug)
            UTIL_ConsolePrintf( "Could not AllocSound() for InsertSound() (DLL)\n" );
         return;
      }

      pSound = CSoundEnt::SoundPointerForIndex( iThisSound );
   }
   
   if(pSound)
   {
      pSound->m_vecOrigin = vecOrigin;
      pSound->m_iVolume = iVolume;
      pSound->m_flExpireTime = gpGlobals->time + flDuration;
      pSound->m_pEdict = pEdict;
      pSound->m_iChannel = channel;
      pSound->m_iBotOwner = iBotOwner;
   }
   
   if(pSoundEnt->m_bDebug == 2)
      UTIL_ParticleEffect ( vecOrigin, Vector(0, 0, 0), 50, 25 ); 
}

//=========================================================
// GetClientChannelSound - Get existing active sound or
// create new one.
//=========================================================
CSound *CSoundEnt::GetEdictChannelSound( edict_t * pEdict, int iChannel )
{
   int iSound;
   
   for(iSound = pSoundEnt->ActiveList(); iSound != SOUNDLIST_EMPTY; iSound = pSoundEnt->m_SoundPool[ iSound ].m_iNext)
      if(pEdict == pSoundEnt->m_SoundPool[ iSound ].m_pEdict)
         if(iChannel == 0 || iChannel == pSoundEnt->m_SoundPool[ iSound ].m_iChannel)
            break;
   
   if(iSound == SOUNDLIST_EMPTY)
   {
      if(iChannel == 0)
         return NULL;
      
      iSound = pSoundEnt->IAllocSound();
      
      if ( iSound == SOUNDLIST_EMPTY )
      {
         if(pSoundEnt->m_bDebug)
            UTIL_ConsolePrintf( "Could not AllocSound() for GetEdictChannelSound() (DLL)\n" );
         return NULL;
      }
      
      CSound *pSound = CSoundEnt::SoundPointerForIndex( iSound );
      
      if(pSound)
      {
         pSoundEnt->m_SoundPool[ iSound ].m_pEdict = pEdict;
         pSoundEnt->m_SoundPool[ iSound ].m_iChannel = iChannel;
      }
   }
   
   return &pSoundEnt->m_SoundPool[ iSound ];
}

//=========================================================
// Initialize - clears all sounds and moves them into the 
// free sound list.
//=========================================================
void CSoundEnt :: Initialize ( void )
{
   int i;
   
#ifdef _DEBUG
   m_bDebug = TRUE;
#else
   m_bDebug = FALSE;
#endif

   m_cLastActiveSounds = 0;
   m_iFreeSound = 0;
   m_iActiveSound = SOUNDLIST_EMPTY;

   for ( i = 0 ; i < MAX_WORLD_SOUNDS ; i++ )
   {
      // clear all sounds, and link them into the free sound list.
      m_SoundPool[ i ].Clear();
      m_SoundPool[ i ].m_iNext = i + 1;
   }

   m_SoundPool[ i - 1 ].m_iNext = SOUNDLIST_EMPTY;// terminate the list here.

   if ( CVAR_GET_FLOAT("displaysoundlist") == 1 )
   {
      m_fShowReport = TRUE;
   }
   else
   {
      m_fShowReport = FALSE;
   }
}

//=========================================================
// ISoundsInList - returns the number of sounds in the desired
// sound list.
//=========================================================
int CSoundEnt :: ISoundsInList ( int iListType )
{
   int i;
   int iThisSound;

   if ( iListType == SOUNDLISTTYPE_FREE )
   {
      iThisSound = m_iFreeSound;
   }
   else if ( iListType == SOUNDLISTTYPE_ACTIVE )
   {
      iThisSound = m_iActiveSound;
   }
   else
   {
      if(m_bDebug)
         UTIL_ConsolePrintf( "Unknown Sound List Type!\n" );
      return 0;
   }

   if ( iThisSound == SOUNDLIST_EMPTY )
   {
      return 0;
   }

   i = 0;

   while ( iThisSound != SOUNDLIST_EMPTY )
   {
      i++;

      iThisSound = m_SoundPool[ iThisSound ].m_iNext;
   }

   return i;
}

//=========================================================
// ActiveList - returns the head of the active sound list
//=========================================================
int CSoundEnt :: ActiveList ( void )
{
   if ( !pSoundEnt )
   {
      return SOUNDLIST_EMPTY;
   }

   return pSoundEnt->m_iActiveSound;
}

//=========================================================
// FreeList - returns the head of the free sound list
//=========================================================
int CSoundEnt :: FreeList ( void )
{
   if ( !pSoundEnt )
   {
      return SOUNDLIST_EMPTY;
   }

   return pSoundEnt->m_iFreeSound;
}

//=========================================================
// SoundPointerForIndex - returns a pointer to the instance
// of CSound at index's position in the sound pool.
//=========================================================
CSound* CSoundEnt :: SoundPointerForIndex( int iIndex )
{
   if ( !pSoundEnt )
   {
      return NULL;
   }

   if ( iIndex > ( MAX_WORLD_SOUNDS - 1 ) )
   {
      if(pSoundEnt->m_bDebug)
         UTIL_ConsolePrintf( "SoundPointerForIndex() - Index too large!\n" );
      return NULL;
   }

   if ( iIndex < 0 )
   {
      if(pSoundEnt->m_bDebug)
         UTIL_ConsolePrintf( "SoundPointerForIndex() - Index < 0!\n" );
      return NULL;
   }

   return &pSoundEnt->m_SoundPool[ iIndex ];
}

//=========================================================
// Clients are numbered from 1 to MAXCLIENTS, but the client
// reserved sounds in the soundlist are from 0 to MAXCLIENTS - 1,
// so this function ensures that a client gets the proper index
// to his reserved sound in the soundlist.
//=========================================================
int CSoundEnt :: ClientSoundIndex ( edict_t *pClient )
{
   int iReturn = ENTINDEX( pClient ) - 1;

   if ( iReturn < 0 || iReturn >= gpGlobals->maxClients )
   {
      if(pSoundEnt->m_bDebug)
         UTIL_ConsolePrintf( "** ClientSoundIndex returning a bogus value! **\n" );
   }

   return iReturn;
}
