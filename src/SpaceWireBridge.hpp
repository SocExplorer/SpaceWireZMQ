/*------------------------------------------------------------------------------
--  This file is a part of the SocExplorer Software
--  Copyright (C) 2020, Plasma Physics Laboratory - CNRS
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
#include "config/Config.hpp"
#include <memory>
#include <thread>
#include <vector>
#include <chrono>
using namespace std::chrono_literals;

class ISpaceWireBridge
{
public:
    virtual bool send_packet(const spw_packet& packet) = 0;
    virtual spw_packet receive_packet() = 0;

    virtual bool packet_received() = 0;
    virtual bool set_configuration(const Config& cfg) = 0;
    virtual Config configuration() const = 0;
    ISpaceWireBridge() = default;
    virtual ~ISpaceWireBridge() = default;
};


class SpaceWireBridge
{
    std::unique_ptr<ISpaceWireBridge> m_bridge;
    packet_queue m_sending_queue;
    packet_queue* m_publish_queue = nullptr;
    std::thread m_rec_thread;
    std::thread m_send_thread;
public:
    SpaceWireBridge(std::unique_ptr<ISpaceWireBridge>&& bridge, packet_queue* publish_queue)
            : m_bridge { std::move(bridge) }, m_publish_queue { publish_queue }
    {
        m_rec_thread = std::thread([this]()
        {
            while (!m_publish_queue->closed()) {
                if(m_bridge->packet_received())
                    *m_publish_queue << m_bridge->receive_packet();
                std::this_thread::sleep_for(100us);
            }
        });

        m_send_thread = std::thread([this]()
        {
            while (!m_sending_queue.closed()) {
                m_bridge->send_packet(*m_sending_queue.take());
            }
        });
    }
    ~SpaceWireBridge()
    {
        m_sending_queue.close();
        m_rec_thread.join();
        m_send_thread.join();
    }

    bool set_configuration(const Config& cfg) { return m_bridge->set_configuration(cfg); }
    Config configuration() const { return m_bridge->configuration(); }
};
