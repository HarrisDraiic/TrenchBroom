/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "architect/Error.h"
#include "architect/MockPlanner.h"
#include "architect/RoomBlueprint.h"

#include <variant>

#include <catch2/catch_test_macros.hpp>

namespace tb::architect
{

TEST_CASE("MockPlanner")
{
  const auto prompt =
    "Create a stone room 12 metres wide, 10 metres deep and 5 metres tall, with one "
    "doorway centered on the southern wall.";

  SECTION("creates a deterministic grid-aligned room blueprint")
  {
    const auto first = MockPlanner::planRoom(prompt, 32.0, "stone");
    const auto second = MockPlanner::planRoom(prompt, 32.0, "stone");

    REQUIRE(std::holds_alternative<RoomBlueprint>(first));
    REQUIRE(std::holds_alternative<RoomBlueprint>(second));
    const auto& blueprint = std::get<RoomBlueprint>(first);
    CHECK(blueprint == std::get<RoomBlueprint>(second));
    CHECK(blueprint.interiorWidth == 384.0);
    CHECK(blueprint.interiorDepth == 320.0);
    CHECK(blueprint.interiorHeight == 160.0);
    CHECK(blueprint.wallThickness == 8.0);
    CHECK(blueprint.doorway.wall == WallSide::South);
    CHECK(blueprint.doorway.width == 40.0);
    CHECK(blueprint.doorway.height == 72.0);
  }

  SECTION("rejects prompts without all dimensions")
  {
    const auto result = MockPlanner::planRoom("Create a large stone room", 32.0, "stone");

    REQUIRE(std::holds_alternative<Error>(result));
    CHECK(std::get<Error>(result).code == ErrorCode::InvalidArgument);
  }


  SECTION("converts imperial dimensions and supplies an empty material")
  {
    const auto result = MockPlanner::planRoom(
      "Create a room 12 feet wide, 10 feet deep and 8 feet tall", 32.0, {});

    REQUIRE(std::holds_alternative<RoomBlueprint>(result));
    const auto& blueprint = std::get<RoomBlueprint>(result);
    CHECK(blueprint.interiorWidth == 120.0);
    CHECK(blueprint.interiorDepth == 96.0);
    CHECK(blueprint.interiorHeight == 80.0);
    CHECK(blueprint.material == "__TB_empty");
  }
  SECTION("rejects an unsafe scale")
  {
    const auto result = MockPlanner::planRoom(prompt, 0.0, "stone");

    REQUIRE(std::holds_alternative<Error>(result));
    CHECK(std::get<Error>(result).code == ErrorCode::InvalidArgument);
  }
}

} // namespace tb::architect
