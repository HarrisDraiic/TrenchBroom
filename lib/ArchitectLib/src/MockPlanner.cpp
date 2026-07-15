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
#include <exception>
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

  try
  {
    const auto value = std::stod(match[1].str());
    const auto unit = match[2].str();
    const auto isFeet = !unit.empty() && (unit.front() == 'f' || unit.front() == 'F');
    return isFeet ? value * 0.3048 : value;
  }
  catch (const std::exception&)
  {
    return std::nullopt;
  }
}

Error invalidArgument(std::string message)
{
  return Error{ErrorCode::InvalidArgument, std::move(message)};
}

bool validProfileContext(const ProfilePlanningContext& profile)
{
  static const auto idExpression = std::regex{
    R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})", std::regex::icase};
  static const auto slugExpression = std::regex{R"([a-z0-9]+(?:-[a-z0-9]+)*)"};
  return std::regex_match(profile.id, idExpression)
         && std::regex_match(profile.slug, slugExpression) && profile.version >= 1
         && !profile.designLanguage.empty() && profile.designLanguage.size() <= 8 * 1024;
}

using ProfileWallThicknessResult = std::variant<std::optional<double>, Error>;

ProfileWallThicknessResult parseProfileWallThickness(const std::string& designLanguage)
{
  static const auto marker =
    std::regex{R"(\broom\s+wall\s+thickness\s*:)", std::regex::icase};
  if (!std::regex_search(designLanguage, marker))
  {
    return std::optional<double>{};
  }

  static const auto expression = std::regex{
    R"(\broom\s+wall\s+thickness\s*:\s*([0-9]+(?:\.[0-9]+)?)\s*(metres?|meters?|m|feet|foot|ft)\b)",
    std::regex::icase};
  auto match = std::smatch{};
  if (!std::regex_search(designLanguage, match, expression))
  {
    return invalidArgument(
      "The active profile has an invalid room wall thickness directive.");
  }

  try
  {
    const auto value = std::stod(match[1].str());
    const auto unit = match[2].str();
    const auto isFeet = !unit.empty() && (unit.front() == 'f' || unit.front() == 'F');
    const auto metres = isFeet ? value * 0.3048 : value;
    if (!std::isfinite(metres) || metres < 0.1 || metres > 2.0)
    {
      return invalidArgument(
        "Profile room wall thickness must be between 0.1 and 2 metres.");
    }
    return std::optional<double>{metres};
  }
  catch (const std::exception&)
  {
    return invalidArgument(
      "The active profile has an invalid room wall thickness directive.");
  }
}

} // namespace

RoomPlanResult MockPlanner::planRoom(
  const std::string_view promptView,
  const double unitsPerMetre,
  std::string material,
  std::optional<ProfilePlanningContext> profile)
{
  if (promptView.empty())
  {
    return invalidArgument("Enter a room request.");
  }
  if (!std::isfinite(unitsPerMetre) || unitsPerMetre < 1.0 || unitsPerMetre > 4096.0)
  {
    return invalidArgument("units_per_metre must be between 1 and 4096.");
  }

  auto wallThicknessMetres = 0.25;
  auto scaleAssumption =
    std::string{"Mock provider used the editor-supplied units-per-metre scale."};
  auto provenance = std::optional<ProfileProvenance>{};
  if (profile)
  {
    if (!validProfileContext(*profile))
    {
      return invalidArgument("The active profile planning context is invalid.");
    }

    const auto thicknessResult = parseProfileWallThickness(profile->designLanguage);
    if (const auto* error = std::get_if<Error>(&thicknessResult))
    {
      return *error;
    }
    const auto& profileThickness = std::get<std::optional<double>>(thicknessResult);
    if (profileThickness)
    {
      wallThicknessMetres = *profileThickness;
      scaleAssumption += " Active profile " + profile->slug + " version "
                         + std::to_string(profile->version)
                         + " applied its explicit room wall thickness rule.";
    }
    else
    {
      scaleAssumption += " Active profile " + profile->slug + " version "
                         + std::to_string(profile->version)
                         + " supplied provenance; no supported room rule was present.";
    }
    provenance = ProfileProvenance{
      .id = profile->id,
      .slug = profile->slug,
      .version = profile->version,
    };
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

  const auto wallThickness = snapToGrid(wallThicknessMetres * unitsPerMetre);
  const auto slabThickness = snapToGrid(0.25 * unitsPerMetre);
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
    .floorThickness = slabThickness,
    .ceilingThickness = slabThickness,
    .origin = {0.0, 0.0, 0.0},
    .material = std::move(material),
    .doorway =
      DoorwayBlueprint{
        .wall = WallSide::South,
        .width = doorwayWidth,
        .height = doorwayHeight,
        .centerOffset = 0.0,
      },
    .scaleAssumption = std::move(scaleAssumption),
    .profile = std::move(provenance),
  };
}

} // namespace tb::architect
