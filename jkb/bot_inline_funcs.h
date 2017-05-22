//
// JK_Botti - be more human!
//
// bot_inline_funcs.h
//

#ifndef BOT_INLINE_FUNCS
#define BOT_INLINE_FUNCS

#include <inttypes.h>
#include <sys/time.h>

// Manual branch optimization for GCC 3.0.0 and newer
#if !defined(__GNUC__) || __GNUC__ < 3
	#define likely(x) (x)
	#define unlikely(x) (x)
#else
	#define likely(x) __builtin_expect((long int)!!(x), true)
	#define unlikely(x) __builtin_expect((long int)!!(x), false)
#endif

//
inline Vector GetGunPosition(edict_t *pEdict)
{
   return (pEdict->v.origin + pEdict->v.view_ofs);
}

inline void UTIL_SelectItem(edict_t *pEdict, const char *item_name)
{
   FakeClientCommand(pEdict, item_name, NULL, NULL);
}

// Overloaded to add IGNORE_GLASS
inline void UTIL_TraceLine( const Vector & vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}

inline void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}

inline void UTIL_TraceHull( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), hullNumber, pentIgnore, ptr );
}

inline void UTIL_TraceMove( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr ) 
{
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), point_hull, pentIgnore, ptr );
}

inline void UTIL_TraceDuck( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr ) 
{
   TRACE_HULL( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), head_hull, pentIgnore, ptr );
}

inline edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
   edict_t  *pentEntity;

   pentEntity = FIND_ENTITY_IN_SPHERE( pentStart, vecCenter, flRadius);

   if (likely(!FNullEnt(pentEntity)))
      return pentEntity;

   return NULL;
}

inline edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
   edict_t *pentEntity;

   pentEntity = FIND_ENTITY_BY_STRING( pentStart, szKeyword, szValue );

   if (likely(!FNullEnt(pentEntity)))
      return pentEntity;
   
   return NULL;
}

inline edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "classname", szName );
}

inline edict_t *UTIL_FindEntityByTarget( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "target", szName );
}

inline edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "targetname", szName );
}

inline double deg2rad(double deg) 
{
   return(deg * (M_PI / 180));
}

inline void BotFixIdealPitch(edict_t *pEdict)
{
   pEdict->v.idealpitch = UTIL_WrapAngle(pEdict->v.idealpitch);
}

inline void BotFixIdealYaw(edict_t *pEdict)
{
   pEdict->v.ideal_yaw = UTIL_WrapAngle(pEdict->v.ideal_yaw);
}

inline Vector UTIL_GetOrigin(edict_t *pEdict)
{
//   if(strncmp(STRING(pEdict->v.classname), "func_", 5) == 0)
   if(unlikely(pEdict->v.solid == SOLID_BSP))
      return VecBModelOrigin(pEdict);

   return pEdict->v.origin; 
}

inline Vector UTIL_GetOriginWithExtent(bot_t &pBot, edict_t *pTarget)
{
   return(UTIL_AdjustOriginWithExtent(pBot, UTIL_GetOrigin(pTarget), pTarget));
}

inline void UTIL_ParticleEffect( const Vector &vecOrigin, const Vector &vecDirection, ULONG ulColor, ULONG ulCount )
{
   PARTICLE_EFFECT( vecOrigin, vecDirection, (float)ulColor, (float)ulCount );
}

inline qboolean FIsClassname(edict_t * pent, const char * cname)
{
   return(likely(pent)?strcmp(STRING(pent->v.classname), cname)==0:FALSE);
}

inline qboolean FIsClassname(const char * cname, edict_t * pent)
{
   return(FIsClassname(pent, cname));
}

inline qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict, edict_t * pOrigin )
{
   edict_t * pHit = NULL;
   return(FVisible(vecOrigin, pEdict, &pHit) || (pOrigin != NULL && pHit == pOrigin));
}

inline qboolean FVisible( const Vector &vecOrigin, edict_t *pEdict )
{
   return(FVisible(vecOrigin, pEdict, (edict_t **)NULL));
}

#endif /*BOT_INLINE_FUNCS*/
