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
#ifndef NPCS_H
#define NPCS_H

#include <hash_map>
#include <vector>
#include <string>

using namespace std;
using namespace stdext;

class Player;
class Packet;
class ReadPacket;

struct NPCInfo {
	int id;
	short x;
	short cy;
	short fh;
	short rx0;
	short rx1; 
};
typedef vector<NPCInfo> NPCsInfo;

namespace NPCs {
	void handleNPC(Player *player, ReadPacket *packet);
	void handleQuestNPC(Player *player, int npcid, bool start);
	void handleNPCIn(Player *player, ReadPacket *packet);
};

class NPC {
private:
	int npcid;
	Player *player;
	char text[1000];
	int state;
	int selected;
	bool cend;
	int getnum;
	string gettext;
	bool isquest;
	bool isstart;
	hash_map <string, int> vars;
public:
	NPC(int npcid, Player *player, bool isquest = false);
	~NPC();
	void addText(const char *text) {
		strcat_s(this->text, strlen(text)+1+strlen(this->text), text);
	}
	void addChar(char cha) {
		char temp[2]={0};
		temp[0] = cha;
		addText(temp);
	}

	Packet npcPacket(char type);
	void sendSimple();
	void sendYesNo();
	void sendNext();
	void sendBackNext();
	void sendBackOK();
	void sendOK();
	void sendAcceptDecline();
	void sendGetText();
	void sendGetNumber(int def, int min, int max);
	void sendStyle(int styles[], char size);
	bool isQuest() {
		return isquest;
	}
	bool isStart() {
		return isstart;
	}
	void setIsStart(bool what) {
		isstart = what;
	}
	void setState(int state) {
		this->state = state;
	}
	int getState() {
		return state;
	}
	int getSelected() {
		return selected;
	}
	void setSelected(int selected) {
		this->selected = selected;
	}
	void setGetNumber(int num) {
		this->getnum = num;
	}
	int getNumber() {
		return getnum;
	}
	void setGetText(const string &text) {
		gettext = text;
	}
	string & getText() {
		return gettext;
	}
	void end() {
		cend = true;
	}
	bool isEnd() {
		return cend;
	}
	int getNpcID() {
		return npcid;
	}
	Player * getPlayer() {
		return player;
	}
	void showShop();
	void setVariable(const string &name, int val) {
		vars[name] = val;
	}
	int getVariable(const string &name) {
		if (vars.find(name) == vars.end())
			return 0;
		else
			return vars[name];
	}
};

#endif
