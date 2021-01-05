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
#include <channels/channels.hpp>
#include <zmq.hpp>

struct spw_packet
{
    std::vector<char> data;
    std::size_t port;
    std::string bridge_id;
    spw_packet() = default;
    spw_packet(const spw_packet&) = default;
    spw_packet(spw_packet&&) = default;
    ~spw_packet() = default;
    spw_packet& operator=(const spw_packet&) = default;
    spw_packet& operator=(spw_packet&&) = default;

    bool operator==(const spw_packet& other) const
    {
        return (port == other.port) && (bridge_id == other.bridge_id) && (data == other.data);
    }
    std::size_t size()const {return std::size(data);}
};

using packet_queue = channels::channel<spw_packet, 16, channels::full_policy::wait_for_space>;
