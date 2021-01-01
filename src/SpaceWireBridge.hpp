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
#include <vector>
#include "config/Config.hpp"

class SpaceWireBridge
{
public:
    virtual bool send_packet(unsigned int port_number, const std::vector<char>& packet_data) = 0;
    virtual bool receive_packet(unsigned int port_number, std::vector<char>& packet_data) = 0;

    virtual bool packet_received()=0;
    virtual bool set_configuration(const Config& cfg)=0;
    virtual Config configuration()=0;
    SpaceWireBridge() = default;
    virtual ~SpaceWireBridge() = default;
};
