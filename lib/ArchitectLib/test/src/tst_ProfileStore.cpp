/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "architect/Error.h"
#include "architect/ProfileStore.h"

#include <optional>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>

namespace tb::architect
{

TEST_CASE("ProfileStore")
{
  auto temporaryDirectory = QTemporaryDir{};
  REQUIRE(temporaryDirectory.isValid());
  auto store = ProfileStore{QDir{temporaryDirectory.path()}.filePath("Profiles")};

  SECTION("fresh stores contain zero profiles")
  {
    const auto result = store.list();

    REQUIRE(std::holds_alternative<std::vector<ArchitecturalProfile>>(result));
    CHECK(std::get<std::vector<ArchitecturalProfile>>(result).empty());
    CHECK_FALSE(QFileInfo::exists(store.rootPath()));
  }

  SECTION("creates and resolves a human-inspectable draft")
  {
    const auto created = store.createDraft(
      "Greyhaven Monastery",
      "Sober coastal stone, compact cloisters, and protected archival spaces.",
      {" Greyhaven ", "Coastal Monastery"});

    REQUIRE(std::holds_alternative<ArchitecturalProfile>(created));
    const auto& profile = std::get<ArchitecturalProfile>(created);
    CHECK(profile.slug == "greyhaven-monastery");
    CHECK(profile.status == "draft");
    CHECK(profile.aliases.front() == "Greyhaven");
    const auto profileDirectory = QDir{store.rootPath()}.filePath("greyhaven-monastery");
    const auto metadataPath = QDir{profileDirectory}.filePath("profile.json");
    const auto designLanguagePath = QDir{profileDirectory}.filePath("design-language.md");
    CHECK(QFileInfo::exists(metadataPath));
    CHECK(QFileInfo::exists(designLanguagePath));

    auto metadataFile = QFile{metadataPath};
    REQUIRE(metadataFile.open(QIODevice::ReadOnly));
    const auto metadata = QJsonDocument::fromJson(metadataFile.readAll()).object();
    CHECK(metadata.value("schema").toString() == "architect-profile/1");
    CHECK(metadata.value("version").toInt() == 1);

    auto designLanguageFile = QFile{designLanguagePath};
    REQUIRE(designLanguageFile.open(QIODevice::ReadOnly));
    CHECK(designLanguageFile.readAll().contains("Sober coastal stone"));

    for (const auto& reference : {
           "Greyhaven Monastery",
           "Greyhaven",
           "greyhaven-monastery",
         })
    {
      const auto resolved = store.resolve(reference);
      REQUIRE(std::holds_alternative<ArchitecturalProfile>(resolved));
      CHECK(std::get<ArchitecturalProfile>(resolved).id == profile.id);
    }
  }

  SECTION("persists and clears the active profile")
  {
    const auto created = store.createDraft("Greyhaven", "Sober coastal stone.");
    REQUIRE(std::holds_alternative<ArchitecturalProfile>(created));

    const auto selected = store.setActive("greyhaven");
    REQUIRE(std::holds_alternative<ArchitecturalProfile>(selected));
    const auto active = store.active();
    REQUIRE(std::holds_alternative<std::optional<ArchitecturalProfile>>(active));
    REQUIRE(std::get<std::optional<ArchitecturalProfile>>(active).has_value());
    CHECK(
      std::get<std::optional<ArchitecturalProfile>>(active)->id
      == std::get<ArchitecturalProfile>(created).id);

    CHECK_FALSE(store.clearActive().has_value());
    const auto cleared = store.active();
    REQUIRE(std::holds_alternative<std::optional<ArchitecturalProfile>>(cleared));
    CHECK_FALSE(std::get<std::optional<ArchitecturalProfile>>(cleared).has_value());
  }

  SECTION("rejects traversal-like names and normalized collisions")
  {
    const auto traversal = store.createDraft("../..", "Unsafe name.");
    REQUIRE(std::holds_alternative<Error>(traversal));
    CHECK(std::get<Error>(traversal).code == ErrorCode::InvalidArgument);

    REQUIRE(std::holds_alternative<ArchitecturalProfile>(
      store.createDraft("Greyhaven Monastery", "First draft.")));
    const auto collision = store.createDraft("Greyhaven---Monastery", "Second draft.");
    REQUIRE(std::holds_alternative<Error>(collision));
    CHECK(std::get<Error>(collision).code == ErrorCode::InvalidArgument);
  }

  SECTION("rejects ambiguous aliases")
  {
    REQUIRE(std::holds_alternative<ArchitecturalProfile>(
      store.createDraft("Greyhaven", "First draft.", {"Monastery"})));
    REQUIRE(std::holds_alternative<ArchitecturalProfile>(
      store.createDraft("Redhaven", "Second draft.", {"Monastery"})));

    const auto result = store.resolve("Monastery");
    REQUIRE(std::holds_alternative<Error>(result));
    CHECK(std::get<Error>(result).code == ErrorCode::AmbiguousProfileName);
  }

  SECTION("rejects malformed metadata")
  {
    REQUIRE(std::holds_alternative<ArchitecturalProfile>(
      store.createDraft("Greyhaven", "Sober coastal stone.")));
    const auto metadataPath = QDir{store.rootPath()}.filePath("greyhaven/profile.json");
    auto file = QFile{metadataPath};
    REQUIRE(file.open(QIODevice::ReadOnly));
    auto metadata = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
    metadata.insert("version", 1.5);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    REQUIRE(file.write(QJsonDocument{metadata}.toJson()) > 0);
    file.close();

    const auto result = store.list();
    REQUIRE(std::holds_alternative<Error>(result));
    CHECK(std::get<Error>(result).code == ErrorCode::UnsupportedProfileSchema);
  }
}

} // namespace tb::architect
