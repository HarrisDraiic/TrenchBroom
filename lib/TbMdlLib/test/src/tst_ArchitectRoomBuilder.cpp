/*
 Copyright (C) 2026 TrenchBroom Architect contributors

 This file is part of TrenchBroom Architect, an unofficial TrenchBroom fork.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
*/

#include "architect/MockPlanner.h"
#include "architect/RoomBlueprint.h"
#include "mdl/ArchitectRoomBuilder.h"
#include "mdl/LayerNode.h"
#include "mdl/Map.h"
#include "mdl/MapFixture.h"
#include "mdl/WorldNode.h"

#include <string>
#include <variant>

#include <catch2/catch_test_macros.hpp>

namespace tb::mdl
{

TEST_CASE("ArchitectRoomBuilder")
{
  auto fixture = MapFixture{};
  auto& map = fixture.create();
  const auto prompt =
    "Create a stone room 12 metres wide, 10 metres deep and 5 metres tall, with one "
    "doorway centered on the southern wall.";
  const auto plan = architect::MockPlanner::planRoom(prompt, 32.0, "stone");
  REQUIRE(std::holds_alternative<architect::RoomBlueprint>(plan));
  auto blueprint = std::get<architect::RoomBlueprint>(plan);

  SECTION("creates one deterministic undoable room transaction")
  {
    const auto result = createArchitectRoom(map, blueprint);

    REQUIRE(result.is_success());
    CHECK(result.value().size() == 8u);
    CHECK(map.worldNode().defaultLayer()->children().size() == 8u);
    REQUIRE(map.undoCommandName() != nullptr);
    CHECK(*map.undoCommandName() == "Architect: Create Room");

    map.undoCommand();
    CHECK(map.worldNode().defaultLayer()->children().empty());

    map.redoCommand();
    CHECK(map.worldNode().defaultLayer()->children().size() == 8u);
  }

  SECTION("leaves the map unchanged for an invalid blueprint")
  {
    blueprint.interiorWidth = 0.0;

    const auto result = createArchitectRoom(map, blueprint);

    CHECK(result.is_error());
    CHECK(map.worldNode().defaultLayer()->children().empty());
    CHECK_FALSE(map.canUndoCommand());
  }

  SECTION("leaves the map unchanged when world bounds would be exceeded")
  {
    blueprint.origin = {1.0e9, 0.0, 0.0};

    const auto result = createArchitectRoom(map, blueprint);

    CHECK(result.is_error());
    CHECK(map.worldNode().defaultLayer()->children().empty());
    CHECK_FALSE(map.canUndoCommand());
  }
}

} // namespace tb::mdl
