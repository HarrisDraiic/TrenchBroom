/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#pragma once

#include "Result.h"
#include "architect/Error.h"
#include "architect/RoomBlueprint.h"

#include <vector>

namespace tb::mdl
{
class BrushNode;
class Map;

Result<std::vector<BrushNode*>, architect::Error> createArchitectRoom(
  Map& map, const architect::RoomBlueprint& blueprint);

} // namespace tb::mdl
