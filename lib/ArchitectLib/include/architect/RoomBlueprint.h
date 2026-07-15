/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#pragma once

#include <array>
#include <optional>
#include <string>

namespace tb::architect
{

enum class WallSide
{
  North,
  South,
  East,
  West,
};

struct DoorwayBlueprint
{
  WallSide wall = WallSide::South;
  double width = 0.0;
  double height = 0.0;
  double centerOffset = 0.0;

  bool operator==(const DoorwayBlueprint&) const = default;
};

struct ProfileProvenance
{
  std::string id;
  std::string slug;
  int version = 0;

  bool operator==(const ProfileProvenance&) const = default;
};

struct RoomBlueprint
{
  std::string id;
  double unitsPerMetre = 0.0;
  double interiorWidth = 0.0;
  double interiorDepth = 0.0;
  double interiorHeight = 0.0;
  double wallThickness = 0.0;
  double floorThickness = 0.0;
  double ceilingThickness = 0.0;
  std::array<double, 3> origin = {0.0, 0.0, 0.0};
  std::string material;
  DoorwayBlueprint doorway;
  std::string scaleAssumption;
  std::optional<ProfileProvenance> profile;

  bool operator==(const RoomBlueprint&) const = default;
};

} // namespace tb::architect
