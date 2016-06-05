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
#include "InitializeCommon.hpp"
#include "AbstractServer.hpp"
#include "Database.hpp"
#include "DatabaseUpdater.hpp"
#include "ExitCodes.hpp"
#include "MapleVersion.hpp"
#include "McdbVersion.hpp"
#include "TimeUtilities.hpp"
#include <cstdio>
#include <iomanip>
#include <iostream>

namespace vana {
namespace initializing {

auto check_mcdb_version(abstract_server *server) -> result {
	soci::row row;

	try {
		auto &db = database::get_data_db();
		auto &sql = db.get_session();
		if (!db.table_exists("mcdb_info")) {
			server->log(log_type::critical_error, "mcdb_info does not exist.");
			exit(exit_code::mcdb_error);
			return result::failure;
		}

		sql.once
			<< "SELECT * "
			<< "FROM " << db.make_table("mcdb_info"),
			soci::into(row);

		if (!sql.got_data()) {
			server->log(log_type::critical_error, "mcdb_info is empty.");
			exit(exit_code::mcdb_error);
			return result::failure;
		}
	}
	catch (soci::soci_error &e) {
		server->log(log_type::critical_error, string{e.what()});
		exit(exit_code::mcdb_error);
		return result::failure;
	}

	int32_t version = row.get<int32_t>("version");
	int32_t subversion = row.get<int32_t>("subversion");
	game_version maple_version = row.get<game_version>("maple_version");
	bool test_server = row.get<bool>("test_server");
	string maple_locale = row.get<string>("maple_locale");

	if (version != mcdb::major_version || subversion != mcdb::sub_version) {
		server->log(log_type::critical_error, [&](out_stream &str) {
			str
				<< "MCDB version incompatible." << std::endl
				<< "Vana: " << mcdb::major_version << "." << mcdb::sub_version << std::endl
				<< "MCDB: " << version << "." << subversion;
		});
		exit(exit_code::mcdb_incompatible);
		return result::failure;
	}

	auto make_locale = [](const string &locale, bool test_server) {
		string loc = locale;
		if (test_server) {
			loc += " (test server)";
		}
		return loc;
	};

	if (maple_locale != mcdb::locale || test_server != mcdb::is_test_server) {
		server->log(log_type::critical_error, [&](out_stream &str) {
			str
			<< "Your MCDB is designed for different locale." << std::endl
			<< "Vana: " << make_locale(mcdb::locale, mcdb::is_test_server) << std::endl
			<< "MCDB: " << make_locale(maple_locale, test_server);
		});
		exit(exit_code::mcdb_locale_incompatible);
		return result::failure;
	}

	if (maple_version != maple_version::version) {
		server->log(log_type::warning, [&](out_stream &str) {
			str
				<< "WARNING: Your copy of MCDB is based on an incongruent version of the WZ files." << std::endl
				<< "Vana: " << maple_version::version << std::endl
				<< "MCDB: " << maple_version;
		});
	}

	return result::successful;
}

auto check_schema_version(abstract_server *server, bool update) -> result {
	database_updater db{server, update};

	version_check_result check = db.check_version();

	if (check == version_check_result::database_unavailable) {
		server->log(log_type::critical_error, "Vana database is currently inaccessible.");
		exit(exit_code::info_database_error);
		return result::failure;
	}

	if (check == version_check_result::needs_update) {
		if (!update) {
			// Wrong version and we're not allowed to update, so let's quit
			server->log(log_type::critical_error, "Wrong version of database, please run LoginServer to update.");
			exit(exit_code::info_database_error);
			return result::failure;
		}
		else {
			// Failed, but we can update it
			std::cout << std::setw(output_width) << std::left << "Updating database...";

			db.update();

			std::cout << "DONE" << std::endl;
		}
	}

	return result::successful;
}

auto set_users_offline(abstract_server *server, int32_t online_id) -> void {
	std::cout << "Resetting online status of players..." << std::endl;
	time_point start_time = utilities::time::get_now();

	auto &db = database::get_char_db();
	auto &sql = db.get_session();
	sql.once
		<< "UPDATE " << db.make_table("accounts") << " u "
		<< "INNER JOIN " << db.make_table("characters") << " c ON u.account_id = c.account_id "
		<< "SET "
		<< "	u.online = 0,"
		<< "	c.online = 0 "
		<< "WHERE u.online = :online",
		soci::use(online_id, "online");

	auto loading_time = utilities::time::get_distance<milliseconds>(utilities::time::get_now(), start_time);
	std::cout << "Reset all accounts and players in " << std::setprecision(3) << loading_time / 1000.f << " seconds!" << std::endl << std::endl;
}

}
}