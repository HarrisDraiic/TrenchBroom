/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#pragma once

#include "architect/Error.h"
#include "architect/RoomBlueprint.h"

#include <string>
#include <string_view>
#include <variant>

namespace tb::architect
{

using RoomPlanResult = std::variant<RoomBlueprint, Error>;

class MockPlanner
{
public:
  static RoomPlanResult planRoom(
    std::string_view prompt, double unitsPerMetre, std::string material);
};

} // namespace tb::architect
