//
// JK_Botti - be more human!
//
// bot_config_init.h
//

#define MAX_BOT_NAMES 100
#define MAX_BOT_LOGOS 100

extern int number_names;
extern char bot_names[MAX_BOT_NAMES][BOT_NAME_LEN+1];

extern int num_logos;
extern char bot_logos[MAX_BOT_LOGOS][16];

void BotLogoInit(void);
void BotNameInit(void);
