//
// JK_Botti - be more human!
//
// bot_sound.h
//

#ifndef BOT_SOUND_H
#define BOT_SOUND_H

#define	MAX_WORLD_SOUNDS	1024 	// maximum number of sounds handled by the world at one time.

#define SOUNDLIST_EMPTY		-1

#define SOUNDLISTTYPE_FREE	1	// identifiers passed to functions that can operate on either list, to indicate which list to operate on.
#define SOUNDLISTTYPE_ACTIVE 	2

//=========================================================
// CSound - an instance of a sound in the world.
//=========================================================
class CSound
{
public:
	void	Clear ( void );
	void	Reset ( void );

	Vector	  m_vecOrigin;		// sound's location in space
	int	  m_iVolume;		// how loud the sound is
	edict_t * m_pEdict;
	int       m_iChannel;
	int       m_iBotOwner;

	float	  m_flExpireTime;	// when the sound should be purged from the list
	int	  m_iNext;		// index of next sound in this list ( Active or Free )
	int	  m_iNextAudible;	// temporary link that monsters use to build a list of audible sounds
};

//=========================================================
// CSoundEnt - a single instance of this entity spawns when
// the world spawns. The SoundEnt's job is to update the 
// world's Free and Active sound lists.
//=========================================================
class CSoundEnt
{
public:
	void 		Spawn( void );
	void 		Think( void );
	void 		Initialize ( void );
	
	static void	InsertSound ( edict_t * pEdict, int channel, const Vector &vecOrigin, int iVolume, float flDuration, int iBotOwner );
	static void	FreeSound ( int iSound, int iPrevious );
	static int	ActiveList( void );// return the head of the active list
	static int	FreeList( void );// return the head of the free list
	static CSound*	SoundPointerForIndex( int iIndex );// return a pointer for this index in the sound list
	static int	ClientSoundIndex ( edict_t *pClient );
	static CSound*  GetEdictChannelSound( edict_t *pEdict, int iChannel );

	BOOL		IsEmpty( void ) { return m_iActiveSound == SOUNDLIST_EMPTY; }
	int		ISoundsInList ( int iListType );
	int		IAllocSound ( void );
	
	int		m_iFreeSound;	// index of the first sound in the free sound list
	int		m_iActiveSound; // indes of the first sound in the active sound list
	int		m_cLastActiveSounds; // keeps track of the number of active sounds at the last update. (for diagnostic work)
	
	BOOL		m_fShowReport; // if true, dump information about free/active sounds.
	BOOL    	m_bDebug;
	float   	m_nextthink;
	
private:
	CSound		m_SoundPool[ MAX_WORLD_SOUNDS ];
};

extern CSoundEnt *pSoundEnt;

extern void SaveSound(edict_t * pEdict, const Vector & origin, int volume, int channel, float flDuration);

#endif
