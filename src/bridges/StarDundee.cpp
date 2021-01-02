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
#include "StarDundee.hpp"


bool STARDundeeBridge::send_packet(const spw_packet &packet)
{

}

spw_packet STARDundeeBridge::receive_packet()
{

}

bool STARDundeeBridge::packet_received()
{

}

bool STARDundeeBridge::set_configuration(const Config &cfg)
{
    return true;
}

Config STARDundeeBridge::configuration() const
{
    return {};
}

STARDundeeBridge::STARDundeeBridge()
{

}

STARDundeeBridge::~STARDundeeBridge()
{

}
