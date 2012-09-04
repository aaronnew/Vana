/*
Copyright (C) 2008-2012 Vana Development Team

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
#include "Session.h"
#include "AbstractConnection.h"
#include "Decoder.h"
#include "IpUtilities.h"
#include "PacketCreator.h"
#include "PacketReader.h"
#include "SessionManager.h"
#include <functional>
#include <iostream>

namespace asio = boost::asio;

Session::Session(asio::io_service &ioService, SessionManagerPtr sessionManager, AbstractConnection *connection, bool isForClient, bool isEncrypted, bool usePing, const string &patchLocation) :
	AbstractSession(sessionManager, (!isForClient || isEncrypted)),
	m_socket(ioService),
	m_connection(connection),
	m_isForClient(isForClient),
	m_patchLocation(patchLocation),
	m_decoder(!isForClient || isEncrypted),
	m_usePing(usePing)
{
}

void Session::start() {
	m_sessionManager->start(shared_from_this());
}

void Session::handleStart() {
	m_connection->setSession(this);
	m_connection->setPinging(m_usePing);
	m_connection->setIp(m_socket.remote_endpoint().address().to_v4().to_ulong());

	if (m_isForClient) {
		const PacketCreator &connectPacket = m_decoder.getConnectPacket(m_patchLocation);
		send(connectPacket, false);
	}

	startReadHeader();
}

void Session::stop() {
	m_sessionManager->stop(shared_from_this());
}

void Session::disconnect() {
	stop();
}

void Session::handleStop() {
	boost::system::error_code ec;
	m_socket.close(ec);
	if (ec) {
		std::cerr << "FAILURE TO CLOSE SESSION (" << ec.value() << "): " << ec.message() << std::endl;
	}
}

void Session::send(const PacketCreator &packet, bool encrypt) {
	send(packet.getBuffer(), packet.getSize(), encrypt);
}

void Session::send(const unsigned char *buf, int32_t len, bool encrypt) {
	boost::mutex::scoped_lock l(m_sendMutex);

	unsigned char *sendBuffer;
	size_t realLength = len;

	if (encrypt) {
		realLength += headerLen;
		sendBuffer = new unsigned char[realLength];
		m_sendPacket.reset(sendBuffer);

		memcpy(sendBuffer + headerLen, buf, len);
		m_decoder.createHeader(sendBuffer, static_cast<int16_t>(len));
		m_decoder.encrypt(sendBuffer + headerLen, len, headerLen);
	}
	else {
		sendBuffer = new unsigned char[realLength];
		m_sendPacket.reset(sendBuffer);

		memcpy(sendBuffer, buf, len);
	}

	asio::async_write(m_socket, asio::buffer(sendBuffer, realLength),
		std::bind(&Session::handleWrite, shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2));
}

void Session::startReadHeader() {
	m_buffer.reset(new unsigned char[headerLen]);

	asio::async_read(m_socket,
		asio::buffer(m_buffer.get(), headerLen),
		std::bind(&Session::handleReadHeader, shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2));
}

void Session::handleWrite(const boost::system::error_code &error, size_t bytesTransferred) {
	boost::mutex::scoped_lock l(m_sendMutex);
	if (error) {
		disconnect();
	}
}

void Session::handleReadHeader(const boost::system::error_code &error, size_t bytesTransferred) {
	if (!error) {
		// TODO: Figure out how to distinguish between client versions and server versions, can use this after
		//if (!m_decoder.validPacket(m_buffer.get())) {
		//	// Hacking or trying to crash server
		//	disconnect();
		//	return;
		//}

		size_t len = m_decoder.getLength(m_buffer.get());
		if (len < 2) {
			// Hacking or trying to crash server
			disconnect();
			return;
		}

		m_buffer.reset(new unsigned char[len]);

		asio::async_read(m_socket,
			asio::buffer(m_buffer.get(), len),
			std::bind(&Session::handleReadBody, shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}
	else {
		disconnect();
	}
}

void Session::handleReadBody(const boost::system::error_code &error, size_t bytesTransferred) {
	if (!error) {
		m_decoder.decrypt(m_buffer.get(), bytesTransferred, headerLen);

		PacketReader packet(m_buffer.get(), bytesTransferred);
		m_connection->baseHandleRequest(packet);

		startReadHeader();
	}
	else {
		disconnect();
	}
}

ip_t Session::getIp() const {
	return m_connection->getIp();
}