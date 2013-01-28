/*
Copyright (C) 2008-2013 Vana Development Team

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
#include "ConfigFile.h"
#include "Configuration.h"
#include "ExitCodes.h"
#include "FileUtilities.h"
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

ConfigFile::ConfigFile(const string &filename, bool executeFile)
{
	loadFile(filename);
	if (executeFile) {
		execute();
	}
}

ConfigFile::ConfigFile()
{
}

ConfigFile::~ConfigFile() {
	lua_close(getLuaState());
}

void ConfigFile::loadFile(const string &filename) {
	if (!FileUtilities::fileExists(filename)) {
		cerr << "ERROR: Configuration file " << filename << " does not exist!" << endl;
		ExitCodes::exit(ExitCodes::ConfigFileMissing);
	}

	m_file = filename;
	m_luaVm = luaL_newstate();
	luaopen_base(getLuaState());
}

bool ConfigFile::execute() {
	if (luaL_dofile(getLuaState(), m_file.c_str())) {
		handleError();
		return false;
	}
	return true;
}

void ConfigFile::handleError() {
	printError(lua_tostring(getLuaState(), -1));
}

void ConfigFile::printError(const string &error) {
	cout << error << endl;
}

bool ConfigFile::keyExists(const string &value) {
	lua_getglobal(getLuaState(), value.c_str());
	bool ret = !lua_isnil(getLuaState(), -1);
	lua_pop(getLuaState(), 1);
	return ret;
}

void ConfigFile::keyMustExist(const string &value) {
	if (!keyExists(value)) {
		cerr << "ERROR: Couldn't get a value from config file." << endl;
		cerr << "File: " << m_file << endl;
		cerr << "Value: " << value << endl;
		ExitCodes::exit(ExitCodes::ConfigError);
	}
}

void ConfigFile::setVariable(const string &name, const string &value) {
	lua_pushstring(getLuaState(), value.c_str());
	lua_setglobal(getLuaState(), name.c_str());
}

void ConfigFile::setVariable(const string &name, int32_t value) {
	lua_pushinteger(getLuaState(), value);
	lua_setglobal(getLuaState(), name.c_str());
}

string ConfigFile::getString(const string &value) {
	keyMustExist(value);
	lua_getglobal(getLuaState(), value.c_str());
	string x = lua_tostring(getLuaState(), -1);
	lua_pop(getLuaState(), 1);
	return x;
}

IpMatrix ConfigFile::getIpMatrix(const string &value) {
	keyMustExist(value);
	IpMatrix matrix;

	lua_getglobal(getLuaState(), value.c_str());
	lua_pushnil(getLuaState());
	while (lua_next(getLuaState(), -2)) {
		vector<uint32_t> arr;
		arr.reserve(2);

		lua_pushnil(getLuaState());
		while (lua_next(getLuaState(), -2)) {
			arr.push_back(Ip::stringToIpv4(lua_tostring(getLuaState(), -1)));
			lua_pop(getLuaState(), 1);
		}

		if (arr.size() != 2) {
			std::cerr << "ERROR: " << value << " configuration is malformed!" << endl;
			ExitCodes::exit(ExitCodes::ConfigError);
		}

		matrix.push_back(ExternalIp(arr[0], arr[1]));

		lua_pop(getLuaState(), 1);
	}
	lua_pop(getLuaState(), 1);

	return matrix;
}

vector<int8_t> ConfigFile::getBossChannels(const string &value, size_t maxChannels) {
	keyMustExist(value);
	vector<int8_t> channels;

	lua_getglobal(getLuaState(), value.c_str());
	lua_pushnil(getLuaState());
	while (lua_next(getLuaState(), -2)) {
		channels.push_back(lua_tointeger(getLuaState(), -1));
		lua_pop(getLuaState(), 1);
	}
	lua_pop(getLuaState(), 1);

	if (channels.size() == 1 && channels[0] == -1) {
		channels.clear();
		for (size_t i = 1; i <= maxChannels; i++) {
			channels.push_back(static_cast<int8_t>(i));
		}
	}
	return channels;
}