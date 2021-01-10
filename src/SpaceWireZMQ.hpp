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
enum class types
{
    RMAP = 0,
    CCSDS = 1,
    EXTEND = 2,
    GOES_R = 3,
    STUP = 4
};

namespace strings
{
    static constexpr char RMAP[] = "/RMAP/";
    static constexpr char CCSDS[] = "/CCSDS/";
    static constexpr char EXTEND[] = "/EXTEND/";
    static constexpr char GOES_R[] = "/GOES_R/";
    static constexpr char STUP[] = "/STUP/";
    static constexpr std::array table = { RMAP, CCSDS, EXTEND, GOES_R, STUP };
}

static constexpr std::size_t max_size = 16;

inline constexpr const char* to_string(types topic)
{
    auto topic_index = static_cast<std::size_t>(topic);
    if(topic_index>=std::size(strings::table))
    {
        return "";
    }
    return strings::table[topic_index];
}

inline std::size_t end_of_topic(const unsigned char* buffer)
{
    assert(buffer[0]=='/');
    auto pos=1UL;
    while((pos < topics::max_size) && (buffer[pos]!='/'))
        pos++;
    assert(buffer[pos]=='/');
    return pos+1;
}

inline std::string extract_topic(const unsigned char* buffer)
{
    std::size_t message_begin = topics::end_of_topic(buffer);
    return { buffer, buffer+ message_begin};
}
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

inline spw_packet to_packet(const void*buffer, std::size_t len)
{
    spw_packet p;
    yas::load<yas::mem | yas::binary>(
        yas::intrusive_buffer {reinterpret_cast<const char*>(buffer), len},
        YAS_OBJECT_STRUCT("spw_packet", p, data, port, bridge_id));
    return p;
}

inline spw_packet to_packet(const zmq::message_t& message)
{
    return to_packet(message.data(),message.size());
}

enum class drop_topic_t:bool
{
    no=false,
    yes=true
};

inline spw_packet to_packet(const zmq::message_t& message, drop_topic_t drop_topic)
{
    if(drop_topic==drop_topic_t::yes)
    {
        const auto buffer = reinterpret_cast<const unsigned char*>(message.data());
        const auto size = std::size(message);
        std::size_t message_begin = topics::end_of_topic(buffer);
        return to_packet(buffer+message_begin,size-message_begin);
    }
    else {
        return to_packet(message);
    }
}

