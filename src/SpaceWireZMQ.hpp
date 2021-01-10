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
#include <cstring>
#include <yas/object.hpp>
#include <yas/serialize.hpp>
#include <yas/std_types.hpp>
#include <zmq.hpp>

namespace topics
{
static constexpr char RMAP[] = "/RMAP/";
static constexpr char CCSDS[] = "/CCSDS/";
static constexpr std::size_t max_size = 16;
}


inline zmq::message_t to_message(const spw_packet& packet)
{
    auto buf = yas::save<yas::mem | yas::binary>(
        YAS_OBJECT_STRUCT("spw_packet", packet, data, port, bridge_id));
    return zmq::message_t { buf.data.get(), buf.size };
}

inline zmq::message_t to_message(const std::string& topic, const spw_packet& packet)
{
    const auto topic_len = std::size(topic);
    auto buf = yas::save<yas::mem | yas::binary>(
        YAS_OBJECT_STRUCT("spw_packet", packet, data, port, bridge_id));
    const auto buffer_len = topic_len + buf.size;
    char* buffer = new char[buffer_len]();
    std::memcpy(buffer, topic.data(), topic_len);
    std::memcpy(buffer + topic_len, buf.data.get(), buf.size);
    return zmq::message_t { buffer, buffer_len,
        [](void* data_, void* hint_) {
            (void)hint_;
            delete[] reinterpret_cast<char*>(data_);
        },
        nullptr };
}

inline spw_packet to_packet(const zmq::message_t& message)
{
    spw_packet p;
    yas::load<yas::mem | yas::binary>(
        yas::intrusive_buffer { reinterpret_cast<const char*>(message.data()), message.size() },
        YAS_OBJECT_STRUCT("spw_packet", p, data, port, bridge_id));
    return p;
}

inline spw_packet to_packet(const std::string& topic, const zmq::message_t& message)
{
    const auto topic_size = std::size(topic);
    spw_packet p;
    yas::load<yas::mem | yas::binary>(
        yas::intrusive_buffer { reinterpret_cast<const char*>(message.data()) + topic_size,
            message.size() - topic_size },
        YAS_OBJECT_STRUCT("spw_packet", p, data, port, bridge_id));
    return p;
}


inline std::string which_topic(const zmq::message_t& message)
{
    auto data = reinterpret_cast<const char*>(message.data());
    if (data[0] != '/')
        return {};
    auto pos = std::find(data + 1, data + topics::max_size + 1, '/');
    if (pos == data + topics::max_size + 1)
        return {};
    return { data, pos + 1 };
}

