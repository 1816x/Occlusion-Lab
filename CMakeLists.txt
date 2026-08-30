cmake_minimum_required(VERSION 3.25)

project(OcclusionLab VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OCCLUSION_BUILD_TESTS "Build native tests" ON)
option(OCCLUSION_FETCH_DEPENDENCIES "Fetch pinned lightweight dependencies" ON)
option(OCCLUSION_ENABLE_COLLISION "Build the FCL collision library" OFF)
option(OCCLUSION_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(OCCLUSION_ENABLE_SANITIZERS AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

if(OCCLUSION_BUILD_TESTS)
  include(CTest)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectWarnings)

if(OCCLUSION_FETCH_DEPENDENCIES)
  include(FetchContent)
  set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    Eigen3
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
    URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
    SYSTEM EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(Eigen3)
  if(OCCLUSION_BUILD_TESTS)
    set(BUILD_TESTING ON CACHE BOOL "" FORCE)
  endif()
else()
  find_package(Eigen3 3.4 CONFIG REQUIRED)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/native/libs/occlusion-core/CMakeLists.txt")
  add_subdirectory(native/libs/occlusion-core)
endif()

if(OCCLUSION_ENABLE_COLLISION)
  add_subdirectory(native/libs/occlusion-collision)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/native/apps/occlusion-cli/CMakeLists.txt")
  add_subdirectory(native/apps/occlusion-cli)
endif()

if(OCCLUSION_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/native/tests/core/CMakeLists.txt")
  if(OCCLUSION_FETCH_DEPENDENCIES)
    FetchContent_Declare(
      nlohmann_json
      URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
      URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
      SYSTEM EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(nlohmann_json)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
      URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
      SYSTEM EXCLUDE_FROM_ALL)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  else()
    find_package(GTest CONFIG REQUIRED)
    find_package(nlohmann_json 3.11.3 CONFIG REQUIRED)
  endif()
  add_subdirectory(native/tests/core)
  if(OCCLUSION_ENABLE_COLLISION)
    add_subdirectory(native/tests/collision)
  endif()
endif()
