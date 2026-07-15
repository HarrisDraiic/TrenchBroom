/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "mdl/ArchitectRoomBuilder.h"

#include "mdl/Brush.h"
#include "mdl/BrushBuilder.h"
#include "mdl/BrushNode.h"
#include "mdl/GameConfig.h"
#include "mdl/GameInfo.h"
#include "mdl/Map.h"
#include "mdl/Map_Nodes.h"
#include "mdl/WorldNode.h"

#include "vm/bbox.h"
#include "vm/vec.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace tb::mdl
{
namespace
{

architect::Error roomError(const architect::ErrorCode code, std::string message)
{
  return architect::Error{code, std::move(message)};
}

bool isPositiveFinite(const double value)
{
  return std::isfinite(value) && value > 0.0;
}

} // namespace

Result<std::vector<BrushNode*>, architect::Error> createArchitectRoom(
  Map& map, const architect::RoomBlueprint& blueprint)
{
  if (
    !isPositiveFinite(blueprint.interiorWidth)
    || !isPositiveFinite(blueprint.interiorDepth)
    || !isPositiveFinite(blueprint.interiorHeight)
    || !isPositiveFinite(blueprint.wallThickness)
    || !isPositiveFinite(blueprint.floorThickness)
    || !isPositiveFinite(blueprint.ceilingThickness)
    || !isPositiveFinite(blueprint.doorway.width)
    || !isPositiveFinite(blueprint.doorway.height)
    || !std::ranges::all_of(
      blueprint.origin, [](const auto value) { return std::isfinite(value); }))
  {
    return roomError(
      architect::ErrorCode::InvalidBlueprint,
      "Room dimensions must be positive finite numbers.");
  }

  if (blueprint.doorway.wall != architect::WallSide::South)
  {
    return roomError(
      architect::ErrorCode::InvalidBlueprint,
      "The first room generator supports a doorway on the south wall.");
  }

  const auto halfWidth = blueprint.interiorWidth / 2.0;
  const auto halfDepth = blueprint.interiorDepth / 2.0;
  const auto x0 = blueprint.origin[0];
  const auto y0 = blueprint.origin[1];
  const auto z0 = blueprint.origin[2];
  const auto doorMin =
    x0 + blueprint.doorway.centerOffset - blueprint.doorway.width / 2.0;
  const auto doorMax =
    x0 + blueprint.doorway.centerOffset + blueprint.doorway.width / 2.0;

  if (
    doorMin <= x0 - halfWidth || doorMax >= x0 + halfWidth
    || blueprint.doorway.height >= blueprint.interiorHeight)
  {
    return roomError(
      architect::ErrorCode::InvalidBlueprint,
      "The doorway must fit completely within the south wall.");
  }

  const auto outerBounds = vm::bbox3d{
    {x0 - halfWidth - blueprint.wallThickness,
     y0 - halfDepth - blueprint.wallThickness,
     z0 - blueprint.floorThickness},
    {x0 + halfWidth + blueprint.wallThickness,
     y0 + halfDepth + blueprint.wallThickness,
     z0 + blueprint.interiorHeight + blueprint.ceilingThickness}};
  if (!map.worldBounds().contains(outerBounds))
  {
    return roomError(
      architect::ErrorCode::WorldBoundsExceeded,
      "The room would exceed the active map world bounds.");
  }

  const auto zTop = z0 + blueprint.interiorHeight;
  const auto xMin = x0 - halfWidth;
  const auto xMax = x0 + halfWidth;
  const auto yMin = y0 - halfDepth;
  const auto yMax = y0 + halfDepth;
  const auto wall = blueprint.wallThickness;

  const auto brushBounds = std::vector<vm::bbox3d>{
    // Floor and ceiling.
    {{xMin - wall, yMin - wall, z0 - blueprint.floorThickness},
     {xMax + wall, yMax + wall, z0}},
    {{xMin - wall, yMin - wall, zTop},
     {xMax + wall, yMax + wall, zTop + blueprint.ceilingThickness}},
    // North, west, and east walls.
    {{xMin - wall, yMax, z0}, {xMax + wall, yMax + wall, zTop}},
    {{xMin - wall, yMin, z0}, {xMin, yMax, zTop}},
    {{xMax, yMin, z0}, {xMax + wall, yMax, zTop}},
    // South wall split around a centered doorway.
    {{xMin - wall, yMin - wall, z0}, {doorMin, yMin, zTop}},
    {{doorMax, yMin - wall, z0}, {xMax + wall, yMin, zTop}},
    {{doorMin, yMin - wall, z0 + blueprint.doorway.height}, {doorMax, yMin, zTop}},
  };

  const auto builder = BrushBuilder{
    map.worldNode().mapFormat(),
    map.worldBounds(),
    map.gameInfo().gameConfig.faceAttribsConfig.defaults};

  auto brushes = std::vector<Brush>{};
  brushes.reserve(brushBounds.size());
  for (const auto& bounds : brushBounds)
  {
    auto brush = builder.createCuboid(bounds, blueprint.material);
    if (brush.is_error())
    {
      return roomError(
        architect::ErrorCode::BrushCreationFailed,
        "A validated room brush could not be constructed.");
    }
    brushes.push_back(std::move(brush).value());
  }

  auto nodes = std::vector<BrushNode*>{};
  nodes.reserve(brushes.size());
  for (auto& brush : brushes)
  {
    nodes.push_back(new BrushNode{std::move(brush)});
  }

  const auto added = addNodes(
    map,
    {{parentForNodes(map), std::vector<Node*>{nodes.begin(), nodes.end()}}},
    "Architect: Create Room");
  if (added.size() != nodes.size())
  {
    return roomError(
      architect::ErrorCode::GenerationFailed,
      "The room transaction could not be committed.");
  }

  return nodes;
}

} // namespace tb::mdl
