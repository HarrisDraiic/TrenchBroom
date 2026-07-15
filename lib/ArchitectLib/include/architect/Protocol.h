/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#pragma once

#include <QJsonObject>

#include "architect/Error.h"
#include "architect/MockPlanner.h"
#include "architect/RoomBlueprint.h"

#include <cstddef>
#include <string_view>

namespace tb::architect
{

class ProfileStore;

inline constexpr std::string_view ProtocolVersion = "architect/1";
inline constexpr std::size_t MaximumRequestBytes = 64u * 1024u;
inline constexpr std::size_t MaximumPromptBytes = 8u * 1024u;

QJsonObject roomBlueprintToJson(const RoomBlueprint& blueprint);
RoomPlanResult roomBlueprintFromJson(const QJsonObject& json);

QJsonObject makeErrorResponse(QString id, const Error& error);
QJsonObject handleRequest(
  const QJsonObject& request, ProfileStore* profileStore = nullptr);

} // namespace tb::architect
