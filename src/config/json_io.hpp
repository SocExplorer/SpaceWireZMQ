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
#include <dict.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace config_json
{
using json = nlohmann::json;
using json_t = decltype(json::parse(""));

template <typename config_t>
inline void _load_json_object(const json_t& obj, config_t& cfg);

template <typename config_t>
inline void _load_config(const json_t& node, config_t& cfg)
{
    switch (node.type())
    {
        case json::value_t::string:
            cfg = std::string { node };
            break;
        case json::value_t::boolean:
            cfg = bool { node };
            break;
        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
            cfg = int { node };
            break;
        case json::value_t::number_float:
            cfg = double { node };
            break;
        case json::value_t::object:
            _load_json_object(node, cfg);
            break;
        default:
            break;
    }
}

template <typename config_t>
inline void _load_json_object(const json_t& obj, config_t& cfg)
{
    for (auto& [key, value] : obj.items())
    {
        _load_config(value, cfg[key]);
    }
}

template <typename config_t>
inline config_t load_config(const json_t& json_doc)
{
    config_t cfg;
    _load_json_object(json_doc, cfg);
    return cfg;
}

template <typename config_t>
inline config_t load_config(const std::string& json)
{
    return load_config<config_t>(json::parse(json));
}

template <typename config_t>
inline config_t load_config(const std::filesystem::path& json_file)
{
    return load_config<config_t>(json::parse(std::ifstream { json_file.string() }));
}

}
