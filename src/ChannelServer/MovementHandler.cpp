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
#include "MovementHandler.h"
#include "MovableLife.h"
#include "Pos.h"
#include "PacketReader.h"
#include <iomanip>
#include <iostream>

Pos MovementHandler::parseMovement(MovableLife *life, PacketReader &packet) {
	int16_t foothold = 0;
	int8_t stance = 0;
	int16_t x = 0;
	int16_t y = 0;
	uint8_t n = packet.get<uint8_t>();

	enum MovementTypes {
		NormalMovement = 0,
		Jump = 1,
		JumpKb = 2,
		Unk1 = 3,
		Teleport = 4,
		NormalMovement2 = 5,
		FlashJump = 6,
		Assaulter = 7,
		Assassinate = 8,
		Rush = 9,
		Falling = 10,
		Chair = 11,
		ExcessiveKb = 12,
		RecoilShot = 13,
		Unk2 = 14,
		JumpDown = 15,
		Wings = 16,
		WingsFalling = 17
	};

	for (uint8_t i = 0; i < n; ++i) {
		int8_t type = packet.get<int8_t>();
		switch (type) {
			case Falling:
				packet.skipBytes(1);
				break;
			case Wings:
				packet.skipBytes(7);
				break;
			case WingsFalling:
				x = packet.get<int16_t>();
				y = packet.get<int16_t>();
				foothold = packet.get<int16_t>();
				stance = packet.get<int8_t>();
				packet.skipBytes(6);
				break;
			case ExcessiveKb:
				packet.skipBytes(7);
				break;
			case Unk2:
				packet.skipBytes(9);
				break;
			case NormalMovement:
			case NormalMovement2:
				x = packet.get<int16_t>();
				y = packet.get<int16_t>();
				packet.skipBytes(4);
				foothold = packet.get<int16_t>();
				stance = packet.get<int8_t>();
				packet.skipBytes(2);
				break;
			case Jump:
			case JumpKb:
			case FlashJump:
			case RecoilShot:
				x = packet.get<int16_t>();
				y = packet.get<int16_t>();
				stance = packet.get<int8_t>();
				foothold = packet.get<int16_t>();
				break;
			case JumpDown:
				x = packet.get<int16_t>();
				y = packet.get<int16_t>();
				packet.skipBytes(6);
				foothold = packet.get<int16_t>();
				stance = packet.get<int8_t>();
				packet.skipBytes(2);
				break;
			case Chair:
				x = packet.get<int16_t>();
				y = packet.get<int16_t>();
				foothold = packet.get<int16_t>();
				stance = packet.get<int8_t>();
				packet.skipBytes(2);
				break;
			case Unk1:
			case Teleport:
			case Assaulter:
			case Assassinate:
			case Rush:
				x = packet.get<int16_t>();
				y = packet.get<int16_t>();
				packet.skipBytes(4);
				stance = packet.get<int8_t>();
				break;
			default:
				std::cerr << "New type of movement: 0x" << std::hex << (int16_t) type << std::endl;
				break;
		}
	}
	Pos pos(x, y);
	life->setPos(pos);
	life->setFh(foothold);
	life->setStance(stance);
	return pos;
}