//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_job_functions.h
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

#ifndef BOT_JOB_FUNCTIONS_H
#define BOT_JOB_FUNCTIONS_H

int JobSeekWaypoint(bot_t *pBot);
int JobGetUnstuck(bot_t *pBot);
int JobRoam(bot_t *pBot);
int JobChat(bot_t *pBot);
int JobReport(bot_t *pBot);
int JobPickUpItem(bot_t *pBot);
int JobPickUpFlag(bot_t *pBot);
int JobPushButton(bot_t *pBot);
int JobUseTeleport(bot_t *pBot);
int JobMaintainObject(bot_t *pBot);
int JobBuildSentry(bot_t *pBot);
int JobBuildDispenser(bot_t *pBot);
int JobBuildTeleport(bot_t *pBot);
int JobBuffAlly(bot_t *pBot);
int JobEscortAlly(bot_t *pBot);
int JobCallMedic(bot_t *pBot);
int JobGetHealth(bot_t *pBot);
int JobGetArmor(bot_t *pBot);
int JobGetAmmo(bot_t *pBot);
int JobDisguise(bot_t *pBot);
int JobFeignAmbush(bot_t *pBot);
int JobSnipe(bot_t *pBot);
int JobGuardWaypoint(bot_t *pBot);
int JobDefendFlag(bot_t *pBot);
int JobGetFlag(bot_t *pBot);
int JobCaptureFlag(bot_t *pBot);
int JobHarrassDefense(bot_t *pBot);
int JobRocketJump(bot_t *pBot);
int JobConcussionJump(bot_t *pBot);
int JobDetpackWaypoint(bot_t *pBot);
int JobPipetrap(bot_t *pBot);
int JobInvestigateArea(bot_t *pBot);
int JobPursueEnemy(bot_t *pBot);
int JobPatrolHome(bot_t *pBot);
int JobSpotStimulus(bot_t *pBot);
int JobAttackBreakable(bot_t *pBot);
int JobAttackTeleport(bot_t *pBot);
int JobSeekBackup(bot_t *pBot);
int JobAvoidEnemy(bot_t *pBot);
int JobAvoidAreaDamage(bot_t *pBot);
int JobInfectedAttack(bot_t *pBot);
int JobBinGrenade(bot_t *pBot);
int JobDrownRecover(bot_t *pBot);
int JobMeleeWarrior(bot_t *pBot);
int JobGraffitiArtist(bot_t *pBot);

#endif // BOT_JOB_FUNCTIONS_H
