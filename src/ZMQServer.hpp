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
#include "SpaceWireZMQ.hpp"
#include "SpaceWireBridges.hpp"
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

public:
    packet_queue received_packets;
    std::unordered_map<std::string, packet_queue> packets_to_send;

    ZMQServer(const Config& cfg)
            : m_ctx(0)
            , m_publisher(m_ctx, zmq::socket_type::pub)
            , m_requests(m_ctx, zmq::socket_type::rep)
    {
        const auto address = cfg["server"]["address"].to<std::string>("localhost");
        const auto pub_port = cfg["server"]["pub_port"].to<int>(30000);
        const auto req_port = cfg["server"]["req_port"].to<int>(30001);

        m_publisher.bind(fmt::format("tcp://{}:{}", address, pub_port));
        m_requests.bind(fmt::format("tcp://{}:{}", address, req_port));

        m_req_thread = std::thread([this]() {
            zmq::mutable_buffer buffer;
            while (m_running)
            {
                if (m_requests.recv(buffer, zmq::recv_flags::none))
                {
                    m_requests.send(zmq::message_t { std::string { "ok" } }, zmq::send_flags::none);
                    auto packet = from_buffer(buffer);
                    if (cpp_utils::containers::contains(packets_to_send, packet.bridge_id))
                        packets_to_send[packet.bridge_id] << packet;
                    else
                        spdlog::error("Unknown bridge ID {}, dropping packet.", packet.bridge_id);
                }
            }
        });

        m_publisher_thread = std::thread([this]() {
            while (m_running && !received_packets.closed())
            {
                auto packet = received_packets.take();
                if (packet)
                {
                    m_publisher.send(to_buffer(*packet), zmq::send_flags::dontwait);
                }
            }
        });
    }
    ~ZMQServer()
    {
        m_running = false;
        received_packets.close();
        m_req_thread.join();
        m_publisher_thread.join();
        m_publisher.close();
        m_requests.close();
    }
};
