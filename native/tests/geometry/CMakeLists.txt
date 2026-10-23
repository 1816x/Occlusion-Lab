add_executable(occlusion-geometry-tests geometry_tests.cpp inspection_tests.cpp public_headers.cpp)
target_link_libraries(occlusion-geometry-tests PRIVATE occlusion::geometry occlusion-mesh-inspection GTest::gtest_main nlohmann_json::nlohmann_json occlusion_project_warnings)
target_compile_definitions(occlusion-geometry-tests PRIVATE OCCLUSION_MESH_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/fixtures/mesh-io")
target_compile_features(occlusion-geometry-tests PRIVATE cxx_std_20)
include(GoogleTest)
gtest_discover_tests(occlusion-geometry-tests DISCOVERY_MODE PRE_TEST)

add_test(NAME cli-inspect-valid-box
  COMMAND occlusion-cli --inspect-mesh ${PROJECT_SOURCE_DIR}/fixtures/mesh-io fixed-box.obj)
set_tests_properties(cli-inspect-valid-box PROPERTIES
  PASS_REGULAR_EXPRESSION "\"schema\":\"occlusion-mesh-inspection\".*\"status\":\"valid\"")

add_test(NAME cli-inspect-rejects-intersection
  COMMAND occlusion-cli --inspect-mesh ${PROJECT_SOURCE_DIR}/fixtures/mesh-io intersecting-boxes.obj)
set_tests_properties(cli-inspect-rejects-intersection PROPERTIES WILL_FAIL TRUE)
