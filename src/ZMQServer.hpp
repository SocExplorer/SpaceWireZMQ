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
#include "SpaceWireBridges.hpp"
#include "SpaceWireZMQ.hpp"
#include "config/Config.hpp"
#include "spdlog/spdlog.h"
#include <atomic>
#include <containers/algorithms.hpp>
#include <fmt/format.h>
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

    ZMQServer(const Config& cfg) : m_cfg { cfg }
    {
        m_ctx = zmq::context_t { 1 };
        m_publisher = zmq::socket_t { m_ctx, zmq::socket_type::pub };
        m_requests = zmq::socket_t { m_ctx, zmq::socket_type::rep };
        start();
    }

    inline bool start()
    {
        const auto address = m_cfg["address"].to<std::string>("127.0.0.1");
        const auto pub_port = m_cfg["pub_port"].to<int>(30000);
        const auto req_port = m_cfg["req_port"].to<int>(30001);

        m_publisher.bind(fmt::format("tcp://{}:{}", address, pub_port));
        m_requests.bind(fmt::format("tcp://{}:{}", address, req_port));

        m_req_thread = std::thread(&ZMQServer::handle_requests, this);
        m_publisher_thread = std::thread(&ZMQServer::publish_packets, this);
        return true;
    }

    inline void loop()
    {
        while (m_running)
        {
            std::this_thread::sleep_for(10ms);
        }
    }

    inline void close()
    {
        m_running = false;
        received_packets.close();
        if (m_req_thread.joinable())
            m_req_thread.join();
        if (m_publisher_thread.joinable())
            m_publisher_thread.join();
        m_publisher.close();
        m_requests.close();
    }

    inline Config configuration() { return m_cfg; }

    ~ZMQServer() { close(); }

private:
    void publish_packets()
    {
        while (m_running && !received_packets.closed())
        {
            auto packet = received_packets.take();
            if (packet)
            {
                m_publisher.send(to_message("Packets",*packet), zmq::send_flags::none);
            }
        }
    }

    void handle_requests()
    {
        using namespace cpp_utils::containers;
        zmq::message_t message;
        while (m_running)
        {
            int tries = 0;
            do
            {
                if (m_requests.recv(message, zmq::recv_flags::dontwait))
                {
                    tries = 0;
                    m_requests.send(zmq::message_t { std::string { "ok" } }, zmq::send_flags::none);
                    SpaceWireBriges::send(to_packet(message));
                }
                else
                {
                    tries++;
                }
            } while (tries < 100);
            std::this_thread::sleep_for(10us);
        }
    }
};
