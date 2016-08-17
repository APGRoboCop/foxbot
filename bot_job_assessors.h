//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_job_assessors.h
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

#ifndef JOB_ASSESSOR_FUNCS_H
#define JOB_ASSESSOR_FUNCS_H

// a job with this priority level should be uninterruptable
#define PRIORITY_MAXIMUM (INT_MAX)

// a job with this priority level should be removed from the buffer
#define PRIORITY_NONE (INT_MIN)

int assess_JobSeekWaypoint(const bot_t *pBot, const job_struct &r_job);
int assess_JobGetUnstuck(const bot_t *pBot, const job_struct &r_job);
int assess_JobRoam(const bot_t *pBot, const job_struct &r_job);
int assess_JobChat(const bot_t *pBot, const job_struct &r_job);
int assess_JobReport(const bot_t *pBot, const job_struct &r_job);
int assess_JobPickUpItem(const bot_t *pBot, const job_struct &r_job);
int assess_JobPickUpFlag(const bot_t *pBot, const job_struct &r_job);
int assess_JobPushButton(const bot_t *pBot, const job_struct &r_job);
int assess_JobUseTeleport(const bot_t *pBot, const job_struct &r_job);
int assess_JobMaintainObject(const bot_t *pBot, const job_struct &r_job);
int assess_JobBuildSentry(const bot_t *pBot, const job_struct &r_job);
int assess_JobBuildDispenser(const bot_t *pBot, const job_struct &r_job);
int assess_JobBuildTeleport(const bot_t *pBot, const job_struct &r_job);
int assess_JobBuffAlly(const bot_t *pBot, const job_struct &r_job);
int assess_JobEscortAlly(const bot_t *pBot, const job_struct &r_job);
int assess_JobCallMedic(const bot_t *pBot, const job_struct &r_job);
int assess_JobGetHealth(const bot_t *pBot, const job_struct &r_job);
int assess_JobGetArmor(const bot_t *pBot, const job_struct &r_job);
int assess_JobGetAmmo(const bot_t *pBot, const job_struct &r_job);
int assess_JobDisguise(const bot_t *pBot, const job_struct &r_job);
int assess_JobFeignAmbush(const bot_t *pBot, const job_struct &r_job);
int assess_JobSnipe(const bot_t *pBot, const job_struct &r_job);
int assess_JobGuardWaypoint(const bot_t *pBot, const job_struct &r_job);
int assess_JobDefendFlag(const bot_t *pBot, const job_struct &r_job);
int assess_JobGetFlag(const bot_t *pBot, const job_struct &r_job);
int assess_JobCaptureFlag(const bot_t *pBot, const job_struct &r_job);
int assess_JobHarrassDefense(const bot_t *pBot, const job_struct &r_job);
int assess_JobRocketJump(const bot_t *pBot, const job_struct &r_job);
int assess_JobConcussionJump(const bot_t *pBot, const job_struct &r_job);
int assess_JobDetpackWaypoint(const bot_t *pBot, const job_struct &r_job);
int assess_JobPipetrap(const bot_t *pBot, const job_struct &r_job);
int assess_JobInvestigateArea(const bot_t *pBot, const job_struct &r_job);
int assess_JobPursueEnemy(const bot_t *pBot, const job_struct &r_job);
int assess_JobPatrolHome(const bot_t *pBot, const job_struct &r_job);
int assess_JobSpotStimulus(const bot_t *pBot, const job_struct &r_job);
int assess_JobAttackBreakable(const bot_t *pBot, const job_struct &r_job);
int assess_JobAttackTeleport(const bot_t *pBot, const job_struct &r_job);
int assess_JobSeekBackup(const bot_t *pBot, const job_struct &r_job);
int assess_JobAvoidEnemy(const bot_t *pBot, const job_struct &r_job);
int assess_JobAvoidAreaDamage(const bot_t *pBot, const job_struct &r_job);
int assess_JobInfectedAttack(const bot_t *pBot, const job_struct &r_job);
int assess_JobBinGrenade(const bot_t *pBot, const job_struct &r_job);
int assess_JobDrownRecover(const bot_t *pBot, const job_struct &r_job);
int assess_JobMeleeWarrior(const bot_t *pBot, const job_struct &r_job);
int assess_JobGraffitiArtist(const bot_t *pBot, const job_struct &r_job);


#endif // JOB_ASSESSOR_FUNCS_H
