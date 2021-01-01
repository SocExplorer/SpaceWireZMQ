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
#include "config/json_io.hpp"
#include "config/yaml_io.hpp"

using Config = cppdict::Dict<bool, int, double, std::string>;


inline Config from_json(const std::string& json)
{
    return config_json::load_config<Config>(json);
}

inline Config from_yaml(const std::string& yaml)
{
    //return details::load_config(json::parse(json));
    return config_yaml::load_config<Config>(yaml);
}
