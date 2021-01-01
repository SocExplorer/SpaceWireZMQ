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
#include <dict.hpp>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <yaml-cpp/yaml.h>
namespace config_yaml
{


enum class StringType
{
    String,
    Bool,
    Int,
    Float
};

StringType str_type(const std::string& data)
{
    const std::regex float_regex { R"(^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$)" };
    const std::regex bool_regex { R"(^(bool)?|(true)?$)" };
    const std::regex int_regex { R"(^[-+]?\d+$)" };
    if (std::regex_match(data, bool_regex))
        return StringType::Bool;
    if (std::regex_match(data, int_regex))
        return StringType::Int;
    if (std::regex_match(data, float_regex))
        return StringType::Float;
    return StringType::String;
}

template <typename config_t>
inline void _handle_scalar(const YAML::Node& node, config_t& cfg)
{
    const auto& data = node.as<std::string>();
    switch (str_type(data))
    {
        case StringType::String:
            cfg = data;
            break;
        case StringType::Float:
            cfg = node.as<double>();
            break;
        case StringType::Bool:
            cfg = node.as<bool>();
            break;
        case StringType::Int:
            cfg = node.as<int>();
            break;
        default:
            break;
    }
}

template <typename config_t>
inline void _load_config(const YAML::Node& node, config_t& cfg)
{
    switch (node.Type())
    {
        case YAML::NodeType::Scalar:
            _handle_scalar(node, cfg);
            break;
        case YAML::NodeType::Map:
            for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
            {
                _load_config(it->second, cfg[it->first.as<std::string>()]);
            }
            break;
        default:
            break;
    }
}

template <typename config_t>
inline config_t load_config(const YAML::Node& node)
{
    config_t cfg;
    _load_config(node, cfg);
    return cfg;
}

template <typename config_t>
inline config_t load_config(const std::string& yaml)
{
    return load_config<config_t>(YAML::Load(yaml));
}

}
