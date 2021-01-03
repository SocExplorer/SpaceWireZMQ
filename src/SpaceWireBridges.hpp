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
#include <containers/algorithms.hpp>
#include <functional>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

using SpaceWireBridge_ctor = std::function<std::unique_ptr<SpaceWireBridge>(
    const Config& cfg, packet_queue* publish_queue)>;

namespace details
{
struct SpaceWireBrigesSingleton
{
    static SpaceWireBrigesSingleton& instance()
    {
        static SpaceWireBrigesSingleton self;
        return self;
    }
    static void load_bridge(
        const std::string& bridge_id, const Config& config, packet_queue* publish_queue)
    {
        using namespace cpp_utils::containers;
        auto& self = instance();
        if (contains(self.factory, bridge_id) && !contains(self.loaded_bridges, bridge_id))
        {
            auto bridge = self.factory[bridge_id](config, publish_queue);
            self.loaded_bridges[bridge_id] = std::move(bridge);
        }
        else
        {
            spdlog::error("Unknown bridge ID {}, not loading it.", bridge_id);
        }
    }

    inline void send(spw_packet&& packet)
    {
        using namespace cpp_utils::containers;
        auto& self = instance();
        if (contains(self.loaded_bridges, packet.bridge_id))
        {
            self.loaded_bridges[packet.bridge_id]->send(std::move(packet));
        }
        else
        {
            spdlog::error("Unknown bridge ID {}, dropping packet.", packet.bridge_id);
        }
    }

    inline void teardown() { loaded_bridges.clear(); }

    std::unordered_map<std::string, SpaceWireBridge_ctor> factory;
    std::unordered_map<std::string, std::unique_ptr<SpaceWireBridge>> loaded_bridges;
};
};

class SpaceWireBridges
{
public:
    static void setup(Config config, packet_queue* publish_queue)
    {
        using namespace cpp_utils::containers;
        if (!config.isEmpty())
        {
            for (const auto& [name, node] : config)
            {
                details::SpaceWireBrigesSingleton::instance().load_bridge(
                    name, *node.get(), publish_queue);
            }
        }
    }

    static void teardown() { details::SpaceWireBrigesSingleton::instance().teardown(); }

    static bool register_ctor(const std::string& name, SpaceWireBridge_ctor&& ctor)
    {
        using namespace details;
        SpaceWireBrigesSingleton::instance().factory[name] = std::move(ctor);
        return true;
    }

    static inline void send(spw_packet&& packet)
    {
        using namespace details;
        SpaceWireBrigesSingleton::instance().send(std::move(packet));
    }
};
