/*
Copyright (C) 2008 Vana Development Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "SkillTimer.h"
#include "Skills.h"

/* Skill Timer */
SkillTimer * SkillTimer::singleton = 0;

void SkillTimer::setSkillTimer(Player *player, int skill, int time) {
	STimer timer;
	timer.id = Timer::Instance()->setTimer(time, this);
	timer.player = player;
	timer.skill = skill;
	timer.time = time;
	timers.push_back(timer);
	act[timer.id] = false;
}

void SkillTimer::setSkillTimer(Player *player, int skill, char *name, short value, int time) {
	SActTimer timer;
	timer.id = Timer::Instance()->setTimer(time, this);
	timer.player = player;
	timer.skill = skill;
	strcpy_s(timer.act, 50, name);
	timer.time = time;
	timer.value = value;
	acttimers.push_back(timer);
	act[timer.id] = true;
}

void SkillTimer::stop(Player *player, int skill) {
	for (size_t i = 0; i < timers.size(); i++) {
		if (player == timers[i].player && timers[i].skill == skill) {
			Timer::Instance()->cancelTimer(timers[i].id);
			break;
		}
	}
}

void SkillTimer::stop(Player *player, int skill, char *name) {
	for (size_t i = 0; i < acttimers.size(); i++) {
		if (player == acttimers[i].player && strcmp(acttimers[i].act, name) == 0 && skill == acttimers[i].skill) {
			Timer::Instance()->cancelTimer(acttimers[i].id);
			break;
		}
	}
}

void SkillTimer::stop(Player *player, bool endSkills) {
	for (size_t i = 0; i < timers.size(); i++) {
		if (player == timers[i].player) {
			if (endSkills)
				Skills::endSkill(player, timers[i].skill);
			Timer::Instance()->cancelTimer(timers[i].id);
			i--;
		}
	} 
	for (size_t i = 0; i < acttimers.size(); i++) { 
		if (player == acttimers[i].player) {
			Timer::Instance()->cancelTimer(acttimers[i].id);
			i--;
		}
	} 
}

int SkillTimer::skillTime(Player *player, int skillid) { // Get skill time
	int timeleft = 0;
	for (size_t i = 0; i < timers.size(); i++) {
		if (player == timers[i].player && timers[i].skill == skillid) {
			timeleft = Timer::Instance()->timeLeft(timers[i].id);
		}
	}
	return timeleft;
}

void SkillTimer::handle(Timer *timer, int id) {
	int skill;
	Player *player;
	if (act[id]) {
		char name[50];
		short value;
		for (size_t i = 0; i < acttimers.size(); i++) {
			if (acttimers[i].id == id) {
				player = acttimers[i].player;
				skill = acttimers[i].skill;
				strcpy_s(name, 50, acttimers[i].act);
				value = acttimers[i].value;
				break;
			}
		}
		if (strcmp(name, "heal") == 0) Skills::heal(player, value, skill);
		else if (strcmp(name, "hurt") == 0) Skills::hurt(player, value, skill);
		// else if (...
	}
	else {
		for (size_t i = 0; i < timers.size(); i++) {
			if (timers[i].id == id) {
				player = timers[i].player;
				skill = timers[i].skill;
				break;
			}
		}
		Skills::endSkill(player, skill);
	}
}

void SkillTimer::remove(int id) {
	if (act[id]) {
		for (size_t i = 0; i < acttimers.size(); i++) {
			if (acttimers[i].id == id) {	
				acttimers.erase(acttimers.begin()+i);	
				return;
			}
		}
	}
	else {
		for (size_t i = 0; i < timers.size(); i++) {
			if (timers[i].id == id) {	
				timers.erase(timers.begin()+i);	
				return;
			}
		}
	}
	act.erase(id);
}

/* Cooldown Timer */
CoolTimer * CoolTimer::singleton = 0;

void CoolTimer::setCoolTimer(Player *player, int skill, int time) {
	CTimer timer;
	timer.id = Timer::Instance()->setTimer(time, this);
	timer.player = player;
	timer.skill = (skill + 10000000);
	timer.time = time;
	timers.push_back(timer);
}

void CoolTimer::stop(Player *player, int skill) {
	for (size_t i = 0; i < timers.size(); i++) {
		if (player == timers[i].player && timers[i].skill == (skill + 10000000)) {
			Timer::Instance()->cancelTimer(timers[i].id);
			break;
		}
	}
}

void CoolTimer::stop(Player *player) {
	for (size_t i = 0; i < timers.size(); i++) {
		if (player == timers[i].player) {
			Timer::Instance()->cancelTimer(timers[i].id);
			i--;
		}
	}
}

void CoolTimer::handle(Timer *timer, int id) {
	int skill;
	Player *player;
	for (size_t i = 0; i < timers.size(); i++) {
		if (timers[i].id == id) {
			player = timers[i].player;
			skill = timers[i].skill;
			break;
		}
	}
	Skills::stopCooldown(player, (skill - 10000000));
}

void CoolTimer::remove(int id) {
	for (size_t i = 0; i < timers.size(); i++) {
		if (timers[i].id == id) {	
			timers.erase(timers.begin() + i);	
			return;
		}
	}
}

int CoolTimer::coolTime(Player *player, int skillid) {
	int timeleft = 0;
	for (size_t i = 0; i < timers.size(); i++) {
		if (player == timers[i].player && timers[i].skill == (skillid + 10000000)) {
			timeleft = Timer::Instance()->timeLeft(timers[i].id);
		}
	}
	return timeleft;
}
