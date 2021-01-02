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
#include <yas/object.hpp>
#include <yas/serialize.hpp>
#include <yas/std_types.hpp>
#include <zmq.hpp>

inline zmq::const_buffer to_buffer(const spw_packet& packet)
{
    auto buf = yas::save<yas::mem | yas::binary>(
        YAS_OBJECT_STRUCT("spw_packet", packet, packet, port, bridge_id));

    return zmq::const_buffer{ buf.data.get(), buf.size };
}


inline spw_packet from_buffer(const zmq::mutable_buffer& buffer)
{
    spw_packet p;
    yas::load<yas::mem | yas::binary>(yas::shared_buffer { buffer.data(), buffer.size() },
        YAS_OBJECT_STRUCT("spw_packet", p, packet, port, bridge_id));
    return p;
}
