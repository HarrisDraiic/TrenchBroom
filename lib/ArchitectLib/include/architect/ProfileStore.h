/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#pragma once

#include <QString>

#include "architect/Error.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace tb::architect
{

struct ArchitecturalProfile
{
  std::string id;
  std::string displayName;
  std::string slug;
  std::vector<std::string> aliases;
  int version = 1;
  std::string status = "draft";

  bool operator==(const ArchitecturalProfile&) const = default;
};

using ProfileResult = std::variant<ArchitecturalProfile, Error>;
using ProfileListResult = std::variant<std::vector<ArchitecturalProfile>, Error>;
using ActiveProfileResult = std::variant<std::optional<ArchitecturalProfile>, Error>;

class ProfileStore
{
private:
  QString m_rootPath;

public:
  explicit ProfileStore(QString rootPath);

  const QString& rootPath() const;

  ProfileListResult list() const;
  ProfileResult createDraft(
    std::string displayName,
    std::string designLanguage,
    std::vector<std::string> aliases = {});
  ProfileResult resolve(const std::string& reference) const;
  ActiveProfileResult active() const;
  ProfileResult setActive(const std::string& reference);
  std::optional<Error> clearActive();
};

} // namespace tb::architect
