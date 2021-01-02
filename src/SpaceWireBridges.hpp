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
#include "SpaceWireBridge.hpp"
#include "config/Config.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <containers/algorithms.hpp>

using SpaceWireBridge_ctor
    = std::function<SpaceWireBridge*(const Config& cfg, packet_queue* publish_queue)>;

namespace details
{
struct SpaceWireBrigesSingleton
{
    static SpaceWireBrigesSingleton& instance()
    {
        static SpaceWireBrigesSingleton self;
        return self;
    }
    std::unordered_map<std::string, SpaceWireBridge_ctor> factory;
};
};

class SpaceWireBriges
{
public:
    static bool register_ctor(const std::string& name, SpaceWireBridge_ctor&& ctor)
    {
        using namespace details;
        SpaceWireBrigesSingleton::instance().factory[name] = std::move(ctor);
        return true;
    }

    static SpaceWireBridge* make_bridge(const std::string& name,const Config& cfg, packet_queue* publish_queue)
    {
        using namespace cpp_utils::containers;
        using  namespace details;
        if(contains(SpaceWireBrigesSingleton::instance().factory, name))
        {
            return SpaceWireBrigesSingleton::instance().factory[name](cfg,publish_queue);
        }
        return nullptr;
    }
};
