//
// JK_Botti - be more human!
//
// bot_func.h
//

#ifndef BOT_FUNC_H
#define BOT_FUNC_H

//prototypes of bot functions...

//bot.cpp:
void BotCreate( const char *skin, const char *name, int skill, int top_color, int bottom_color, int cfg_bot_index );
void BotThink( bot_t &pBot );
void BotCheckTeamplay(void);
void BotKick(bot_t &pBot);
char * GetSpecificTeam(char * teamstr, size_t slen, qboolean get_smallest, qboolean get_largest, qboolean only_count_bots);
void BotReplaceConnectionTime(const char * name, float * timeslot);

// bot_chat.cpp:
void LoadBotChat(void);
void BotChatTaunt(bot_t &pBot, edict_t *victim_edict);
void BotChatWhine(bot_t &pBot);
void BotChatTalk(bot_t &pBot);
void BotChatEndGame(bot_t &pBot);

// bot_combat.cpp:
void BotAimPre( bot_t &pBot );
void BotAimPost( bot_t &pBot );
void free_posdata_list(int idx);
void GatherPlayerData(edict_t * pEdict);
qboolean FPredictedVisible(bot_t &pBot);
void BotUpdateHearingSensitivity(bot_t &pBot);
void BotRemoveEnemy( bot_t &pBot, qboolean b_keep_tracking);
void BotFindEnemy( bot_t &pBot );
void BotShootAtEnemy( bot_t &pBot );
qboolean BotShootTripmine( bot_t &pBot );

// bot_models.cpp:
void LoadBotModels(void);

// bot_navigate.cpp:
float BotChangePitch( bot_t &pBot, float speed );
float BotChangeYaw( bot_t &pBot, float speed );
qboolean BotUpdateTrackSoundGoal( bot_t &pBot );
qboolean BotHeadTowardWaypoint( bot_t &pBot );
void BotOnLadder( bot_t &pBot, float moved_distance );
void BotUnderWater( bot_t &pBot );
void BotUseLift( bot_t &pBot, float moved_distance );
qboolean BotStuckInCorner( bot_t &pBot );
void BotTurnAtWall( bot_t &pBot, TraceResult *tr, qboolean negative );
qboolean BotCantMoveForward( bot_t &pBot, TraceResult *tr );
qboolean BotCanJumpUp( bot_t &pBot, qboolean *bDuckJump );
qboolean BotCanDuckUnder( bot_t &pBot );
qboolean BotEdgeForward( bot_t &pBot, const Vector &v_move_dir );
qboolean BotEdgeRight( bot_t &pBot, const Vector &v_move_dir );
qboolean BotEdgeLeft( bot_t &pBot, const Vector &v_move_dir );
void BotRandomTurn( bot_t &pBot );
qboolean BotCheckWallOnLeft( bot_t &pBot );
qboolean BotCheckWallOnRight( bot_t &pBot );
qboolean BotCheckWallOnBack( bot_t &pBot );
qboolean BotCheckWallOnForward( bot_t &pBot );
void BotLookForDrop( bot_t &pBot );

// commands.cpp:
const cfg_bot_record_t * GetUnusedCfgBotRecord(void);
void FreeCfgBotRecord(void);
int AddToCfgBotRecord(const char *skin, const char *name, int skill, int top_color, int bottom_color);
void ClientCommand( edict_t *pEntity );
void FakeClientCommand(edict_t *pBot, const char *arg1, const char *arg2, const char *arg3);
void jk_botti_ServerCommand(void);
void ProcessBotCfgFile(void);

// dll.cpp:
void jkbotti_ClientPutInServer( edict_t *pEntity );
BOOL jkbotti_ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );

#endif // BOT_FUNC_H

