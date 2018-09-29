#include <catch2/catch.hpp>

#include "volume.h"
/*
TEST_CASE("computeBrickOffset (stock:0,0,0; cutter: 8,8,8)", "[volume]") {

  Volume *tool = new Volume(glm::vec3(0.5));
  Brick *toolBrick = tool->AddBrick(glm::ivec3(0));

  Volume *stock = new Volume(glm::vec3(0.0));
  Brick *stockBrick = stock->AddBrick(glm::ivec3(0));

  glm::vec3 stockOffset, toolOffset;


  stock->computeBrickOffset(
    tool,
    stockBrick,
    toolBrick,
    &stockOffset,
    &toolOffset
  );

  REQUIRE(stockOffset.x == 8);
  REQUIRE(stockOffset.y == 8);
  REQUIRE(stockOffset.z == 8);

  REQUIRE(toolOffset.x == 0);
  REQUIRE(toolOffset.y == 0);
  REQUIRE(toolOffset.z == 0);
}

TEST_CASE("computeBrickOffset (stock:8,8,8; cutter: 0,0,0)", "[volume]") {

  Volume *tool = new Volume(glm::vec3(0.5));
  Brick *toolBrick = tool->AddBrick(glm::ivec3(0));

  Volume *stock = new Volume(glm::vec3(0.0));
  Brick *stockBrick = stock->AddBrick(glm::ivec3(0));

  glm::vec3 stockOffset, toolOffset;


  stock->computeBrickOffset(
    tool,
    stockBrick,
    toolBrick,
    &stockOffset,
    &toolOffset
  );

  REQUIRE(stockOffset.x == 0);
  REQUIRE(stockOffset.y == 0);
  REQUIRE(stockOffset.z == 0);

  REQUIRE(toolOffset.x == 8);
  REQUIRE(toolOffset.y == 8);
  REQUIRE(toolOffset.z == 8);
}
*/