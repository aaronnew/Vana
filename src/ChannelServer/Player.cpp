/*
Copyright (C) 2008-2009 Vana Development Team

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
#include "BuddyListHandler.h"
#include "BuddyListPacket.h"
#include "ChannelServer.h"
#include "ChatHandler.h"
#include "CommandHandler.h"
#include "Connectable.h"
#include "Database.h"
#include "Drops.h"
#include "Fame.h"
#include "GameConstants.h"
#include "Instance.h"
#include "Inventory.h"
#include "KeyMaps.h"
#include "Levels.h"
#include "LevelsPacket.h"
#include "MapleSession.h"
#include "Maps.h"
#include "Mobs.h"
#include "NPCs.h"
#include "PacketReader.h"
#include "Party.h"
#include "Pets.h"
#include "PlayerHandler.h"
#include "PlayerPacket.h"
#include "Players.h"
#include "Quests.h"
#include "Reactors.h"
#include "RecvHeader.h"
#include "ServerPacket.h"
#include "SkillMacros.h"
#include "Skills.h"
#include "Summons.h"
#include "TimeUtilities.h"
#include "Trades.h"
#include "WorldServerConnectPlayer.h"
#include "WorldServerConnectPacket.h"

Player::~Player() {
	if (isconnect) {
		if (getInstance() != 0) {
			getInstance()->removePlayer(getId());
			getInstance()->sendMessage(Player_Disconnect, getId());
		}
		//if (this->getHP() == 0)
		//	this->acceptDeath();
		// "Bug" in global, would be fixed here:
		// When disconnecting and dead, you actually go back to forced return map before the death return map
		// (that means that it's parsed while logging in, not while logging out)
		if (save_on_dc) {
			saveAll();
			setOnline(false);
		}
		if (isTrading()) {
			Trades::cancelTrade(this);
		}
		WorldServerConnectPacket::removePlayer(ChannelServer::Instance()->getWorldPlayer(), id);
		Maps::getMap(map)->removePlayer(this);
		Players::Instance()->removePlayer(this);
	}
}

void Player::realHandleRequest(PacketReader &packet) {
	switch (packet.get<int16_t>()) {
		case RECV_ADD_SKILL: Skills::addSkill(this, packet); break;
		case RECV_ADD_STAT: Levels::addStat(this, packet); break;
		case RECV_ANIMATE_NPC: NPCs::handleNPCAnimation(this, packet); break;
		case RECV_BUDDYLIST: BuddyListHandler::handleBuddyList(this, packet); break;
		case RECV_CANCEL_ITEM: Inventory::cancelItem(this, packet); break;
		case RECV_CANCEL_SKILL: Skills::cancelSkill(this, packet); break;
		case RECV_CHANGE_CHANNEL: changeChannel(packet.get<int8_t>()); break;
		case RECV_CHANGE_MAP: Maps::usePortal(this, packet); break;
		case RECV_CHANGE_MAP_SPECIAL: Maps::useScriptedPortal(this, packet); break;
		case RECV_CHANNEL_LOGIN: playerConnect(packet); break;
		case RECV_CHAT: ChatHandler::handleChat(this, packet); break;
		case RECV_COMMAND: CommandHandler::handleCommand(this, packet); break;
		case RECV_CONTROL_MOB: Mobs::monsterControl(this, packet); break;
		case RECV_DAMAGE_MOB: Mobs::damageMob(this, packet); break;
		case RECV_DAMAGE_MOB_RANGED: Mobs::damageMobRanged(this, packet); break;
		case RECV_DAMAGE_MOB_SPELL: Mobs::damageMobSpell(this, packet); break;
		case RECV_DAMAGE_MOB_ENERGYCHARGE: Mobs::damageMobEnergyCharge(this, packet); break;
		case RECV_DAMAGE_MOB_SUMMON: Mobs::damageMobSummon(this, packet); break;
		case RECV_DAMAGE_PLAYER: PlayerHandler::handleDamage(this, packet); break;
		case RECV_DAMAGE_SUMMON: Summons::damageSummon(this, packet); break;
		case RECV_DROP_MESO: Drops::dropMesos(this, packet); break;
		case RECV_FACE_EXPRESSION: PlayerHandler::handleFacialExpression(this, packet); break;
		case RECV_FAME: Fame::handleFame(this, packet); break;
		case RECV_GET_PLAYER_INFO: PlayerHandler::handleGetInfo(this, packet); break;
		case RECV_GET_QUEST: Quests::getQuest(this, packet); break;
		case RECV_GROUP_CHAT: ChatHandler::handleGroupChat(this, packet); break;
		case RECV_HEAL_PLAYER: PlayerHandler::handleHeal(this, packet); break;
		case RECV_HIT_REACTOR: Reactors::hitReactor(this, packet); break;
		case RECV_KEYMAP: changeKey(packet); break;
		case RECV_LOOT_ITEM: Drops::playerLoot(this, packet); break;
		case RECV_MOVE_ITEM: Inventory::itemMove(this, packet); break;
		case RECV_MOVE_PLAYER: PlayerHandler::handleMoving(this, packet); break;
		case RECV_MOVE_SUMMON: Summons::moveSummon(this, packet); break;
		case RECV_NPC_TALK: NPCs::handleNPC(this, packet); break;
		case RECV_NPC_TALK_CONT: NPCs::handleNPCIn(this, packet); break;
		case RECV_PARTY_ACTION: Party::handleRequest(this, packet); break;
		case RECV_PET_CHAT: Pets::handleChat(this, packet); break;
		case RECV_PET_COMMAND: Pets::handleCommand(this, packet); break;
		case RECV_PET_FEED: Pets::handleFeed(this, packet); break;
		case RECV_PET_LOOT: Drops::petLoot(this, packet); break;
		case RECV_PET_MOVE: Pets::handleMovement(this, packet); break;
		case RECV_PET_SUMMON: Pets::handleSummon(this, packet); break;
		case RECV_PLAYER_ROOM_ACTION: Trades::tradeHandler(this, packet); break;
		case RECV_SHOP_ENTER: Inventory::useShop(this, packet); break;
		case RECV_SKILL_MACRO: changeSkillMacros(packet); break;
		case RECV_SPECIAL_SKILL: PlayerHandler::handleSpecialSkills(this, packet); break;
		case RECV_STOP_CHAIR: Inventory::stopChair(this, packet); break;
		case RECV_USE_CASH_ITEM: Inventory::useCashItem(this, packet); break;
		case RECV_USE_CHAIR: Inventory::useChair(this, packet); break;
		case RECV_USE_ITEM: Inventory::useItem(this, packet); break;
		case RECV_USE_ITEM_EFFECT: Inventory::useItemEffect(this, packet); break;
		case RECV_USE_RETURN_SCROLL: Inventory::useReturnScroll(this, packet); break;
		case RECV_USE_SCROLL: Inventory::useScroll(this, packet); break;
		case RECV_USE_SKILL: Skills::useSkill(this, packet); break;
		case RECV_USE_SKILLBOOK: Inventory::useSkillbook(this, packet); break;
		case RECV_USE_STORAGE: Inventory::useStorage(this, packet); break;
		case RECV_USE_SUMMON_BAG: Inventory::useSummonBag(this, packet); break;
	}
}
void Player::playerConnect(PacketReader &packet) {
	int32_t id = packet.get<int32_t>();
	if (!Connectable::Instance()->checkPlayer(id)) {
		// Hacking
		getSession()->disconnect();
		return;
	}
	this->id = id;
	activeBuffs.reset(new PlayerActiveBuffs(this));
	summons.reset(new PlayerSummons(this));
	buddyList.reset(new PlayerBuddyList(this));
	quests.reset(new PlayerQuests(this));
	pets.reset(new PlayerPets(this));
	// Character info
	mysqlpp::Query query = Database::getCharDB().query();
	query << "SELECT characters.*, users.gm FROM characters LEFT JOIN users on characters.userid = users.id WHERE characters.id = " << id;
	mysqlpp::StoreQueryResult res = query.store();

	if (res.empty()) {
		// Hacking
		getSession()->disconnect();
		return;
	}

	res[0]["name"].to_string(name);
	userid		= res[0]["userid"];
	exp			= res[0]["exp"];
	map			= res[0]["map"];
	gm			= res[0]["gm"];
	eyes		= res[0]["eyes"];
	hair		= res[0]["hair"];
	world_id	= static_cast<int8_t>(res[0]["world_id"]);
	gender		= static_cast<int8_t>(res[0]["gender"]);
	skin		= static_cast<int8_t>(res[0]["skin"]);
	mappos		= static_cast<int8_t>(res[0]["pos"]);
	level		= static_cast<uint8_t>(res[0]["level"]);
	job			= static_cast<int16_t>(res[0]["job"]);
	str			= static_cast<int16_t>(res[0]["str"]);
	dex			= static_cast<int16_t>(res[0]["dex"]);
	intt		= static_cast<int16_t>(res[0]["int"]);
	luk			= static_cast<int16_t>(res[0]["luk"]);
	hp			= static_cast<int16_t>(res[0]["chp"]);
	rmhp = mhp	= static_cast<int16_t>(res[0]["mhp"]);
	mp			= static_cast<int16_t>(res[0]["cmp"]);
	rmmp = mmp	= static_cast<int16_t>(res[0]["mmp"]);
	ap			= static_cast<int16_t>(res[0]["ap"]);
	sp			= static_cast<int16_t>(res[0]["sp"]);
	fame		= static_cast<int16_t>(res[0]["fame"]);
	hpmp_ap		= static_cast<uint16_t>(res[0]["hpmp_ap"]);
	buddylist_size = static_cast<uint8_t>(res[0]["buddylist_size"]);

	// Inventory
	uint8_t maxslots[5];
	maxslots[0] = static_cast<uint8_t>(res[0]["equip_slots"]);
	maxslots[1] = static_cast<uint8_t>(res[0]["use_slots"]);
	maxslots[2] = static_cast<uint8_t>(res[0]["setup_slots"]);
	maxslots[3] = static_cast<uint8_t>(res[0]["etc_slots"]);
	maxslots[4] = static_cast<uint8_t>(res[0]["cash_slots"]);
	inv.reset(new PlayerInventory(this, maxslots, res[0]["mesos"]));

	// Skills
	skills.reset(new PlayerSkills(this));

	// Storage
	storage.reset(new PlayerStorage(this));

	// Key Maps and Macros
	KeyMaps keyMaps;
	keyMaps.load(id);

	SkillMacros skillMacros;
	skillMacros.load(id);

	// Character variables
	query << "SELECT * FROM character_variables WHERE charid = " << id;
	res = query.store();
	for (size_t i = 0; i < res.size(); i++) {
		variables[(string) res[i]["key"]] = string(res[i]["value"]);
	}

	if (Maps::getMap(map)->getInfo()->forcedReturn != 999999999) {
		map = Maps::getMap(map)->getInfo()->forcedReturn;
		mappos = 0;
		if (hp == 0)
			hp = 50;
	}
	else {
		if (hp == 0) {
			hp = 50;
			map = Maps::getMap(map)->getInfo()->rm;
		}
	}

	m_pos = Maps::getMap(map)->getSpawnPoint(mappos)->pos;
	m_stance = 0;
	m_foothold = 0;

	PlayerPacket::connectData(this);

	if (ChannelServer::Instance()->getScrollingHeader().size() > 0)
		ServerPacket::showScrollingHeader(this, ChannelServer::Instance()->getScrollingHeader());

	for (int8_t i = 0; i < 3; i++) {
		if (Pet *pet = pets->getSummoned(i))
			pet->setPos(Maps::getMap(map)->getSpawnPoint(mappos)->pos);
	}

	PlayerPacket::showKeys(this, &keyMaps);

	BuddyListPacket::update(this, BuddyListPacket::add);

	if (skillMacros.getMax() > -1)
		PlayerPacket::showSkillMacros(this, &skillMacros);

	Maps::newMap(this, map);

	setOnline(true);
	isconnect = true;
	WorldServerConnectPacket::registerPlayer(ChannelServer::Instance()->getWorldPlayer(), ip, id, name, map, job, level);
}

void Player::setHP(int16_t shp, bool is) {
	if (shp < 0)
		hp = 0;
	else if (shp > mhp)
		hp = mhp;
	else
		hp = shp;
	if (is)
		PlayerPacket::updateStatShort(this, 0x400, hp);
	if (getPartyId())
		Party::showHPBar(this);
	getActiveBuffs()->checkBerserk();
	if (hp == 0 && getInstance() != 0) {
		getInstance()->sendMessage(Player_Death, getId());
	}
}

void Player::modifyHP(int16_t nhp, bool is) {
	if ((hp + nhp) < 0)
		hp = 0;
	else if ((hp + nhp) > mhp)
		hp = mhp;
	else
		hp = (hp + nhp);
	if (is)
		PlayerPacket::updateStatShort(this, 0x400, hp);
	if (getPartyId())
		Party::showHPBar(this);
	getActiveBuffs()->checkBerserk();
	if (hp == 0 && getInstance() != 0) {
		getInstance()->sendMessage(Player_Death, getId());
	}
}

void Player::damageHP(uint16_t dhp) {
	hp = (dhp > hp ? 0 : hp - dhp);
	PlayerPacket::updateStatShort(this, 0x400, hp);
	if (getPartyId())
		Party::showHPBar(this);
	getActiveBuffs()->checkBerserk();
	if (hp == 0 && getInstance() != 0) {
		getInstance()->sendMessage(Player_Death, getId());
	}
}

void Player::setMP(int16_t smp, bool is) {
	if (!(getActiveBuffs()->getActiveSkillLevel(Jobs::FPArchMage::Infinity) > 0 || getActiveBuffs()->getActiveSkillLevel(Jobs::ILArchMage::Infinity) > 0 || getActiveBuffs()->getActiveSkillLevel(Jobs::Bishop::Infinity) > 0)) {
		if (smp < 0)
			mp = 0;
		else if (smp > mmp)
			mp = mmp;
		else
			mp = smp;
	}
	PlayerPacket::updateStatShort(this, 0x1000, mp, is);
}

void Player::modifyMP(int16_t nmp, bool is) {
	if (!(getActiveBuffs()->getActiveSkillLevel(Jobs::FPArchMage::Infinity) > 0 || getActiveBuffs()->getActiveSkillLevel(Jobs::ILArchMage::Infinity) > 0 || getActiveBuffs()->getActiveSkillLevel(Jobs::Bishop::Infinity) > 0)) {
		if ((mp + nmp) < 0)
			mp = 0;
		else if ((mp + nmp) > mmp)
			mp = mmp;
		else
			mp = (mp + nmp);
	}
	PlayerPacket::updateStatShort(this, 0x1000, mp, is);
}

void Player::damageMP(uint16_t dmp) {
	if (!(getActiveBuffs()->getActiveSkillLevel(Jobs::FPArchMage::Infinity) > 0 || getActiveBuffs()->getActiveSkillLevel(Jobs::ILArchMage::Infinity) > 0 || getActiveBuffs()->getActiveSkillLevel(Jobs::Bishop::Infinity) > 0)) {
		mp = (dmp > mp ? 0 : mp - dmp);
	}
	PlayerPacket::updateStatShort(this, 0x1000, mp, false);
}

void Player::setSP(int16_t sp) {
	this->sp = sp;
	PlayerPacket::updateStatShort(this, 0x8000, sp);
}

void Player::setAP(int16_t ap) {
	this->ap = ap;
	PlayerPacket::updateStatShort(this, 0x4000, ap);
}

void Player::setJob(int16_t job) {
	this->job = job;
	PlayerPacket::updateStatShort(this, 0x20, job);
	LevelsPacket::jobChange(this);
	WorldServerConnectPacket::updateJob(ChannelServer::Instance()->getWorldPlayer(), id, job);
}

void Player::setStr(int16_t str) {
	this->str = str;
	PlayerPacket::updateStatShort(this, 0x40, str);
}

void Player::setDex(int16_t dex) {
	this->dex = dex;
	PlayerPacket::updateStatShort(this, 0x80, dex);
}

void Player::setInt(int16_t intt) {
	this->intt = intt;
	PlayerPacket::updateStatShort(this, 0x100, intt);
}

void Player::setLuk(int16_t luk) {
	this->luk = luk;
	PlayerPacket::updateStatShort(this, 0x200, luk);
}

void Player::setMHP(int16_t mhp) {
	if (mhp > 30000)
		mhp = 30000;
	else if (mhp < 1)
		mhp = 1;
	this->mhp = mhp;
	PlayerPacket::updateStatShort(this, 0x800, rmhp);
	if (getPartyId())
		Party::showHPBar(this);
	getActiveBuffs()->checkBerserk();
}

void Player::setMMP(int16_t mmp) {
	if (mmp > 30000)
		mmp = 30000;
	else if (mmp < 1)
		mmp = 1;
	this->mmp = mmp;
	PlayerPacket::updateStatShort(this, 0x2000, rmmp);
}

void Player::setHyperBody(int16_t modx, int16_t mody) {
	modx += 100;
	mody += 100;
	mhp = ((rmhp * modx / 100) > 30000 ? 30000 : rmhp * modx / 100);
	mmp = ((rmmp * mody / 100) > 30000 ? 30000 : rmmp * mody / 100);
	PlayerPacket::updateStatShort(this, 0x800, rmhp);
	PlayerPacket::updateStatShort(this, 0x2000, rmmp);
	if (getPartyId())
		Party::showHPBar(this);
	getActiveBuffs()->checkBerserk();
}

void Player::setRMHP(int16_t rmhp) {
	if (rmhp > 30000)
		rmhp = 30000;
	else if (rmhp < 1)
		rmhp = 1;
	this->rmhp = rmhp;
	PlayerPacket::updateStatShort(this, 0x800, rmhp);
}

void Player::setRMMP(int16_t rmmp) {
	if (rmmp > 30000)
		rmmp = 30000;
	else if (rmmp < 1)
		rmmp = 1;
	this->rmmp = rmmp;
	PlayerPacket::updateStatShort(this, 0x2000, rmmp);
}

void Player::modifyRMHP(int16_t mod) {
	rmhp = (((rmhp + mod) > 30000) ? 30000 : (rmhp + mod));
	PlayerPacket::updateStatShort(this, 0x800, rmhp);
}

void Player::modifyRMMP(int16_t mod) {
	rmmp = (((rmmp + mod) > 30000) ? 30000 : (rmmp + mod));
	PlayerPacket::updateStatShort(this, 0x2000, rmmp);
}

void Player::setExp(int32_t exp) {
	this->exp = exp;
	PlayerPacket::updateStatInt(this, 0x10000, exp);
}

void Player::setLevel(uint8_t level) {
	this->level = level;
	PlayerPacket::updateStatShort(this, 0x10, level);
	LevelsPacket::levelUp(this);
	WorldServerConnectPacket::updateLevel(ChannelServer::Instance()->getWorldPlayer(), id, level);
}

void Player::changeChannel(int8_t channel) {
	ChannelServer::Instance()->getWorldPlayer()->playerChangeChannel(id, channel);
}

void Player::changeKey(PacketReader &packet) {
	packet.skipBytes(4);
	int32_t howmany = packet.get<int32_t>();
	if (howmany == 0)
		return;

	KeyMaps keyMaps; // We don't need old values here because it is only used to save the new values
	for (int32_t i = 0; i < howmany; i++) {
		int32_t pos = packet.get<int32_t>();
		int8_t type = packet.get<int8_t>();
		int32_t action = packet.get<int32_t>();
		keyMaps.add(pos, new KeyMaps::KeyMap(type, action));
	}

	// Update to MySQL
	keyMaps.save(this->id);
}

void Player::changeSkillMacros(PacketReader &packet) {
	uint8_t num = packet.get<int8_t>();
	if (num == 0)
		return;

	SkillMacros skillMacros;
	for (uint8_t i = 0; i < num; i++) {
		string name = packet.getString();
		bool shout = packet.get<int8_t>() != 0;
		int32_t skill1 = packet.get<int32_t>();
		int32_t skill2 = packet.get<int32_t>();
		int32_t skill3 = packet.get<int32_t>();

		skillMacros.add(i, new SkillMacros::SkillMacro(name, shout, skill1, skill2, skill3));
	}
	skillMacros.save(id);
}

void Player::setHair(int32_t id) {
	this->hair = id;
	PlayerPacket::updateStatInt(this, 0x04, id);
}

void Player::setEyes(int32_t id) {
	this->eyes = id;
	PlayerPacket::updateStatInt(this, 0x02, id);
}

void Player::setSkin(int8_t id) {
	this->skin = id;
	PlayerPacket::updateStatInt(this, 0x01, id);
}

void Player::setFame(int16_t fame) {
	if (fame < -30000)
		fame = -30000;
	else if (fame > 30000)
		fame = 30000;
	this->fame = fame;
	PlayerPacket::updateStatInt(this, 0x20000, fame);
}

void Player::deleteVariable(const string &name) {
	if (variables.find(name) != variables.end())
		variables.erase(name);
}

void Player::setVariable(const string &name, const string &val) {
	variables[name] = val;
}

string Player::getVariable(const string &name) {
	return (variables.find(name) == variables.end()) ? "" : variables[name];
}

bool Player::addWarning() {
	int32_t t = TimeUtilities::clock_in_ms();
	// Deleting old warnings
	for (size_t i = 0; i < warnings.size(); i++) {
		if (warnings[i] + 300000 < t) {
			warnings.erase(warnings.begin() + i);
			i--;
		}
	}
	warnings.push_back(t);
	if (warnings.size() > 50) {
		// Hacker - Temp DCing
		getSession()->disconnect();
		return true;
	}
	return false;
}

void Player::saveStats() {
	mysqlpp::Query query = Database::getCharDB().query();
	query << "UPDATE characters SET "
		<< "level = " << static_cast<int16_t>(level) << "," // Queries have problems with int8_t due to being derived from ostream
		<< "job = " << job << ","
		<< "str = " << str << ","
		<< "dex = " << dex << ","
		<< "`int` = " << intt << ","
		<< "luk = " << luk << ","
		<< "chp = " << (hp > rmhp ? rmhp : hp) << "," // TODO: Change for HP gear calculation
		<< "mhp = " << rmhp << ","
		<< "cmp = " << (mp > rmmp ? rmmp : mp) << "," // TODO: Change for MP gear calculation
		<< "mmp = " << rmmp << ","
		<< "hpmp_ap = " << hpmp_ap << ","
		<< "ap = " << ap << ","
		<< "sp = " << sp << ","
		<< "exp = " << exp << ","
		<< "fame = " << fame << ","
		<< "map = " << map << ","
		<< "gender = " << static_cast<int16_t>(gender) << ","
		<< "skin = " << static_cast<int16_t>(skin) << ","
		<< "eyes = " << eyes << ","
		<< "hair = " << hair << ","
		<< "mesos = " << inv->getMesos() << ","
		<< "equip_slots = " << static_cast<int16_t>(inv->getMaxSlots(1)) << ","
		<< "use_slots = " << static_cast<int16_t>(inv->getMaxSlots(2)) << ","
		<< "setup_slots = " << static_cast<int16_t>(inv->getMaxSlots(3)) << ","
		<< "etc_slots = " << static_cast<int16_t>(inv->getMaxSlots(4)) << ","
		<< "cash_slots = " << static_cast<int16_t>(inv->getMaxSlots(5)) << ","
		<< "buddylist_size = " << static_cast<int16_t>(buddylist_size)
		<< " WHERE id = " << id;
	query.exec();
}

void Player::saveVariables() {
	mysqlpp::Query query = Database::getCharDB().query();
	query << "DELETE FROM character_variables WHERE charid = " << this->id;
	query.exec();

	if (variables.size() > 0) {
		bool firstrun = true;
		for (unordered_map<string, string>::iterator iter = variables.begin(); iter != variables.end(); iter++) {
			if (firstrun) {
				query << "INSERT INTO character_variables VALUES (";
				firstrun = false;
			}
			else {
				query << ",(";
			}
			query << id << ","
					<< mysqlpp::quote << iter->first << ","
					<< mysqlpp::quote << iter->second << ")";
		}
		query.exec();
	}
}

void Player::saveAll() {
	saveStats();
	saveVariables();
	getInventory()->save();
	getPets()->save();
	getSkills()->save();
	getStorage()->save();
}

void Player::setOnline(bool online) {
	int32_t onlineid = online ? ChannelServer::Instance()->getOnlineId() : 0;
	mysqlpp::Query query = Database::getCharDB().query();
	query << "UPDATE users INNER JOIN characters ON users.id = characters.userid SET users.online = " << onlineid <<
			", characters.online = " << online << " WHERE characters.id = " << id;
	query.exec();
}

void Player::setLevelDate() {
	mysqlpp::Query query = Database::getCharDB().query();
	query << "UPDATE characters SET time_level = NOW() WHERE characters.id = " << id;
	query.exec();
}

void Player::acceptDeath() {
	int32_t tomap = (Maps::getMap(map) ? Maps::getMap(map)->getInfo()->rm : map);
	setHP(50, false);
	getActiveBuffs()->removeBuff();
	Maps::changeMap(this, tomap, 0);
}

bool Player::hasGMEquip() {
	if (getInventory()->getEquippedID(1) == GMSuit::Hat)
		return true;
	if (getInventory()->getEquippedID(5) == GMSuit::Top)
		return true;
	if (getInventory()->getEquippedID(6) == GMSuit::Bottom)
		return true;
	if (getInventory()->getEquippedID(11) == GMSuit::Weapon)
		return true;
	return false;
}

void Player::setBuddyListSize(uint8_t size) {
	buddylist_size = size;
	BuddyListPacket::showSize(this);
}