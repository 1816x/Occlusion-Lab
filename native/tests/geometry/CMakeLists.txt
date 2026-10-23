add_executable(occlusion-geometry-tests geometry_tests.cpp inspection_tests.cpp public_headers.cpp)
target_link_libraries(occlusion-geometry-tests PRIVATE occlusion::geometry occlusion-mesh-inspection GTest::gtest_main nlohmann_json::nlohmann_json occlusion_project_warnings)
target_compile_definitions(occlusion-geometry-tests PRIVATE OCCLUSION_MESH_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/fixtures/mesh-io")
target_compile_features(occlusion-geometry-tests PRIVATE cxx_std_20)
include(GoogleTest)
gtest_discover_tests(occlusion-geometry-tests DISCOVERY_MODE PRE_TEST)

add_test(NAME cli-inspection-contract
  COMMAND ${CMAKE_COMMAND}
    -DCLI=$<TARGET_FILE:occlusion-cli>
    -DMESH_DIR=${PROJECT_SOURCE_DIR}/fixtures/mesh-io
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_inspection_contract.cmake)
