include(CTest)

add_subdirectory(deps/catch2)

include_directories(
  deps/catch2/include
  include
)

file(GLOB_RECURSE TEST_HEADERS "include/*.h")
add_executable(voxviz_test main.cpp ${TEST_HEADERS})
target_link_libraries(voxviz_test Catch2::Catch2)

include("deps/catch2/contrib/Catch.cmake")
catch_discover_tests(voxviz_test)

include("deps/catch2/contrib/ParseAndAddCatchTests.cmake")
ParseAndAddCatchTests(voxviz_test)
