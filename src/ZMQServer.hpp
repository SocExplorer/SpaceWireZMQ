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
#include "callable.hpp"
#include "config/Config.hpp"
#include <atomic>
#include <string_view>
#include <thread>
#include <zmq.hpp>

class ZMQServer
{
    zmq::context_t m_ctx;
    zmq::socket_t m_publisher;
    zmq::socket_t m_requests;
    std::thread m_publisher_thread;
    std::thread m_req_thread;
    std::atomic<bool> m_running { true };
    Config m_cfg;

public:
    packet_queue received_packets;

    bool start();
    void loop();
    void close();

    inline Config configuration() { return m_cfg; }

    ZMQServer(const Config& cfg) : m_cfg { cfg }
    {
        m_ctx = zmq::context_t { 1 };
        m_publisher = zmq::socket_t { m_ctx, zmq::socket_type::pub };
        m_requests = zmq::socket_t { m_ctx, zmq::socket_type::rep };
        start();
    }

    ~ZMQServer() { close(); }

private:
    void publish_packets();

    void handle_requests();
};
