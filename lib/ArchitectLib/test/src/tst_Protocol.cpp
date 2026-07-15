/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include <QJsonObject>
#include <QString>

#include "architect/Error.h"
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

  SECTION("rejects invalid blueprint dimensions")
  {
    const auto parsed = roomBlueprintFromJson(QJsonObject{{"schema", "room/1"}});
    REQUIRE(std::holds_alternative<Error>(parsed));
    CHECK(std::get<Error>(parsed).code == ErrorCode::InvalidBlueprint);
  }
}

} // namespace tb::architect
