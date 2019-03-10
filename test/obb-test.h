#include <catch2/catch.hpp>

#include <glm/glm.hpp>
#include <obb.h>

TEST_CASE("[obb] chaining", "obb") {

  OBB *obb = new OBB();
  
  OBB *move = obb->move(glm::vec3(10.0, 0.0, 0.0));
  REQUIRE(obb == move);

  OBB *rotate = obb->move(glm::vec3(10.0, 0.0, 0.0));
  REQUIRE(obb == rotate);


}
