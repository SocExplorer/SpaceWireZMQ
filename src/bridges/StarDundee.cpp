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
#include "StarDundee.hpp"
#include "PacketQueue.hpp"
#include "SpaceWireBridges.hpp"
#include "StarAPI.hpp"
#include "config/Config.hpp"
#include <star-api.h>


static auto t = SpaceWireBridges::register_ctor(
    "STAR-Dundee", [](const Config& cfg, packet_queue* publish_queue) {
        return std::make_unique<SpaceWireBridge>(
            std::make_unique<STARDundeeBridge>(cfg), publish_queue);
    });


bool STARDundeeBridge::send_packet(const spw_packet& packet)
{
    if (m_setup && packet.port < std::size(m_channels))
    {
        m_channels[packet.port].send_packet(
            { (unsigned char*)packet.data.data(), std::size(packet.data) });
        return true;
    }
    return false;
}

spw_packet STARDundeeBridge::receive_packet()
{
    spw_packet packet;
    if (m_setup && packet.port < std::size(m_channels))
    {
        auto buffer = m_channels[packet.port].receive_packet();
        packet.data.resize(buffer.size);
        std::memcpy(packet.data.data(),buffer.data, buffer.size);
    }
    return packet;
}

bool STARDundeeBridge::packet_received()
{
    return std::accumulate(std::begin(m_channels), std::end(m_channels),0,[](int value,auto & chan)
    {
        return value+chan.available_packets_count();
    });
}

bool STARDundeeBridge::set_configuration(const Config& cfg)
{
    return true;
}

Config STARDundeeBridge::configuration() const
{
    return {};
}

STARDundeeBridge::STARDundeeBridge(const Config& cfg)
{
    using namespace StarAPI;
    m_device = Device { "30170316" };
    m_device.set_speed(10000000,1);
    m_device.set_speed(10000000,2);
    m_channels.resize(2);
    m_channels[0].open(m_device, 1 );
    m_channels[1].open(m_device, 2 );
    m_setup = true;
}

STARDundeeBridge::~STARDundeeBridge() { }
