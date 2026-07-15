/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "architect/MockPlanner.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <regex>
#include <string>
#include <utility>

namespace tb::architect
{
namespace
{

constexpr double GridSize = 8.0;

double snapToGrid(const double value)
{
  return std::max(GridSize, std::round(value / GridSize) * GridSize);
}

std::optional<double> parseMeasurement(
  const std::string& prompt, const std::string_view suffix)
{
  const auto expression = std::regex{
    "([0-9]+(?:\\.[0-9]+)?)\\s*(metres?|meters?|m|feet|foot|ft)\\s*(?:"
      + std::string{suffix} + ")",
    std::regex::icase};
  auto match = std::smatch{};
  if (!std::regex_search(prompt, match, expression))
  {
    return std::nullopt;
  }

  const auto value = std::stod(match[1].str());
  const auto unit = match[2].str();
  const auto isFeet = !unit.empty() && (unit.front() == 'f' || unit.front() == 'F');
  return isFeet ? value * 0.3048 : value;
}

Error invalidArgument(std::string message)
{
  return Error{ErrorCode::InvalidArgument, std::move(message)};
}

} // namespace

RoomPlanResult MockPlanner::planRoom(
  const std::string_view promptView, const double unitsPerMetre, std::string material)
{
  if (promptView.empty())
  {
    return invalidArgument("Enter a room request.");
  }
  if (!std::isfinite(unitsPerMetre) || unitsPerMetre < 1.0 || unitsPerMetre > 4096.0)
  {
    return invalidArgument("units_per_metre must be between 1 and 4096.");
  }

  const auto prompt = std::string{promptView};
  const auto widthMetres = parseMeasurement(prompt, "wide|width");
  const auto depthMetres = parseMeasurement(prompt, "deep|depth|long");
  const auto heightMetres = parseMeasurement(prompt, "tall|high|height");
  if (!widthMetres || !depthMetres || !heightMetres)
  {
    return invalidArgument(
      "Specify room width, depth, and height with metric or imperial units.");
  }

  if (
    *widthMetres < 2.0 || *depthMetres < 2.0 || *heightMetres < 2.2
    || *widthMetres > 100.0 || *depthMetres > 100.0 || *heightMetres > 100.0)
  {
    return invalidArgument(
      "Room dimensions must be 2-100 metres, with a height of at least 2.2 metres.");
  }

  if (material.empty())
  {
    material = "__TB_empty";
  }

  const auto wallThickness = snapToGrid(0.25 * unitsPerMetre);
  const auto interiorWidth = snapToGrid(*widthMetres * unitsPerMetre);
  const auto interiorDepth = snapToGrid(*depthMetres * unitsPerMetre);
  const auto interiorHeight = snapToGrid(*heightMetres * unitsPerMetre);
  const auto doorwayWidth =
    std::min(snapToGrid(1.2 * unitsPerMetre), interiorWidth - 2.0 * wallThickness);
  const auto doorwayHeight =
    std::min(snapToGrid(2.2 * unitsPerMetre), interiorHeight - wallThickness);

  if (doorwayWidth < GridSize || doorwayHeight < GridSize)
  {
    return invalidArgument("The requested room is too small for a safe doorway.");
  }

  return RoomBlueprint{
    .id = "mock-room-v1",
    .unitsPerMetre = unitsPerMetre,
    .interiorWidth = interiorWidth,
    .interiorDepth = interiorDepth,
    .interiorHeight = interiorHeight,
    .wallThickness = wallThickness,
    .floorThickness = wallThickness,
    .ceilingThickness = wallThickness,
    .origin = {0.0, 0.0, 0.0},
    .material = std::move(material),
    .doorway =
      DoorwayBlueprint{
        .wall = WallSide::South,
        .width = doorwayWidth,
        .height = doorwayHeight,
        .centerOffset = 0.0,
      },
    .scaleAssumption = "Mock provider used the editor-supplied units-per-metre scale.",
  };
}

} // namespace tb::architect
