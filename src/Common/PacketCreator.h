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
#pragma once

#include "IPacket.h"
#include "shared_array.hpp"
#include "Types.h"
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;

class PacketReader;

class PacketCreator {
public:
	PacketCreator();

	template <typename T> void add(T value);
	template <> void add<bool>(bool value);
	template <typename T> void addVector(const vector<T> &vec);
	template <typename T> void addClass(const IPacketWritable &obj);
	template <typename T> void addClassVector(const vector<T> &vec);
	template <typename T> void set(T value, size_t pos);

	void addString(const string &str); // Dynamically-lengthed strings
	void addString(const string &str, size_t len); // Static-lengthed strings
	void addBytes(const char *hex);
	void addBuffer(const unsigned char *bytes, size_t len);
	void addBuffer(const PacketCreator &packet);
	void addBuffer(const PacketReader &packet);

	const unsigned char * getBuffer() const;
	size_t getSize() const;
	string toString() const;
private:
	static const size_t bufferLen = 1000; // Initial buffer length
	friend std::ostream & operator <<(std::ostream &out, const PacketCreator &packet);

	unsigned char * getBuffer(size_t pos, size_t len);
	unsigned char getHexByte(unsigned char input);

	size_t m_pos;
	MiscUtilities::shared_array<unsigned char> m_packet;
	size_t m_packetCapacity;
};

template <typename T>
void PacketCreator::add(T value) {
	(*(T *) getBuffer(m_pos, sizeof(T))) = value;
	m_pos += sizeof(T);
}

template <>
void PacketCreator::add<bool>(bool value) {
	add<int8_t>(value ? 1 : 0);
}

template <typename T>
void PacketCreator::set(T value, size_t pos) {
	(*(T *) getBuffer(pos, sizeof(T))) = value;
}

template <typename T>
void PacketCreator::addVector(const vector<T> &vec) {
	add<uint32_t>(vec.size());
	for (typename vector<T>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter) {
		add(*iter);
	}
}

template <typename T>
void PacketCreator::addClass(const IPacketWritable &obj) {
	obj.write(*this);
}

template <typename T>
void PacketCreator::addClassVector(const vector<T> &vec) {
	add<uint32_t>(vec.size());
	for (typename vector<T>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter) {
		addClass<T>(*iter);
	}
}

inline
const unsigned char * PacketCreator::getBuffer() const {
	return m_packet.get();
}

inline
void PacketCreator::addBuffer(const unsigned char *bytes, size_t len) {
	memcpy(getBuffer(m_pos, len), bytes, len);
	m_pos += len;
}

inline
size_t PacketCreator::getSize() const {
	return m_pos;
}

inline
std::ostream & operator <<(std::ostream &out, const PacketCreator &packet) {
	out << packet.toString();
	return out;
}