/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "architect/Error.h"

#include <array>
#include <utility>

namespace tb::architect
{

std::string_view errorCodeName(const ErrorCode code)
{
  using Entry = std::pair<ErrorCode, std::string_view>;
  static constexpr auto entries = std::array{
    Entry{ErrorCode::InvalidRequest, "invalid_request"},
    Entry{ErrorCode::Unauthorized, "unauthorized"},
    Entry{ErrorCode::UnsupportedProtocol, "unsupported_protocol"},
    Entry{ErrorCode::UnknownMethod, "unknown_method"},
    Entry{ErrorCode::InvalidArgument, "invalid_argument"},
    Entry{ErrorCode::BridgeDisabled, "bridge_disabled"},
    Entry{ErrorCode::WriteDisabled, "write_disabled"},
    Entry{ErrorCode::DocumentNotOpen, "document_not_open"},
    Entry{ErrorCode::ObjectNotFound, "object_not_found"},
    Entry{ErrorCode::ProfileNotFound, "profile_not_found"},
    Entry{ErrorCode::AmbiguousProfileName, "ambiguous_profile_name"},
    Entry{ErrorCode::UnsupportedProfileSchema, "unsupported_profile_schema"},
    Entry{ErrorCode::AssetNotFound, "asset_not_found"},
    Entry{ErrorCode::InvalidAsset, "invalid_asset"},
    Entry{ErrorCode::MaterialNotFound, "material_not_found"},
    Entry{ErrorCode::EntityDefinitionNotFound, "entity_definition_not_found"},
    Entry{ErrorCode::WorldBoundsExceeded, "world_bounds_exceeded"},
    Entry{ErrorCode::BrushCreationFailed, "brush_creation_failed"},
    Entry{ErrorCode::InvalidBlueprint, "invalid_blueprint"},
    Entry{ErrorCode::GenerationFailed, "generation_failed"},
    Entry{ErrorCode::ValidationFailed, "validation_failed"},
    Entry{ErrorCode::OperationBusy, "operation_busy"},
    Entry{ErrorCode::OperationCancelled, "operation_cancelled"},
    Entry{ErrorCode::OperationTimeout, "operation_timeout"},
    Entry{ErrorCode::ProviderNotConfigured, "provider_not_configured"},
    Entry{ErrorCode::ProviderAuthenticationFailed, "provider_authentication_failed"},
    Entry{ErrorCode::InternalError, "internal_error"},
  };

  for (const auto& [entryCode, name] : entries)
  {
    if (entryCode == code)
    {
      return name;
    }
  }
  return "internal_error";
}

} // namespace tb::architect
