#include "occlusion/mesh_io/obj_files.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {
namespace fs = std::filesystem;
using occlusion::mesh_io::load_synthetic_obj;
using occlusion::mesh_io::ObjFileError;

const fs::path fixture_dir{OCCLUSION_MESH_FIXTURE_DIR};

TEST(ObjFileAdapter, LoadsClosedMeshWithoutCollisionOrGeometryLibraries) {
  ObjFileError error;
  const auto mesh = load_synthetic_obj(fixture_dir, "fixed-box.obj", error);
  ASSERT_TRUE(mesh) << error.message;
  EXPECT_EQ(mesh->vertices_meters.size(), 8);
  EXPECT_EQ(mesh->triangles.size(), 12);
}

TEST(ObjFileAdapter, ReportsStructuredNameAndFileErrors) {
  ObjFileError error;
  EXPECT_FALSE(load_synthetic_obj(fixture_dir, "../fixed-box.obj", error));
  EXPECT_EQ(error.code, "invalid_name");
  EXPECT_EQ(error.field, "filename");
  EXPECT_FALSE(load_synthetic_obj(fixture_dir, "missing.obj", error));
  EXPECT_EQ(error.code, "invalid_file");
  EXPECT_EQ(error.filename, "missing.obj");
}

TEST(ObjFileAdapter, RejectsOpenMeshBeforePublishingIt) {
  const auto directory =
      fs::temp_directory_path() /
      ("occlusion-obj-adapter-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(directory);
  {
    std::ofstream file(directory / "open.obj");
    file << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
  }
  ObjFileError error;
  const auto mesh = load_synthetic_obj(directory, "open.obj", error);
  fs::remove_all(directory);
  EXPECT_FALSE(mesh);
  EXPECT_EQ(error.code, "invalid_topology");
  EXPECT_EQ(error.filename, "open.obj");
}
} // namespace
