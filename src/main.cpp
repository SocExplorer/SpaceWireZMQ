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
#include "SpaceWireBridge.hpp"
#include "SpaceWireBridges.hpp"
#include "ZMQServer.hpp"
#include "config/Config.hpp"
#include "config/json_io.hpp"
#include "config/yaml_io.hpp"
#include "spdlog/spdlog.h"
#include <argparse/argparse.hpp>
#include <filesystem>


void load_config(const std::filesystem::path& path, Config& cfg)
{
    const auto ext = path.extension();
    if (ext == "yaml" or ext == "yml")
    {
        cfg = config_yaml::load_config<Config>(path);
        return;
    }
    if (ext == "json")
    {
        cfg = config_json::load_config<Config>(path);
        return;
    }
}

int main(int argc, char** argv)
{
    spdlog::info("Spacewire ZMQ server startup");
    argparse::ArgumentParser program("SpwaceWire ZMQ Server");
    program.add_argument("--config").help("Configuration file path");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        std::cout << err.what() << std::endl;
        std::cout << program;
        exit(0);
    }

    Config cfg;
    if (program.present("--config"))
    {
        const auto config_file = program.get<std::string>("--config");
        if (std::filesystem::exists(config_file))
        {
            spdlog::info("Loadding config file {}", config_file);
            load_config(config_file, cfg);
        }
        else
        {
            spdlog::error("Config file {} does not exist", config_file);
        }
    }

    ZMQServer server { cfg["server"] };
    {
        const auto _ = SpaceWireBridges::setup(cfg["bridges"], &server.received_packets);
        server.loop();
    }
    return 0;
}
