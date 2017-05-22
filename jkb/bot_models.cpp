//
// JK_Botti - be more human!
//
// bot_models.cpp
//

#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"

#ifndef __linux__
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <dirent.h>
#endif


skin_t bot_skins[MAX_SKINS];
int number_skins;

#define VALVE_MAX_SKINS     10
#define GEARBOX_MAX_SKINS   10
#define DMC_MAX_SKINS		10


// default player models for various MODs...
char *default_bot_models[VALVE_MAX_SKINS + GEARBOX_MAX_SKINS + DMC_MAX_SKINS] = {
   "barney", "gina", "gman", "gordon", "helmet",
   "hgrunt", "recon", "robo", "scientist", "zombie",
   
   "beret", "cl_suit", "drill", "fassn", "grunt", 
   "massn", "otis", "recruit", "shephard", "tower"
};

// default names for each of the above player models...
char *default_bot_names[VALVE_MAX_SKINS + GEARBOX_MAX_SKINS + DMC_MAX_SKINS] = {
   "Barney", "Gina", "G-Man", "Gordon", "Helmet",
   "H-Grunt", "Recon", "Robo", "Scientist", "Zombie",
   
   "Beret", "Cleansuit", "Drill", "Female Assassin", "Grunt",
   "Male Assassin", "Otis", "Recruit", "Shephard", "Tower"
};


#ifndef __linux__

// MS-DOS directory wildcard routines...
static HANDLE FindDirectory(HANDLE hFile, char *dirname, int sizeof_dirname, char *dirspec)
{
   WIN32_FIND_DATA pFindFileData;

   if (hFile == NULL)
   {
      hFile = FindFirstFile(dirspec, &pFindFileData);

      if (hFile == INVALID_HANDLE_VALUE)
      {
         hFile = NULL;
	 return hFile; // bugfix
      }

      while (pFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
      {
         if (FindNextFile(hFile, &pFindFileData) == 0)
         {
            FindClose(hFile);
            hFile = NULL;
            return hFile;
         }
      }

      safe_strcopy(dirname, sizeof_dirname, pFindFileData.cFileName);

      return hFile;
   }
   else
   {
      if (FindNextFile(hFile, &pFindFileData) == 0)
      {
         FindClose(hFile);
         hFile = NULL;
         return hFile;
      }

      while (pFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
      {
         if (FindNextFile(hFile, &pFindFileData) == 0)
         {
            FindClose(hFile);
            hFile = NULL;
            return hFile;
         }
      }

      safe_strcopy(dirname, sizeof_dirname, pFindFileData.cFileName);

      return hFile;
   }
}

#else

// Linux directory wildcard routines...
static DIR *FindDirectory(DIR *directory, char *dirname, int sizeof_dirname, char *dirspec)
{
   char pathname[256];
   struct dirent *dirent;
   struct stat stat_str;

   if (directory == NULL)
   {
      if ((directory = opendir(dirspec)) == NULL)
         return NULL;
   }

   while ((dirent = readdir(directory)) != NULL)
   {
      safevoid_snprintf(pathname, sizeof(pathname), "%s/%s", dirspec, dirent->d_name);

      if (stat(pathname, &stat_str) == 0)
      {
         if (stat_str.st_mode & S_IFDIR)
         {
            safe_strcopy(dirname, sizeof_dirname, dirent->d_name);
            return directory;
         }
      }
   }
   
   /* at end of directory? */
   closedir(directory);
   return NULL;
}
#endif


//
void LoadBotModels(void)
{
   char game_dir[256];
   char path[MAX_PATH];
   char search_path[MAX_PATH];
   char dirname[MAX_PATH];
   char filename[MAX_PATH];
   int index;
   struct stat stat_str;
#ifndef __linux__
   HANDLE directory = NULL;
#else
   DIR *directory = NULL;
#endif

   for (index=0; index < MAX_SKINS; index++)
      bot_skins[index].skin_used = FALSE;

   number_skins = VALVE_MAX_SKINS;
   if(submod_id == SUBMOD_OP4)
      number_skins += GEARBOX_MAX_SKINS;

   for (index=0; index < number_skins; index++)
   {
      safe_strcopy(bot_skins[index].model_name, sizeof(bot_skins[index].model_name), default_bot_models[index]);
      safe_strcopy(bot_skins[index].bot_name, sizeof(bot_skins[index].bot_name), default_bot_names[index]);
   }

   // find the directory name of the currently running MOD...
   GetGameDir (game_dir);

   safevoid_snprintf(path, sizeof(path), "%s/models/player", game_dir);

   if (stat(path, &stat_str) != 0)
   {
      // use the valve/models/player directory if no MOD models/player
      strcpy(path, "valve/models/player");
   }

#ifndef __linux__
   safevoid_snprintf(search_path, sizeof(search_path), "%s/*", path);
#else
   strcpy(search_path, path);
#endif

   while ((directory = FindDirectory(directory, dirname, sizeof(dirname), search_path)) != NULL)
   {
      if ((strcmp(dirname, ".") == 0) || (strcmp(dirname, "..") == 0))
         continue;

      safevoid_snprintf(filename, sizeof(filename), "%s/%s/%s.mdl", path, dirname, dirname);

      if (stat(filename, &stat_str) == 0)
      {
         // add this model to the array of models...
         for (index=0; dirname[index]; index++)
            dirname[index] = tolower(dirname[index]);

         // check for duplicate...
         for (index=0; index < number_skins; index++)
         {
            if (strcmp(dirname, bot_skins[index].model_name) == 0)
               break;
         }

         if (index == number_skins)
         {
            // add this model to the bot_skins array...
            safe_strcopy(bot_skins[number_skins].model_name, sizeof(bot_skins[number_skins].model_name), dirname);

            dirname[0] = toupper(dirname[0]);
            safe_strcopy(bot_skins[number_skins].bot_name, sizeof(bot_skins[number_skins].bot_name), dirname);

            number_skins++;
         }
      }

      if (number_skins == MAX_SKINS)
         break;  // break out if max models reached
   }
}

