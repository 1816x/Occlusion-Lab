add_executable(occlusion-cli src/main.cpp)
target_link_libraries(occlusion-cli PRIVATE occlusion::core occlusion_project_warnings)
target_compile_features(occlusion-cli PRIVATE cxx_std_20)
