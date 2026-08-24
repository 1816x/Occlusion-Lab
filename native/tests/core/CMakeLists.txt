add_executable(occlusion-core-tests core_tests.cpp public_headers.cpp)
target_link_libraries(occlusion-core-tests PRIVATE occlusion::core GTest::gtest_main occlusion_project_warnings)
target_compile_features(occlusion-core-tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(occlusion-core-tests DISCOVERY_MODE PRE_TEST)

add_test(NAME cli-self-check COMMAND occlusion-cli --self-check)
set_tests_properties(cli-self-check PROPERTIES PASS_REGULAR_EXPRESSION "^occlusion-cli self-check: PASS")
add_test(NAME cli-version COMMAND occlusion-cli --version)
set_tests_properties(cli-version PROPERTIES PASS_REGULAR_EXPRESSION "^Occlusion Lab native 0.4.0")
