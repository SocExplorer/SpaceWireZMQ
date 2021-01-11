/*------------------------------------------------------------------------------
--  This file is a part of the SocExplorer Software
--  Copyright (C) 2021, Plasma Physics Laboratory - CNRS
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 2 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program; if not, write to the Free Software
--  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
-------------------------------------------------------------------------------*/
/*--                  Author : Alexis Jeandet
--                     Mail : alexis.jeandet@lpp.polytechnique.fr
----------------------------------------------------------------------------*/
#pragma once
#include "PacketQueue.hpp"
#include "SpaceWireBridge.hpp"
#include "SpaceWireBridges.hpp"
#include <vector>

static std::vector<spw_packet> sent_packets;
static std::queue<spw_packet> loopback_packets;

class MockBridge : public ISpaceWireBridge
{
    char redirect_value { 0 };

public:
    virtual bool send_packet(const spw_packet& packet) final
    {
        sent_packets.push_back(packet);
        if (packet.data[0] == redirect_value)
            loopback_packets.push(packet);
        return true;
    }
    virtual spw_packet receive_packet() final
    {
        auto packet = loopback_packets.front();
        loopback_packets.pop();
        return packet;
    }

    virtual bool packet_received() final { return std::size(loopback_packets); }
    virtual bool set_configuration(const Config& cfg) final
    {
        redirect_value = cfg["redirect_value"].to<int>(0);
        return true;
    }
    virtual Config configuration() const final { return {}; }
    MockBridge(const Config& cfg) { set_configuration(cfg); }
    virtual ~MockBridge() { }
};

static auto t = SpaceWireBridges::register_ctor(
    "Mock", [](const Config& cfg, packet_queue* publish_queue) {
        return std::make_unique<SpaceWireBridge>(std::make_unique<MockBridge>(cfg), publish_queue);
    });


const auto YML_Config = std::string(R"(
Mock:
  redirect_value: 1
)");
