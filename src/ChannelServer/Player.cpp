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
#include "Player.h"
#include "Drops.h"
#include "Inventory.h"
#include "Maps.h"
#include "Mobs.h"
#include "NPCs.h"
#include "Reactors.h"
#include "Players.h"
#include "MySQLM.h"
#include "PlayerPacket.h"
#include "Levels.h"
#include "Skills.h"
#include "Quests.h"
#include "Fame.h"
#include "BufferUtilities.h"
#include "ChannelServer.h"
#include "RecvHeader.h"
#include "WorldServerConnectPlayer.h"
#include "WorldServerConnectPlayerPacket.h"
#include "ServerPacket.h"
#include "Connectable.h"

int distPos(Pos pos1, Pos pos2){
	return (int)sqrt(pow((float)(pos1.x+pos2.x), 2)+pow((float)(pos1.y+pos2.y), 2));
}

Player::~Player(){
	if(isconnect){
		if (save_on_dc)
			save();
		Skills::stopTimerPlayer(this);
		Inventory::stopTimerPlayer(this);
		Maps::removePlayer(this);
		Players::deletePlayer(this);
		setOnline(false);
		isconnect = false;
		WorldServerConnectPlayerPacket::removePlayer(ChannelServer::Instance()->getWorldPlayer(), id);
	}
}

void Player::realHandleRequest(unsigned char* buf, int len){
	short header = buf[0] + buf[1]*0x100;
	switch(header){  
		case RECV_CHANNEL_LOGIN: playerConnect(buf+2); break;
		case RECV_NPC_TALK_CONT: NPCs::handleNPCIn(this, buf+2); break;
		case RECV_SHOP_ENTER: Inventory::useShop(this, buf+2); break;
		case RECV_NPC_TALK: NPCs::handleNPC(this, buf+2); break;
		case RECV_CHANGE_CHANNEL: changeChannel(buf[2]); break;
		case RECV_DAMAGE_PLAYER: Players::damagePlayer(this, buf+2); break;
		case RECV_STOP_CHAIR: Inventory::stopChair(this, buf+2); break;
		case RECV_CHAT: Players::chatHandler(this, buf+2); break;
		case RECV_USE_CHAIR: Inventory::useChair(this, buf+2); break;
		case RECV_USE_ITEM_EFFECT: Inventory::useItemEffect(this, buf+2); break;
		case RECV_DAMAGE_MOB_SPELL: Mobs::damageMobSpell(this, buf+2); break;
		case RECV_CHANGE_MAP: Maps::moveMap(this, buf+2); break;
		case RECV_MOVE_PLAYER: Players::handleMoving(this, buf+2, len-2); break;
		case RECV_DAMAGE_MOB_RANGED: Mobs::damageMobRanged(this, buf+2, len-2); break;
		case RECV_GET_PLAYER_INFO: Players::getPlayerInfo(this, buf+2); break;
		case RECV_CHANGE_MAP_SPECIAL: Maps::moveMapS(this, buf+2); break; // Portals that cause scripted events
		case RECV_USE_SUMMON_BAG: Inventory::useSummonBag(this, buf+2); break;
		case RECV_ADD_SKILL: Skills::addSkill(this, buf+2); break;
		case RECV_CANCEL_SKILL: Skills::cancelSkill(this, buf+2); break;
		case RECV_USE_SKILL: Skills::useSkill(this, buf+2); break;
		case RECV_USE_MEGAPHONE: Inventory::useMegaphone(this, buf+2); break;
		case RECV_COMMAND: Players::commandHandler(this, buf+2); break;
		case RECV_DAMAGE_MOB: Mobs::damageMob(this, buf+2); break;
		case RECV_FACE_EXPERIMENT: Players::faceExperiment(this, buf+2); break;
		case RECV_HIT_REACTOR: Reactors::hitReactor(this, buf+2); break;
		case RECV_MOVE_ITEM: Inventory::itemMove(this, buf+2); break;
		case RECV_USE_ITEM: Inventory::useItem(this, buf+2); break;
		case RECV_CANCEL_ITEM: Inventory::cancelItem(this, buf+2); break;
		case RECV_USE_SKILLBOOK: Inventory::useSkillbook(this, buf+2); break; // Skillbooks
		case RECV_USE_RETURN_SCROLL: Inventory::useReturnScroll(this, buf+2); break; 
		case RECV_USE_SCROLL: Inventory::useScroll(this, buf+2); break;
		case RECV_ADD_STAT: Levels::addStat(this, buf+2); break;
		case RECV_HEAL_PLAYER: Players::healPlayer(this, buf+2); break;
		case RECV_DROP_MESO: Drops::dropMesos(this ,buf+2); break;
		case RECV_FAME: Fame::handleFame(this, buf+2); break;
		case RECV_GET_QUEST: Quests::getQuest(this, buf+2); break;
		case RECV_KEYMAP: changeKey(buf+2);
		case RECV_LOOT_ITEM: Drops::lootItem(this, buf+2); break;
		case RECV_CONTROL_MOB: Mobs::monsterControl(this, buf+2, len-2); break;
		case RECV_CONTROL_MOB_SKILL: Mobs::monsterControlSkill(this, buf+2); break;
	}
}

void Player::playerConnect(unsigned char *packet){
	int id = BufferUtilities::getInt(packet);
	if (!Connectable::Instance()->checkPlayer(id)) {
		//hacking
		disconnect();
		return;
	}
	setPlayerid(id);
	inv = new PlayerInventory();
	skills = new PlayerSkills();
	skills->setPlayer(this);
	quests = new PlayerQuests();
	quests->setPlayer(this);

	mysqlpp::Query query = db.query();
	query << "SELECT characters.*, users.gm FROM characters LEFT JOIN users on characters.userid = users.id WHERE characters.id = " << mysqlpp::quote << getPlayerid();
	mysqlpp::StoreQueryResult res = query.store();

	if (res.empty()) {
		//hacking
		disconnect();
		return;
	}

	strcpy_s(name, res[0]["name"]);
	gender = (unsigned char) res[0]["gender"];
	skin = (unsigned char) res[0]["skin"];
	eyes = res[0]["eyes"];
	hair = res[0]["hair"];
	level = (unsigned char) res[0]["level"];
	job = (short) res[0]["job"];
	str = (short) res[0]["str"];
	dex = (short) res[0]["dex"];
	intt = (short) res[0]["intt"];
	luk = (short) res[0]["luk"];
	hp = (short) res[0]["chp"];
	rmhp = mhp = (short) res[0]["mhp"];
	mp = (short) res[0]["cmp"];
	rmmp = mmp = (short) res[0]["mmp"];
	hpmp_ap = (short) res[0]["hpmp_ap"];
	ap = (short) res[0]["ap"];
	sp = (short) res[0]["sp"];
	exp = res[0]["exp"];
	fame = (short) res[0]["fame"];
	map = res[0]["map"];
	mappos = (unsigned char) res[0]["pos"];
	gm = res[0]["gm"];

	inv->setMesosStart(res[0]["mesos"]);
	inv->setPlayer(this);

	query << "SELECT * FROM equip WHERE charid = " << mysqlpp::quote << getPlayerid();
	res = query.store();

	for (size_t i = 0; i < res.num_rows(); ++i) {
		Equip* equip = new Equip;
		equip->id = res[i][0];
		equip->slots = (unsigned char) res[i][4];
		equip->scrolls = res[i][5];
		equip->type = (unsigned char) res[i][1];
		equip->pos = res[i][3];
		equip->istr = res[i][6];
		equip->idex = res[i][7];
		equip->iint = res[i][8];
		equip->iluk = res[i][9];
		equip->ihp = res[i][10];
		equip->imp = res[i][11];
		equip->iwatk = res[i][12];
		equip->imatk = res[i][13];
		equip->iwdef = res[i][14];
		equip->imdef = res[i][15];
		equip->iacc = res[i][16];
		equip->iavo = res[i][17];
		equip->ihand = res[i][18];
		equip->ijump = res[i][19];
		equip->ispeed = res[i][20];
		inv->addEquip(equip);
	}

	query << "SELECT itemid, inv, pos, amount FROM items WHERE charid = " << mysqlpp::quote << getPlayerid();
	res = query.store();
	for (size_t i = 0; i < res.num_rows(); ++i) {
		Item* item = new Item;
		item->id = res[i][0];
		item->inv = (unsigned char) res[i][1];
		item->pos = res[i][2];
		item->amount = res[i][3];
		inv->addItem(item);
	}

	query << "SELECT skillid, points, maxlevel FROM skills WHERE charid = " << mysqlpp::quote << getPlayerid();
	res = query.store();
	for (size_t i = 0; i < res.num_rows(); ++i) {
		skills->addSkillLevel(res[i][0], res[i][1], false);
		if(FORTHJOB_SKILL(res[i][0])){
			skills->setMaxSkillLevel(res[i][0], res[i][2]);
		}
	}
	
	query << "SELECT * FROM keymap WHERE charid = " << mysqlpp::quote << getPlayerid();
	res = query.store();
	for (size_t i=0; i<90; i++) {
		keys[i] = res[0][i+1];
	}

	query << "SELECT * FROM character_variables WHERE charid = " << mysqlpp::quote << getPlayerid();
	res = query.store();
	for (size_t i = 0; i < res.size(); i++) {
		variables[(string) res[i]["key"]] = res[i]["value"];
	}

	if (Maps::info[map].forcedReturn != 999999999) {
		map = Maps::info[map].forcedReturn;
	}

	PlayerPacket::connectData(this);
	
	if (strlen(ChannelServer::Instance()->getScrollingHeader()) > 0) {
		ServerPacket::changeScrollingHeader(ChannelServer::Instance()->getScrollingHeader());
	}

	pos.x = Maps::info[map].Portals[0].x;
	pos.y = Maps::info[map].Portals[0].y;

	type = 0;
	PlayerPacket::showKeys(this, keys);
	Maps::newMap(this, map);

	setOnline(true);
	isconnect = true;
	WorldServerConnectPlayerPacket::registerPlayer(ChannelServer::Instance()->getWorldPlayer(), id, name);
}

void Player::setHP(int hp, bool is){
	if(hp<0)
		this->hp=0;
	else if(hp>mhp)
		this->hp=mhp;
	else
		this->hp=hp;
	if(is)
		PlayerPacket::newHP(this, (short)this->hp);
}

void Player::setMP(int mp, bool is){
	if(mp<0)
		this->mp=0;
	else if(mp>mmp)
		this->mp=mmp;
	else
		this->mp=mp;
	PlayerPacket::newMP(this, (short)this->mp, is);
}

void Player::setSp(short sp){
	this->sp=sp;
	PlayerPacket::setSP(this);
}

void Player::setAp(short ap) {
	this->ap = ap;
	PlayerPacket::setAP(this);
}

void Player::setJob(short job){
	this->job=job;
	PlayerPacket::setJob(this);
}


void Player::setExp(int exp, bool is){
	this->exp=exp;
	if(is)
		PlayerPacket::newEXP(this, this->exp);
}

void Player::setLevel(int level){
	this->level=(unsigned char)level;

}

void Player::changeChannel(char channel) {
	ChannelServer::Instance()->getWorldPlayer()->playerChangeChannel(id, channel);
}

void Player::changeKey(unsigned char* packet){
	int howmany = BufferUtilities::getInt(packet+4);
	if (howmany == 0) return;
	for(int i=0; i<howmany; i++){
		int pos = BufferUtilities::getInt(packet+8+i*9);
		int key = BufferUtilities::getInt(packet+12+i*9);
		if(packet[12+i*9] == 0) // TODO 1st type byte, than key int
			key=0;
		if(pos>=0 && pos<90)
			keys[pos] = key;
	}

	// Update to mysql
	mysqlpp::Query query = db.query();
	query << "UPDATE keymap SET ";
	for(int i=0; i<90; i++){
		query << "pos" << mysqlpp::quote << i << "=" << mysqlpp::quote << keys[i];
		if(i!=89)
			query << ",";
		else
			query << " WHERE charid = " << mysqlpp::quote << getPlayerid();
	}
	query.exec();
}

void Player::setHair(int id){
	this->hair = id;
	PlayerPacket::newHair(this);
}

void Player::setEyes(int id){
	this->eyes = id;
	PlayerPacket::newEyes(this);
}

void Player::setSkin(char id){
	this->skin = id;
	PlayerPacket::newSkin(this);
}

int Player::addWarning(){
	int t = clock();
	// Deleting old warnings
	for(unsigned int i=0; i<warnings.size(); i++){
		if(warnings[i] + 300000 < t){
			warnings.erase(warnings.begin()+i);
			i--;
		}
	}
	warnings.push_back(t);
	if(warnings.size()>50){
		// Hacker - Temp DCing
		disconnect();
		return 1;
	}
	return 0;
}

void Player::saveSkills() {
	mysqlpp::Query query = db.query();
	query << "DELETE FROM skills WHERE charid = " << mysqlpp::quote << getPlayerid();
	query.exec();

	bool firstrun = true;
	for(int i=0; i<skills->getSkillsNum(); i++){
		if(firstrun == true){
			query << "INSERT INTO skills VALUES (" << mysqlpp::quote << getPlayerid() << "," << mysqlpp::quote << skills->getSkillID(i) << "," << mysqlpp::quote << skills->getSkillLevel(skills->getSkillID(i)) << "," << mysqlpp::quote << skills->getMaxSkillLevel(skills->getSkillID(i)) << ")";
			firstrun = false;
		}
		else{
			query << ",(" << mysqlpp::quote << getPlayerid() << "," << mysqlpp::quote << skills->getSkillID(i) << "," << mysqlpp::quote << skills->getSkillLevel(skills->getSkillID(i)) << "," << mysqlpp::quote << skills->getMaxSkillLevel(skills->getSkillID(i)) << ")";
		}
	}
	query.exec();
}

void Player::saveStats() {
	mysqlpp::Query query = db.query();
	query << "UPDATE characters SET "
			<< "level = " << mysqlpp::quote << getLevel() << ","
			<< "job = " << mysqlpp::quote << getJob() << ","
			<< "str = " << mysqlpp::quote << getStr() << ","
			<< "dex = " << mysqlpp::quote << getDex() << ","
			<< "intt = " << mysqlpp::quote << getInt() << ","
			<< "luk = " << mysqlpp::quote << getLuk() << ","
			<< "chp = " << mysqlpp::quote << getHP() << ","
			<< "mhp = " << mysqlpp::quote << getRMHP() << ","
			<< "cmp = " << mysqlpp::quote << getMP() << ","
			<< "mmp = " << mysqlpp::quote << getRMMP() << ","
			<< "hpmp_ap = " << mysqlpp::quote << getHPMPAp() << ","
			<< "ap = " << mysqlpp::quote << getAp() << ","
			<< "sp = " << mysqlpp::quote << getSp() << ","
			<< "exp = " << mysqlpp::quote << getExp() << ","
			<< "fame = " << mysqlpp::quote << getFame() << ","
			<< "map = " << mysqlpp::quote << getMap() << ","
			<< "gender = " << mysqlpp::quote << (short) getGender() << ","
			<< "skin = " << mysqlpp::quote << (short) getSkin() << ","
			<< "eyes = " << mysqlpp::quote << getEyes() << ","
			<< "hair = " << mysqlpp::quote << getHair() << ","
			<< "mesos = " << mysqlpp::quote << inv->getMesos()
			<< " WHERE id = " << getPlayerid();
	query.exec();
}

void Player::saveEquips() {
	mysqlpp::Query query = db.query();
	query << "DELETE FROM equip WHERE charid = " << getPlayerid();
	query.exec();

	bool firstrun = true;
	for(int i=0; i<inv->getEquipNum(); i++){
		if(firstrun == true){
			query << "INSERT INTO equip VALUES (";
			firstrun = false;
		}
		else{
			query << ",(";
		}
		query << mysqlpp::quote << inv->getEquip(i)->id << ","
				<< mysqlpp::quote << (short) Drops::equips[inv->getEquip(i)->id].type << ","
				<< mysqlpp::quote << getPlayerid() << ","
				<< mysqlpp::quote << (short) inv->getEquipPos(i) << ","
				<< mysqlpp::quote << (short) inv->getEquip(i)->slots << ","
				<< mysqlpp::quote << inv->getEquip(i)->scrolls << ","
				<< mysqlpp::quote << inv->getEquip(i)->istr << ","
				<< mysqlpp::quote << inv->getEquip(i)->idex << ","
				<< mysqlpp::quote << inv->getEquip(i)->iint << ","
				<< mysqlpp::quote << inv->getEquip(i)->iluk << ","
				<< mysqlpp::quote << inv->getEquip(i)->ihp << ","
				<< mysqlpp::quote << inv->getEquip(i)->imp << ","
				<< mysqlpp::quote << inv->getEquip(i)->iwatk << ","
				<< mysqlpp::quote << inv->getEquip(i)->imatk << ","
				<< mysqlpp::quote << inv->getEquip(i)->iwdef << ","
				<< mysqlpp::quote << inv->getEquip(i)->imdef << ","
				<< mysqlpp::quote << inv->getEquip(i)->iacc << ","
				<< mysqlpp::quote << inv->getEquip(i)->iavo << ","
				<< mysqlpp::quote << inv->getEquip(i)->ihand << ","
				<< mysqlpp::quote << inv->getEquip(i)->ijump << ","
				<< mysqlpp::quote << inv->getEquip(i)->ispeed << ")";
	}
	query.exec();
}

void Player::saveItems(){
	mysqlpp::Query query = db.query();
	query << "DELETE FROM items WHERE charid = " << getPlayerid();
	query.exec();

	bool firstrun = true;
	for(int i=0; i<inv->getItemNum(); i++){
		if(firstrun == true){
			query << "INSERT INTO items VALUES (";
			firstrun = false;
		}
		else{
			query << ",(";
		}
		query << mysqlpp::quote << inv->getItem(i)->id << ","
				<< mysqlpp::quote << getPlayerid() << ","
				<< mysqlpp::quote << (short) inv->getItem(i)->inv << ","
				<< mysqlpp::quote << inv->getItem(i)->pos << ","
				<< mysqlpp::quote << inv->getItem(i)->amount << ")";
	}
	query.exec();
}

void Player::saveVariables() {
	if (variables.size() > 0) {
		mysqlpp::Query query = db.query();
		query << "DELETE FROM character_variables WHERE charid = " << getPlayerid();
		query.exec();

		bool firstrun = true;
		for (hash_map <string, string>::iterator iter = variables.begin(); iter != variables.end(); iter++){
			if (firstrun == true) {
				query << "INSERT INTO character_variables VALUES (";
				firstrun = false;
			}
			else{
				query << ",(";
			}
			query << mysqlpp::quote << getPlayerid() << ","
					<< mysqlpp::quote << iter->first << ","
					<< mysqlpp::quote << iter->second << ")";
		}
		query.exec();
	}
}

void Player::save() {
	saveSkills();
	saveStats();
	saveEquips();
	saveItems();
	saveVariables();
}

void Player::setOnline(bool online) {
	int onlineid = online ? ChannelServer::Instance()->getOnlineId() : 0;
	mysqlpp::Query query = db.query();
	query << "UPDATE users INNER JOIN characters ON users.id = characters.userid SET users.online = " << mysqlpp::quote << onlineid << " WHERE characters.id = " << mysqlpp::quote << getPlayerid();
	query.exec();
}
