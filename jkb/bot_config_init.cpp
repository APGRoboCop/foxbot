//
// JK_Botti - be more human!
//
// bot.cpp
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
#include "waypoint.h"
#include "bot_weapons.h"
#include "bot_skill.h"

#include "bot_config_init.h"

int number_names = 0;
char bot_names[MAX_BOT_NAMES][BOT_NAME_LEN+1];

int num_logos = 0;
char bot_logos[MAX_BOT_LOGOS][16];

void BotLogoInit(void)
{
   FILE *bot_logo_fp;
   char bot_logo_filename[256];
   char logo_buffer[80];
   int length;

   UTIL_BuildFileName_N(bot_logo_filename, sizeof(bot_logo_filename), "addons/jk_botti/jk_botti_logo.cfg", NULL);

   bot_logo_fp = fopen(bot_logo_filename, "r");

   if (bot_logo_fp != NULL)
   {
      UTIL_ConsolePrintf("Loading %s...\n", bot_logo_filename);
      
      while ((num_logos < MAX_BOT_LOGOS) &&
             (fgets(logo_buffer, 80, bot_logo_fp) != NULL))
      {
         length = strlen(logo_buffer);

         if (logo_buffer[length-1] == '\n')
         {
            logo_buffer[length-1] = 0;  // remove '\n'
            length--;
         }

         if (logo_buffer[0] != 0)
         {
            safe_strcopy(bot_logos[num_logos], sizeof(bot_logos[num_logos]), logo_buffer);

            num_logos++;
         }
      }

      fclose(bot_logo_fp);
   }
}


void BotNameInit( void )
{
   FILE *bot_name_fp;
   char bot_name_filename[256];
   int str_index;
   char name_buffer[80];
   int length, index;

   UTIL_BuildFileName_N(bot_name_filename, sizeof(bot_name_filename), "addons/jk_botti/jk_botti_names.txt", NULL);

   bot_name_fp = fopen(bot_name_filename, "r");

   if (bot_name_fp != NULL)
   {
      UTIL_ConsolePrintf("Loading %s...\n", bot_name_filename);
      
      while ((number_names < MAX_BOT_NAMES) &&
             (fgets(name_buffer, 80, bot_name_fp) != NULL))
      {
         length = strlen(name_buffer);

         if (name_buffer[length-1] == '\n')
         {
            name_buffer[length-1] = 0;  // remove '\n'
            length--;
         }

         str_index = 0;
         while (str_index < length)
         {
            if ((name_buffer[str_index] < ' ') || (name_buffer[str_index] > '~') ||
                (name_buffer[str_index] == '"'))
            for (index=str_index; index < length; index++)
               name_buffer[index] = name_buffer[index+1];

            str_index++;
         }

         if (name_buffer[0] != 0)
         {
            safe_strcopy(bot_names[number_names], sizeof(bot_names[number_names]), name_buffer);

            number_names++;
         }
      }

      fclose(bot_name_fp);
   }
}

