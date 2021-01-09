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
#include "SpaceWireBridge.hpp"
#include <star-api.h>
#include "StarAPI.hpp"
#include <queue>

class STARDundeeBridge: public ISpaceWireBridge
{
    StarAPI::Device m_device;
    std::vector<StarAPI::Channel<StarAPI::channel_direction_t::inout>> m_channels;
    bool m_setup{false};
    std::queue<spw_packet> m_rx_packets;
public:
    void packet_receiver_callback(STAR_TRANSFER_OPERATION *pOperation, STAR_TRANSFER_STATUS status);
    virtual bool send_packet(const spw_packet& packet)final;
    virtual spw_packet receive_packet()final;

    virtual bool packet_received()final;
    virtual bool set_configuration(const Config& cfg)final;
    virtual Config configuration()const final;
    STARDundeeBridge(const Config& cfg);
    virtual ~STARDundeeBridge();
};
