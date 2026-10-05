configure_file("${PROJECT_SOURCE_DIR}/fixtures/parity/contact-normalization-v1.json" "${CMAKE_CURRENT_BINARY_DIR}/contact-normalization-v1.json" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/fixtures/parity/evaluated-sweep-summary-v1.json" "${CMAKE_CURRENT_BINARY_DIR}/evaluated-sweep-summary-v1.json" COPYONLY)
add_executable(occlusion-evaluation-tests evaluation_tests.cpp public_headers.cpp)
target_link_libraries(occlusion-evaluation-tests PRIVATE occlusion::evaluation GTest::gtest_main nlohmann_json::nlohmann_json occlusion_project_warnings)
target_compile_definitions(occlusion-evaluation-tests PRIVATE OCCLUSION_NORMALIZATION_FIXTURE_SOURCE="${PROJECT_SOURCE_DIR}/fixtures/parity/contact-normalization-v1.json" OCCLUSION_NORMALIZATION_FIXTURE_COPY="${CMAKE_CURRENT_BINARY_DIR}/contact-normalization-v1.json" OCCLUSION_SWEEP_SUMMARY_FIXTURE_SOURCE="${PROJECT_SOURCE_DIR}/fixtures/parity/evaluated-sweep-summary-v1.json" OCCLUSION_SWEEP_SUMMARY_FIXTURE_COPY="${CMAKE_CURRENT_BINARY_DIR}/evaluated-sweep-summary-v1.json")
target_compile_features(occlusion-evaluation-tests PRIVATE cxx_std_20)
include(GoogleTest)
gtest_discover_tests(occlusion-evaluation-tests DISCOVERY_MODE PRE_TEST)

add_test(NAME cli-evaluate-synthetic-pose
  COMMAND occlusion-cli --evaluate-fixture pose 0 0 0)
set_tests_properties(cli-evaluate-synthetic-pose PROPERTIES
  PASS_REGULAR_EXPRESSION "\"fixture\":\"synthetic-boxes-v1\".*\"classification\":\"separated\"")

add_test(NAME cli-evaluate-synthetic-sweep
  COMMAND occlusion-cli --evaluate-fixture sweep closing 11)
set_tests_properties(cli-evaluate-synthetic-sweep PROPERTIES
  PASS_REGULAR_EXPRESSION "\"requestedFrameCount\":11.*\"totalFrameCount\":11")

add_test(NAME cli-rejects-non-finite-pose
  COMMAND occlusion-cli --evaluate-fixture pose nan 0 0)
set_tests_properties(cli-rejects-non-finite-pose PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli-rejects-out-of-range-sweep
  COMMAND occlusion-cli --evaluate-fixture sweep closing 1)
set_tests_properties(cli-rejects-out-of-range-sweep PROPERTIES WILL_FAIL TRUE)
