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
#include "SpaceWireZMQ.hpp"
#include "config/Config.hpp"
#include "fmt/format.h"
#include <chrono>
#include <mutex>
#include <thread>
#include <zmq.hpp>
using namespace std::chrono_literals;

namespace topic_policy
{
struct per_topic_queue
{
};
struct merge_all_topics
{
};

template <typename _topic_policy>
static inline constexpr bool is_per_topic = std::is_same_v<_topic_policy, per_topic_queue>;
template <typename _topic_policy>
static inline constexpr bool is_all_topic_merged = std::is_same_v<_topic_policy, merge_all_topics>;

}

template <typename topic_policy_t>
class ZMQClient
{
    zmq::context_t m_ctx;
    zmq::socket_t m_subscription;
    zmq::socket_t m_requests;
    std::array<bool,
        topic_policy::is_all_topic_merged<topic_policy_t> ? 1 : std::size(topics::strings::table)>
        m_topic_enabled;
    std::array<packet_queue,
        topic_policy::is_all_topic_merged<topic_policy_t> ? 1 : std::size(topics::strings::table)>
        m_received_packets;
    bool m_running = false;
    std::thread m_sub_thread;

    std::size_t topic_index(const zmq::message_t& message)
    {
        using namespace topics;
        using namespace topics::strings;
        if constexpr (topic_policy::is_per_topic<topic_policy_t>)
        {
            return static_cast<std::size_t>(
                to_type(extract_topic(reinterpret_cast<const unsigned char*>(message.data()))));
        }
        return 0;
    }

    void store_packet(const zmq::message_t& message)
    {
        const std::size_t index = topic_index(message);
        assert(m_topic_enabled[index]);
        if ((index < std::size(m_received_packets)) && m_topic_enabled[index])
            m_received_packets[index] << to_packet(message, drop_topic_t::yes);
    }

    void subscription_thread()
    {
        zmq::message_t message;
        while (m_running)
        {
            int tries = 0;
            do
            {
                if (m_subscription.recv(message, zmq::recv_flags::dontwait))
                {
                    store_packet(message);
                    tries = 0;
                }
                else
                {
                    tries++;
                }
            } while (tries < 100);
            std::this_thread::sleep_for(10us);
        }
    }

    std::vector<spw_packet> get_packets(std::size_t index)
    {
        assert(index < std::size(m_received_packets));
        std::vector<spw_packet> packets;
        while (std::size(m_received_packets[index]))
        {
            packets.push_back(*m_received_packets[index].take());
        }
        return packets;
    }

public:
    ZMQClient(const std::initializer_list<topics::types>& subscribed_topics, Config cfg,
        topic_policy_t = topic_policy::per_topic_queue {})
    {
        const auto address = cfg["address"].to<std::string>("127.0.0.1");
        const auto pub_port = cfg["pub_port"].to<int>(30000);
        const auto req_port = cfg["req_port"].to<int>(30001);

        m_ctx = zmq::context_t { 1 };
        m_requests = zmq::socket_t { m_ctx, zmq::socket_type::req };

        m_requests.connect(fmt::format("tcp://{}:{}", address, req_port));

        m_subscription = zmq::socket_t { m_ctx, zmq::socket_type::sub };
        m_subscription.connect(fmt::format("tcp://{}:{}", address, pub_port));

        for (auto& enabled : m_topic_enabled)
        {
            enabled = false;
        }
        for (const auto& topic : subscribed_topics)
        {
            m_subscription.set(zmq::sockopt::subscribe, topics::to_string(topic));
            m_topic_enabled[static_cast<std::size_t>(topic)] = true;
        }
        m_running = true;
        m_sub_thread = std::thread(&ZMQClient::subscription_thread, this);
    }

    ~ZMQClient()
    {
        m_running = false;
        for (auto& queue : m_received_packets)
        {
            queue.close();
        }
        m_sub_thread.join();
        m_requests.close();
        m_subscription.close();
    }

    void send_packet(const spw_packet& packet)
    {
        if (m_requests.connected())
        {
            zmq::mutable_buffer resp;
            m_requests.send(to_message(packet), zmq::send_flags::none);
            m_requests.recv(resp);
        }
    }

    template <typename _topic_policy_t = topic_policy_t>
    std::enable_if_t<topic_policy::is_per_topic<_topic_policy_t>, std::vector<spw_packet>>
    get_packets(topics::types topic)
    {
        return get_packets(static_cast<std::size_t>(topic));
    }

    template <typename _topic_policy_t = topic_policy_t>
    std::enable_if_t<topic_policy::is_all_topic_merged<_topic_policy_t>, std::vector<spw_packet>>
    get_packets()
    {
        return get_packets(0);
    }
};
