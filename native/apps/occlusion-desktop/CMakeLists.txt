find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
find_package(VTK REQUIRED COMPONENTS CommonCore CommonDataModel FiltersSources RenderingCore RenderingOpenGL2)

add_executable(occlusion-desktop MACOSX_BUNDLE src/main.cpp)
target_link_libraries(occlusion-desktop PRIVATE occlusion::evaluation Qt6::Widgets Qt6::Concurrent ${VTK_LIBRARIES} occlusion_project_warnings)
target_compile_features(occlusion-desktop PRIVATE cxx_std_20)
vtk_module_autoinit(TARGETS occlusion-desktop MODULES ${VTK_LIBRARIES})

if(OCCLUSION_BUILD_TESTS AND NOT APPLE)
  add_test(NAME desktop-render-smoke
    COMMAND occlusion-desktop --smoke ${PROJECT_SOURCE_DIR}/fixtures/mesh-io)
  set_tests_properties(desktop-render-smoke PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  add_test(NAME desktop-rejects-missing-mesh-directory
    COMMAND occlusion-desktop --smoke ${PROJECT_SOURCE_DIR}/fixtures/mesh-io/missing)
  set_tests_properties(desktop-rejects-missing-mesh-directory PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen" WILL_FAIL TRUE)
endif()
