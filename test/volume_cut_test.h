#include <catch2/catch.hpp>

#include "volume.h"

TEST_CASE("Compute overlap in index space", "[volume]") {

  Volume *tool = new Volume(glm::vec3(0.5));
  tool->AddBrick(glm::ivec3(0));

  Volume *stock = new Volume(glm::vec3(0.0));
  tool->AddBrick(glm::ivec3(0));

  stock->cut(tool, nullptr);


}