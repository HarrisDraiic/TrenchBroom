/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QTemporaryDir>

#include "architect/Error.h"
#include "architect/ProfileStore.h"
#include "architect/Protocol.h"
#include "architect/RoomBlueprint.h"

#include <variant>

#include <catch2/catch_test_macros.hpp>

namespace tb::architect
{

TEST_CASE("Protocol")
{
  SECTION("reports mock runtime status without external access")
  {
    const auto response = handleRequest(QJsonObject{
      {"protocol", "architect/1"},
      {"id", "status-1"},
      {"method", "runtime.status"},
    });

    const auto result = response.value("result").toObject();
    CHECK(result.value("provider").toString() == "mock");
    CHECK_FALSE(result.value("external_access").toBool(true));
  }

  SECTION("rejects unsupported protocols with a stable code")
  {
    const auto response = handleRequest(QJsonObject{
      {"protocol", "architect/999"},
      {"id", "bad-version"},
      {"method", "runtime.status"},
    });

    CHECK(
      response.value("error").toObject().value("code").toString()
      == "unsupported_protocol");
  }

  SECTION("rejects invalid request envelopes with a stable code")
  {
    const auto response = handleRequest(QJsonObject{
      {"protocol", "architect/1"},
      {"method", "runtime.status"},
    });

    CHECK(
      response.value("error").toObject().value("code").toString() == "invalid_request");
  }

  SECTION("rejects unknown methods with a stable code")
  {
    const auto response = handleRequest(QJsonObject{
      {"protocol", "architect/1"},
      {"id", "unknown-method"},
      {"method", "map.execute-arbitrary-code"},
    });

    CHECK(
      response.value("error").toObject().value("code").toString() == "unknown_method");
  }

  SECTION("rejects prompts over the protocol limit")
  {
    const auto prompt =
      QString(static_cast<qsizetype>(MaximumPromptBytes + 1u), QChar{'x'});
    const auto response = handleRequest(QJsonObject{
      {"protocol", "architect/1"},
      {"id", "large-prompt"},
      {"method", "plan.room"},
      {"params",
       QJsonObject{
         {"prompt", prompt},
         {"units_per_metre", 32.0},
         {"material", "stone"},
       }},
    });

    CHECK(
      response.value("error").toObject().value("code").toString() == "invalid_argument");
  }

  SECTION("round trips a planned room blueprint")
  {
    const auto response = handleRequest(QJsonObject{
      {"protocol", "architect/1"},
      {"id", "room-1"},
      {"method", "plan.room"},
      {"params",
       QJsonObject{
         {"prompt", "Create a room 12 metres wide, 10 metres deep, 5 metres tall"},
         {"units_per_metre", 32.0},
         {"material", "stone"},
       }},
    });

    const auto json = response.value("result").toObject().value("blueprint").toObject();
    const auto parsed = roomBlueprintFromJson(json);
    REQUIRE(std::holds_alternative<RoomBlueprint>(parsed));
    CHECK(std::get<RoomBlueprint>(parsed).material == "stone");
  }

  SECTION("manages draft profiles through the bounded runtime protocol")
  {
    auto temporaryDirectory = QTemporaryDir{};
    REQUIRE(temporaryDirectory.isValid());
    auto store = ProfileStore{QDir{temporaryDirectory.path()}.filePath("Profiles")};

    const auto created = handleRequest(
      QJsonObject{
        {"protocol", "architect/1"},
        {"id", "create-profile"},
        {"method", "profiles.create_draft"},
        {"params",
         QJsonObject{
           {"display_name", "Greyhaven Monastery"},
           {"design_language", "Sober coastal stone and compact cloisters."},
           {"aliases", QJsonArray{"Greyhaven"}},
         }},
      },
      &store);
    const auto profile = created.value("result").toObject().value("profile").toObject();
    REQUIRE(profile.value("status").toString() == "draft");

    const auto selected = handleRequest(
      QJsonObject{
        {"protocol", "architect/1"},
        {"id", "select-profile"},
        {"method", "profiles.set_active"},
        {"params", QJsonObject{{"reference", "Greyhaven"}}},
      },
      &store);
    CHECK(
      selected.value("result").toObject().value("profile").toObject().value("id")
      == profile.value("id"));

    const auto listed = handleRequest(
      QJsonObject{
        {"protocol", "architect/1"},
        {"id", "list-profiles"},
        {"method", "profiles.list"},
      },
      &store);
    const auto listResult = listed.value("result").toObject();
    CHECK(listResult.value("profiles").toArray().size() == 1);
    CHECK(listResult.value("active_profile_id") == profile.value("id"));

    const auto cleared = handleRequest(
      QJsonObject{
        {"protocol", "architect/1"},
        {"id", "clear-profile"},
        {"method", "profiles.clear_active"},
      },
      &store);
    CHECK(cleared.value("result").toObject().value("cleared").toBool());
  }

  SECTION("keeps profile methods disabled without a configured root")
  {
    const auto response = handleRequest(QJsonObject{
      {"protocol", "architect/1"},
      {"id", "profiles-disabled"},
      {"method", "profiles.list"},
    });

    CHECK(
      response.value("error").toObject().value("code").toString() == "bridge_disabled");
  }

  SECTION("rejects invalid blueprint dimensions")
  {
    const auto parsed = roomBlueprintFromJson(QJsonObject{{"schema", "room/1"}});
    REQUIRE(std::holds_alternative<Error>(parsed));
    CHECK(std::get<Error>(parsed).code == ErrorCode::InvalidBlueprint);
  }
}

} // namespace tb::architect
