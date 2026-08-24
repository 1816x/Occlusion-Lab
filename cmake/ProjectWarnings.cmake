add_library(occlusion_project_warnings INTERFACE)

if(MSVC)
  target_compile_options(occlusion_project_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(
    occlusion_project_warnings
    INTERFACE
      -Wall
      -Wextra
      -Wpedantic
      -Wconversion
      -Wsign-conversion)
endif()
