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
#include "Channels.h"
#include "Channel.h"
#include "LoginServerConnectPacket.h"
#include "PacketCreator.h"
#include "Session.h"
#include "WorldServer.h"
#include "WorldServerAcceptConnection.h"

Channels * Channels::singleton = nullptr;

Channels::Channels()
{
}

void Channels::registerChannel(WorldServerAcceptConnection *connection, uint16_t channel, const Ip &channelIp, const IpMatrix &extIp, port_t port) {
	shared_ptr<Channel> chan(new Channel());
	chan->setConnection(connection);
	chan->setId(channel);
	chan->setExternalIpInformation(channelIp, extIp);
	chan->setPort(port);
	m_channels[channel] = chan;
}

void Channels::removeChannel(uint16_t channel) {
	m_channels.erase(channel);
}

Channel * Channels::getChannel(uint16_t num) {
	return m_channels.find(num) != m_channels.end() ? m_channels[num].get() : nullptr;
}

void Channels::sendToAll(const PacketCreator &packet) {
	for (unordered_map<uint16_t, shared_ptr<Channel>>::iterator iter = m_channels.begin(); iter != m_channels.end(); ++iter) {
		sendToChannel(iter->first, packet);
	}
}

void Channels::sendToChannel(uint16_t channel, const PacketCreator &packet) {
	getChannel(channel)->send(packet);
}

void Channels::increasePopulation(uint16_t channel) {
	LoginServerConnectPacket::updateChannelPop(channel, getChannel(channel)->increasePlayers());
}

void Channels::decreasePopulation(uint16_t channel) {
	LoginServerConnectPacket::updateChannelPop(channel, getChannel(channel)->decreasePlayers());
}

uint16_t Channels::size() {
	return m_channels.size();
}

uint16_t Channels::getAvailableChannel() {
	uint16_t channel = -1;
	uint16_t max = static_cast<uint16_t>(WorldServer::Instance()->getMaxChannels());
	for (uint16_t i = 0; i < max; ++i) {
		if (m_channels.find(i) == m_channels.end()) {
			channel = i;
			break;
		}
	}
	return channel;
}