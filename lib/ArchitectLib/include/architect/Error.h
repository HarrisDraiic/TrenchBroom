/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#pragma once

#include <string>
#include <string_view>

namespace tb::architect
{

enum class ErrorCode
{
  InvalidRequest,
  Unauthorized,
  UnsupportedProtocol,
  UnknownMethod,
  InvalidArgument,
  BridgeDisabled,
  WriteDisabled,
  DocumentNotOpen,
  ObjectNotFound,
  ProfileNotFound,
  AmbiguousProfileName,
  UnsupportedProfileSchema,
  AssetNotFound,
  InvalidAsset,
  MaterialNotFound,
  EntityDefinitionNotFound,
  WorldBoundsExceeded,
  BrushCreationFailed,
  InvalidBlueprint,
  GenerationFailed,
  ValidationFailed,
  OperationBusy,
  OperationCancelled,
  OperationTimeout,
  ProviderNotConfigured,
  ProviderAuthenticationFailed,
  InternalError,
};

std::string_view errorCodeName(ErrorCode code);

struct Error
{
  ErrorCode code;
  std::string message;

  bool operator==(const Error&) const = default;
};

} // namespace tb::architect
