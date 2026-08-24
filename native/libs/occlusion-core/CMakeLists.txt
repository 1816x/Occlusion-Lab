add_library(occlusion-core STATIC src/domain.cpp)
add_library(occlusion::core ALIAS occlusion-core)

target_include_directories(occlusion-core PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_link_libraries(occlusion-core PUBLIC Eigen3::Eigen PRIVATE occlusion_project_warnings)
target_compile_features(occlusion-core PUBLIC cxx_std_20)
