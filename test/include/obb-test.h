#include <catch2/catch.hpp>

#include <math.h>
#include <glm/glm.hpp>
#include <obb.h>

using namespace glm;

TEST_CASE("[obb] basic", "obb") {

  OBB *obb = new OBB(
    vec3(5.0),
    vec3(10.0)
  );
  
  OBB *move = obb->move(vec3(10.0, 0.0, 0.0));
  REQUIRE(obb == move);
  REQUIRE(all(equal(move->getPosition(), vec3(15.0, 5.0, 5.0))));

  vec3 rotateVec(0.25, 0.0, 0.0);
  OBB *rotate = obb->rotate(rotateVec);
  REQUIRE(obb == rotate);
  REQUIRE(all(equal(rotate->getRotation(), rotateVec)));

  vec3 resizeVec(20.0);
  OBB *resize = obb->resize(resizeVec);
  REQUIRE(obb == resize);
  REQUIRE(all(equal(resize->getRadius(), resizeVec)));w


}
