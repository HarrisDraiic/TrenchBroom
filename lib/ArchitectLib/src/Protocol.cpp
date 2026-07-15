/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "architect/Protocol.h"

#include <QJsonArray>
#include <QString>

#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace tb::architect
{
namespace
{

QString qString(const std::string_view value)
{
  return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

QString wallSideName(const WallSide side)
{
  switch (side)
  {
  case WallSide::North:
    return "north";
  case WallSide::South:
    return "south";
  case WallSide::East:
    return "east";
  case WallSide::West:
    return "west";
  }
  return "south";
}

std::optional<WallSide> parseWallSide(const QString& value)
{
  if (value == "north")
    return WallSide::North;
  if (value == "south")
    return WallSide::South;
  if (value == "east")
    return WallSide::East;
  if (value == "west")
    return WallSide::West;
  return std::nullopt;
}

Error invalidBlueprint(std::string message)
{
  return Error{ErrorCode::InvalidBlueprint, std::move(message)};
}

bool isPositiveNumber(const QJsonObject& object, const char* key)
{
  const auto value = object.value(key).toDouble(0.0);
  return std::isfinite(value) && value > 0.0;
}

QJsonObject makeSuccessResponse(QString id, QJsonObject result)
{
  return QJsonObject{
    {"protocol", qString(ProtocolVersion)},
    {"id", std::move(id)},
    {"result", std::move(result)},
  };
}

} // namespace

QJsonObject roomBlueprintToJson(const RoomBlueprint& blueprint)
{
  return QJsonObject{
    {"schema", "room/1"},
    {"id", QString::fromStdString(blueprint.id)},
    {"units_per_metre", blueprint.unitsPerMetre},
    {"interior_width", blueprint.interiorWidth},
    {"interior_depth", blueprint.interiorDepth},
    {"interior_height", blueprint.interiorHeight},
    {"wall_thickness", blueprint.wallThickness},
    {"floor_thickness", blueprint.floorThickness},
    {"ceiling_thickness", blueprint.ceilingThickness},
    {"origin", QJsonArray{blueprint.origin[0], blueprint.origin[1], blueprint.origin[2]}},
    {"material", QString::fromStdString(blueprint.material)},
    {"doorway",
     QJsonObject{
       {"wall", wallSideName(blueprint.doorway.wall)},
       {"width", blueprint.doorway.width},
       {"height", blueprint.doorway.height},
       {"center_offset", blueprint.doorway.centerOffset},
     }},
    {"scale_assumption", QString::fromStdString(blueprint.scaleAssumption)},
  };
}

RoomPlanResult roomBlueprintFromJson(const QJsonObject& json)
{
  if (json.value("schema").toString() != "room/1")
  {
    return invalidBlueprint("Unsupported room blueprint schema.");
  }

  for (const auto* key : {
         "units_per_metre",
         "interior_width",
         "interior_depth",
         "interior_height",
         "wall_thickness",
         "floor_thickness",
         "ceiling_thickness",
       })
  {
    if (!isPositiveNumber(json, key))
    {
      return invalidBlueprint(std::string{"Invalid or missing "} + key + ".");
    }
  }

  const auto origin = json.value("origin").toArray();
  if (origin.size() != 3)
  {
    return invalidBlueprint("origin must contain three coordinates.");
  }
  for (const auto coordinate : origin)
  {
    if (!coordinate.isDouble() || !std::isfinite(coordinate.toDouble()))
    {
      return invalidBlueprint("origin contains an invalid coordinate.");
    }
  }

  const auto doorwayJson = json.value("doorway").toObject();
  const auto wall = parseWallSide(doorwayJson.value("wall").toString());
  const auto centerOffset = doorwayJson.value("center_offset").toDouble();
  if (
    !wall || !isPositiveNumber(doorwayJson, "width")
    || !isPositiveNumber(doorwayJson, "height") || !std::isfinite(centerOffset))
  {
    return invalidBlueprint("doorway is invalid.");
  }

  const auto material = json.value("material").toString();
  if (material.isEmpty() || material.size() > 1024)
  {
    return invalidBlueprint("material is invalid.");
  }

  return RoomBlueprint{
    .id = json.value("id").toString().toStdString(),
    .unitsPerMetre = json.value("units_per_metre").toDouble(),
    .interiorWidth = json.value("interior_width").toDouble(),
    .interiorDepth = json.value("interior_depth").toDouble(),
    .interiorHeight = json.value("interior_height").toDouble(),
    .wallThickness = json.value("wall_thickness").toDouble(),
    .floorThickness = json.value("floor_thickness").toDouble(),
    .ceilingThickness = json.value("ceiling_thickness").toDouble(),
    .origin = {origin[0].toDouble(), origin[1].toDouble(), origin[2].toDouble()},
    .material = material.toStdString(),
    .doorway =
      DoorwayBlueprint{
        .wall = *wall,
        .width = doorwayJson.value("width").toDouble(),
        .height = doorwayJson.value("height").toDouble(),
        .centerOffset = centerOffset,
      },
    .scaleAssumption = json.value("scale_assumption").toString().toStdString(),
  };
}

QJsonObject makeErrorResponse(QString id, const Error& error)
{
  return QJsonObject{
    {"protocol", qString(ProtocolVersion)},
    {"id", std::move(id)},
    {"error",
     QJsonObject{
       {"code", qString(errorCodeName(error.code))},
       {"message", QString::fromStdString(error.message)},
     }},
  };
}

QJsonObject handleRequest(const QJsonObject& request)
{
  const auto id = request.value("id").toString();
  if (id.isEmpty() || id.size() > 128)
  {
    return makeErrorResponse(
      {}, Error{ErrorCode::InvalidRequest, "id must be a non-empty short string."});
  }
  if (request.value("protocol").toString() != qString(ProtocolVersion))
  {
    return makeErrorResponse(
      id, Error{ErrorCode::UnsupportedProtocol, "Use protocol architect/1."});
  }

  const auto method = request.value("method").toString();
  if (method == "runtime.status")
  {
    return makeSuccessResponse(
      id,
      QJsonObject{
        {"runtime", "TrenchBroom Architect Runtime"},
        {"provider", "mock"},
        {"external_access", false},
        {"transport", "child_process_stdio"},
      });
  }

  if (method == "plan.room")
  {
    const auto params = request.value("params").toObject();
    const auto prompt = params.value("prompt").toString();
    if (prompt.toUtf8().size() > static_cast<qsizetype>(MaximumPromptBytes))
    {
      return makeErrorResponse(
        id, Error{ErrorCode::InvalidArgument, "The prompt exceeds 8192 bytes."});
    }

    auto result = MockPlanner::planRoom(
      prompt.toStdString(),
      params.value("units_per_metre").toDouble(0.0),
      params.value("material").toString().toStdString());
    if (const auto* error = std::get_if<Error>(&result))
    {
      return makeErrorResponse(id, *error);
    }
    return makeSuccessResponse(
      id,
      QJsonObject{{"blueprint", roomBlueprintToJson(std::get<RoomBlueprint>(result))}});
  }

  return makeErrorResponse(
    id, Error{ErrorCode::UnknownMethod, "The requested method is not available."});
}

} // namespace tb::architect
