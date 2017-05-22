//
// JK_Botti - be more human!
//
// bot_client.h
//


void BotClient_Valve_WeaponList(void *p, int bot_index);

void BotClient_Valve_CurrentWeapon(void *p, int bot_index);
void PlayerClient_Valve_CurrentWeapon(void *p, int player_index);

void BotClient_Valve_AmmoX(void *p, int bot_index);

void BotClient_Valve_AmmoPickup(void *p, int bot_index);

void BotClient_Valve_WeaponPickup(void *p, int bot_index);

void BotClient_Valve_ItemPickup(void *p, int bot_index);

void BotClient_Valve_Health(void *p, int bot_index);

void BotClient_Valve_Battery(void *p, int bot_index);

void BotClient_Valve_Damage(void *p, int bot_index);

void BotClient_Valve_DeathMsg(void *p, int bot_index);

void BotClient_Valve_ScreenFade(void *p, int bot_index);
