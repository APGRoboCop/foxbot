//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// linkfunc.cpp
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

//#ifndef MR_META

#include "extdll.h"
#include "util.h"

#include "bot.h"

#ifdef _WIN32
extern HINSTANCE h_Library;
#else
extern void* h_Library;
#endif

void helper_LinkEntity(LINK_ENTITY_FUNC& addr, const char* name, entvars_t* pev) {
	if (addr == nullptr) {
		addr = reinterpret_cast<LINK_ENTITY_FUNC>(GetProcAddress(h_Library, name));
	}

	if (addr != nullptr) { //fixed condition? [APG]RoboCop[CL]
		addr(pev);
	}
}

#define LINK_ENTITY_TO_FUNC(entityName)                                                                                                                                                                                                        \
	C_DLLEXPORT void entityName(entvars_t *pev) {                                                                                                                                                                                               \
		static LINK_ENTITY_FUNC addr;                                                                                                                                                                                                            \
		helper_LinkEntity(addr, #entityName, pev);                                                                                                                                                                                               \
	}

LINK_ENTITY_TO_FUNC(aiscripted_sequence);
LINK_ENTITY_TO_FUNC(ambient_generic);
LINK_ENTITY_TO_FUNC(beam);
LINK_ENTITY_TO_FUNC(bodyque);
LINK_ENTITY_TO_FUNC(building_dispenser);
LINK_ENTITY_TO_FUNC(building_sentrygun);
LINK_ENTITY_TO_FUNC(building_sentrygun_base);
LINK_ENTITY_TO_FUNC(building_teleporter);
LINK_ENTITY_TO_FUNC(button_target);
LINK_ENTITY_TO_FUNC(cine_blood);
LINK_ENTITY_TO_FUNC(cycler);
LINK_ENTITY_TO_FUNC(cycler_prdroid);
LINK_ENTITY_TO_FUNC(cycler_sprite);
LINK_ENTITY_TO_FUNC(cycler_weapon);
LINK_ENTITY_TO_FUNC(cycler_wreckage);
LINK_ENTITY_TO_FUNC(detpack);
LINK_ENTITY_TO_FUNC(dispenser_refill_timer);
LINK_ENTITY_TO_FUNC(env_beam);
LINK_ENTITY_TO_FUNC(env_beverage);
LINK_ENTITY_TO_FUNC(env_blood);
LINK_ENTITY_TO_FUNC(env_bubbles);
LINK_ENTITY_TO_FUNC(env_debris);
LINK_ENTITY_TO_FUNC(env_explosion);
LINK_ENTITY_TO_FUNC(env_fade);
LINK_ENTITY_TO_FUNC(env_funnel);
LINK_ENTITY_TO_FUNC(env_global);
LINK_ENTITY_TO_FUNC(env_glow);
LINK_ENTITY_TO_FUNC(env_laser);
LINK_ENTITY_TO_FUNC(env_lightning);
LINK_ENTITY_TO_FUNC(env_message);
LINK_ENTITY_TO_FUNC(env_render);
LINK_ENTITY_TO_FUNC(env_shake);
LINK_ENTITY_TO_FUNC(env_shooter);
LINK_ENTITY_TO_FUNC(env_sound);
LINK_ENTITY_TO_FUNC(env_spark);
LINK_ENTITY_TO_FUNC(env_sprite);
LINK_ENTITY_TO_FUNC(fireanddie);
LINK_ENTITY_TO_FUNC(func_breakable);
LINK_ENTITY_TO_FUNC(func_button);
LINK_ENTITY_TO_FUNC(func_conveyor);
LINK_ENTITY_TO_FUNC(func_door);
LINK_ENTITY_TO_FUNC(func_door_rotating);
LINK_ENTITY_TO_FUNC(func_friction);
LINK_ENTITY_TO_FUNC(func_guntarget);
LINK_ENTITY_TO_FUNC(func_healthcharger);
LINK_ENTITY_TO_FUNC(func_illusionary);
LINK_ENTITY_TO_FUNC(func_ladder);
LINK_ENTITY_TO_FUNC(func_monsterclip);
LINK_ENTITY_TO_FUNC(func_mortar_field);
LINK_ENTITY_TO_FUNC(func_nobuild);
LINK_ENTITY_TO_FUNC(func_nogrenades);
LINK_ENTITY_TO_FUNC(func_pendulum);
LINK_ENTITY_TO_FUNC(func_plat);
LINK_ENTITY_TO_FUNC(func_platrot);
LINK_ENTITY_TO_FUNC(func_pushable);
LINK_ENTITY_TO_FUNC(func_recharge);
LINK_ENTITY_TO_FUNC(func_rot_button);
LINK_ENTITY_TO_FUNC(func_rotating);
LINK_ENTITY_TO_FUNC(func_tank);
LINK_ENTITY_TO_FUNC(func_tankcontrols);
LINK_ENTITY_TO_FUNC(func_tanklaser);
LINK_ENTITY_TO_FUNC(func_tankmortar);
LINK_ENTITY_TO_FUNC(func_tankrocket);
LINK_ENTITY_TO_FUNC(func_trackautochange);
LINK_ENTITY_TO_FUNC(func_trackchange);
LINK_ENTITY_TO_FUNC(func_tracktrain);
LINK_ENTITY_TO_FUNC(func_train);
LINK_ENTITY_TO_FUNC(func_traincontrols);
LINK_ENTITY_TO_FUNC(func_wall);
LINK_ENTITY_TO_FUNC(func_wall_toggle);
LINK_ENTITY_TO_FUNC(func_water);
LINK_ENTITY_TO_FUNC(ghost);
LINK_ENTITY_TO_FUNC(gibshooter);
LINK_ENTITY_TO_FUNC(grenade);
LINK_ENTITY_TO_FUNC(i_p_t);
LINK_ENTITY_TO_FUNC(i_t_g);
LINK_ENTITY_TO_FUNC(i_t_t);
LINK_ENTITY_TO_FUNC(info_areadef);
LINK_ENTITY_TO_FUNC(info_intermission);
LINK_ENTITY_TO_FUNC(info_landmark);
LINK_ENTITY_TO_FUNC(info_node);
LINK_ENTITY_TO_FUNC(info_node_air);
LINK_ENTITY_TO_FUNC(info_null);
LINK_ENTITY_TO_FUNC(info_player_deathmatch);
LINK_ENTITY_TO_FUNC(info_player_start);
LINK_ENTITY_TO_FUNC(info_player_teamspawn);
LINK_ENTITY_TO_FUNC(info_target);
LINK_ENTITY_TO_FUNC(info_teleport_destination);
LINK_ENTITY_TO_FUNC(info_tf_teamcheck);
LINK_ENTITY_TO_FUNC(info_tf_teamset);
LINK_ENTITY_TO_FUNC(info_tfdetect);
LINK_ENTITY_TO_FUNC(info_tfgoal);
LINK_ENTITY_TO_FUNC(info_tfgoal_timer);
LINK_ENTITY_TO_FUNC(infodecal);
LINK_ENTITY_TO_FUNC(item_airtank);
LINK_ENTITY_TO_FUNC(item_antidote);
LINK_ENTITY_TO_FUNC(item_armor1);
LINK_ENTITY_TO_FUNC(item_armor2);
LINK_ENTITY_TO_FUNC(item_armor3);
LINK_ENTITY_TO_FUNC(item_artifact_envirosuit);
LINK_ENTITY_TO_FUNC(item_artifact_invisibility);
LINK_ENTITY_TO_FUNC(item_artifact_invulnerability);
LINK_ENTITY_TO_FUNC(item_artifact_super_damage);
LINK_ENTITY_TO_FUNC(item_battery);
LINK_ENTITY_TO_FUNC(item_cells);
LINK_ENTITY_TO_FUNC(item_health);
LINK_ENTITY_TO_FUNC(item_healthkit);
LINK_ENTITY_TO_FUNC(item_longjump);
LINK_ENTITY_TO_FUNC(item_rockets);
LINK_ENTITY_TO_FUNC(item_security);
LINK_ENTITY_TO_FUNC(item_shells);
LINK_ENTITY_TO_FUNC(item_sodacan);
LINK_ENTITY_TO_FUNC(item_spikes);
LINK_ENTITY_TO_FUNC(item_suit);
LINK_ENTITY_TO_FUNC(item_tfgoal);
LINK_ENTITY_TO_FUNC(laser_spot);
LINK_ENTITY_TO_FUNC(light);
LINK_ENTITY_TO_FUNC(light_environment);
LINK_ENTITY_TO_FUNC(light_spot);
LINK_ENTITY_TO_FUNC(momentary_door);
LINK_ENTITY_TO_FUNC(momentary_rot_button);
LINK_ENTITY_TO_FUNC(monster_cine2_hvyweapons);
LINK_ENTITY_TO_FUNC(monster_cine2_scientist);
LINK_ENTITY_TO_FUNC(monster_cine2_slave);
LINK_ENTITY_TO_FUNC(monster_cine3_barney);
LINK_ENTITY_TO_FUNC(monster_cine3_scientist);
LINK_ENTITY_TO_FUNC(monster_cine_barney);
LINK_ENTITY_TO_FUNC(monster_cine_panther);
LINK_ENTITY_TO_FUNC(monster_cine_scientist);
LINK_ENTITY_TO_FUNC(monster_furniture);
LINK_ENTITY_TO_FUNC(monster_generic);
LINK_ENTITY_TO_FUNC(monster_hevsuit_dead);
LINK_ENTITY_TO_FUNC(monster_miniturret);
LINK_ENTITY_TO_FUNC(monster_mortar);
LINK_ENTITY_TO_FUNC(monster_sentry);
LINK_ENTITY_TO_FUNC(monster_turret);
LINK_ENTITY_TO_FUNC(multi_manager);
LINK_ENTITY_TO_FUNC(multisource);
LINK_ENTITY_TO_FUNC(node_viewer);
LINK_ENTITY_TO_FUNC(node_viewer_fly);
LINK_ENTITY_TO_FUNC(node_viewer_human);
LINK_ENTITY_TO_FUNC(node_viewer_large);
LINK_ENTITY_TO_FUNC(path_corner);
LINK_ENTITY_TO_FUNC(path_track);
LINK_ENTITY_TO_FUNC(player);
LINK_ENTITY_TO_FUNC(player_loadsaved);
LINK_ENTITY_TO_FUNC(player_weaponstrip);
LINK_ENTITY_TO_FUNC(scripted_sentence);
LINK_ENTITY_TO_FUNC(scripted_sequence);
LINK_ENTITY_TO_FUNC(soundent);
LINK_ENTITY_TO_FUNC(spark_shower);
LINK_ENTITY_TO_FUNC(speaker);
LINK_ENTITY_TO_FUNC(target_cdaudio);
LINK_ENTITY_TO_FUNC(teledeath);
LINK_ENTITY_TO_FUNC(test_effect);
LINK_ENTITY_TO_FUNC(testhull);
LINK_ENTITY_TO_FUNC(tf_ammo_rpgclip);
LINK_ENTITY_TO_FUNC(tf_flame);
LINK_ENTITY_TO_FUNC(tf_flamethrower_burst);
LINK_ENTITY_TO_FUNC(tf_gl_grenade);
LINK_ENTITY_TO_FUNC(tf_ic_rocket);
LINK_ENTITY_TO_FUNC(tf_nailgun_nail);
LINK_ENTITY_TO_FUNC(tf_rpg_rocket);
LINK_ENTITY_TO_FUNC(tf_weapon_ac);
LINK_ENTITY_TO_FUNC(tf_weapon_autorifle);
LINK_ENTITY_TO_FUNC(tf_weapon_axe);
LINK_ENTITY_TO_FUNC(tf_weapon_caltrop);
LINK_ENTITY_TO_FUNC(tf_weapon_caltropgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_concussiongrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_empgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_flamethrower);
LINK_ENTITY_TO_FUNC(tf_weapon_gasgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_genericprimedgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_gl);
LINK_ENTITY_TO_FUNC(tf_weapon_ic);
LINK_ENTITY_TO_FUNC(tf_weapon_knife);
LINK_ENTITY_TO_FUNC(tf_weapon_medikit);
LINK_ENTITY_TO_FUNC(tf_weapon_mirvbomblet);
LINK_ENTITY_TO_FUNC(tf_weapon_mirvgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_nailgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_napalmgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_ng);
LINK_ENTITY_TO_FUNC(tf_weapon_normalgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_pl);
LINK_ENTITY_TO_FUNC(tf_weapon_railgun);
LINK_ENTITY_TO_FUNC(tf_weapon_rpg);
LINK_ENTITY_TO_FUNC(tf_weapon_shotgun);
LINK_ENTITY_TO_FUNC(tf_weapon_sniperrifle);
LINK_ENTITY_TO_FUNC(tf_weapon_spanner);
LINK_ENTITY_TO_FUNC(tf_weapon_superng);
LINK_ENTITY_TO_FUNC(tf_weapon_supershotgun);
LINK_ENTITY_TO_FUNC(tf_weapon_tranq);
LINK_ENTITY_TO_FUNC(timer);
LINK_ENTITY_TO_FUNC(trigger);
LINK_ENTITY_TO_FUNC(trigger_auto);
LINK_ENTITY_TO_FUNC(trigger_autosave);
LINK_ENTITY_TO_FUNC(trigger_camera);
LINK_ENTITY_TO_FUNC(trigger_cdaudio);
LINK_ENTITY_TO_FUNC(trigger_changelevel);
LINK_ENTITY_TO_FUNC(trigger_changetarget);
LINK_ENTITY_TO_FUNC(trigger_counter);
LINK_ENTITY_TO_FUNC(trigger_endsection);
LINK_ENTITY_TO_FUNC(trigger_gravity);
LINK_ENTITY_TO_FUNC(trigger_hurt);
LINK_ENTITY_TO_FUNC(trigger_monsterjump);
LINK_ENTITY_TO_FUNC(trigger_multiple);
LINK_ENTITY_TO_FUNC(trigger_once);
LINK_ENTITY_TO_FUNC(trigger_push);
LINK_ENTITY_TO_FUNC(trigger_relay);
LINK_ENTITY_TO_FUNC(trigger_teleport);
LINK_ENTITY_TO_FUNC(trigger_transition);
LINK_ENTITY_TO_FUNC(weapon_crowbar);
LINK_ENTITY_TO_FUNC(weapon_handgrenade);
LINK_ENTITY_TO_FUNC(weaponbox);
LINK_ENTITY_TO_FUNC(world_items);
LINK_ENTITY_TO_FUNC(worldspawn);
LINK_ENTITY_TO_FUNC(xen_hair);
LINK_ENTITY_TO_FUNC(xen_hull);
LINK_ENTITY_TO_FUNC(xen_plantlight);
LINK_ENTITY_TO_FUNC(xen_spore_large);
LINK_ENTITY_TO_FUNC(xen_spore_medium);
LINK_ENTITY_TO_FUNC(xen_spore_small);
LINK_ENTITY_TO_FUNC(xen_tree);
LINK_ENTITY_TO_FUNC(xen_ttrigger);