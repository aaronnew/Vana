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
#pragma once

#include "common_temp/AbstractServer.hpp"
#include "common_temp/FinalizationPool.hpp"
#include "common_temp/Ip.hpp"
#include "common_temp/RatesConfig.hpp"
#include "common_temp/Types.hpp"
#include "common_temp/WorldConfig.hpp"
#include "world_server/Channels.hpp"
#include "world_server/LoginServerSession.hpp"
#include "world_server/PlayerDataProvider.hpp"
#include "world_server/WorldServerAcceptedSession.hpp"
#include <string>

namespace vana {
	class packet_builder;

	namespace world_server {
		class world_server final : public abstract_server {
			SINGLETON(world_server);
		public:
			auto shutdown() -> void override;
			auto established_login_connection(game_world_id world_id, connection_port port, const world_config &conf) -> void;
			auto rehash_config(const world_config &config) -> void;
			auto set_scrolling_header(const string &message) -> void;
			auto set_rates(const rates_config &rates) -> void;
			auto reset_rates(int32_t flags) -> void;
			auto get_player_data_provider() -> player_data_provider &;
			auto get_channels() -> channels &;
			auto is_connected() const -> bool;
			auto get_world_id() const -> game_world_id;
			auto make_channel_port(game_channel_id channel_id) const -> connection_port;
			auto get_config() -> const world_config &;
			auto send_login(const packet_builder &builder) -> void;
			auto on_connect_to_login(ref_ptr<login_server_session> connection) -> void;
			auto on_disconnect_from_login() -> void;
			auto finalize_server_session(ref_ptr<world_server_accepted_session> session) -> void;
		protected:
			auto listen() -> void;
			auto load_data() -> result override;
			auto make_log_identifier() const -> opt_string override;
			auto get_log_prefix() const -> string override;
		private:
			game_world_id m_world_id = -1;
			connection_port m_port = 0;
			world_config m_config;
			rates_config m_default_rates;
			ref_ptr<login_server_session> m_login_session;
			player_data_provider m_player_data_provider;
			channels m_channels;
			finalization_pool<world_server_accepted_session> m_session_pool;
		};
	}
}