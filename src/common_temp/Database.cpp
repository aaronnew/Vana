/*
Copyright (C) 2008-2016 Vana Development Team

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
#include "Database.hpp"
#include "common_temp/ConfigFile.hpp"
#include "common_temp/DbConfig.hpp"
#include <soci-mysql.h>

namespace vana {

thread_local owned_ptr<database> database::m_chardb{nullptr};
thread_local owned_ptr<database> database::m_datadb{nullptr};

auto database::init_char_db() -> database & {
	if (m_chardb != nullptr) throw std::logic_error{"Must not call init_char_db after the database is already initialized"};

	auto config = config_file::get_database_config();
	config->run();
	db_config conf = config->get<db_config>("chardb");
	m_chardb = make_owned_ptr<database>(conf, false);
	auto &sql = m_chardb->get_session();

	if (!schema_exists(sql, conf.database)) {
		sql.once << "CREATE DATABASE " << conf.database;
	}

	m_chardb.release();
	return get_char_db();
}

database::database(const db_config &conf, bool include_database) {
	m_session = make_owned_ptr<soci::session>(soci::mysql, build_connection_string(conf, include_database));
	m_session->reconnect();
	m_schema = conf.database;
	m_table_prefix = conf.table_prefix;
}

auto database::get_session() -> soci::session & {
	return *m_session;
}

auto database::get_schema() const -> string {
	return m_schema;
}

auto database::get_table_prefix() const -> string {
	return m_table_prefix;
}

auto database::make_table(const string &table) const -> string {
	return m_table_prefix + table;
}

auto database::table_exists(const string &table) -> bool {
	return table_exists(get_session(), m_schema, make_table(table));
}

auto database::schema_exists(soci::session &sql, const string &schema) -> bool {
	opt_string database;
	sql.once
		<< "SELECT SCHEMA_NAME "
		<< "FROM INFORMATION_SCHEMA.SCHEMATA "
		<< "WHERE SCHEMA_NAME = :schema "
		<< "LIMIT 1",
		soci::use(schema, "schema"),
		soci::into(database);
	return database.is_initialized();
}

auto database::table_exists(soci::session &sql, const string &schema, const string &table) -> bool {
	opt_string database_table;
	sql.once
		<< "SELECT TABLE_NAME "
		<< "FROM INFORMATION_SCHEMA.TABLES "
		<< "WHERE TABLE_SCHEMA = :schema AND TABLE_NAME = :table "
		<< "LIMIT 1",
		soci::use(schema, "schema"),
		soci::use(table, "table"),
		soci::into(database_table);
	return database_table.is_initialized();
}

auto database::connect_char_db() -> void {
	auto config = config_file::get_database_config();
	config->run();
	db_config conf = config->get<db_config>("chardb");
	m_chardb = make_owned_ptr<database>(conf, true);
}

auto database::connect_data_db() -> void {
	auto config = config_file::get_database_config();
	config->run();
	db_config conf = config->get<db_config>("datadb");
	m_datadb = make_owned_ptr<database>(conf, true);
}

auto database::build_connection_string(const db_config &conf, bool include_database) -> string {
	out_stream str;

	if (include_database) {
		str << "dbname=" << conf.database << " ";
	}

	str
		<< "user=" << conf.username
		<< " password='" << conf.password << "'"
		<< " host=" << conf.host
		<< " port=" << conf.port;

	return str.str();
}

}