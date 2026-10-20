#include "mesh_files.hpp"
#include "occlusion/mesh_io/mesh_io.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace {
namespace fs = std::filesystem;

class MeshFileTest : public ::testing::Test {
protected:
  fs::path directory =
      fs::temp_directory_path() /
      ("occlusion-mesh-files-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  void SetUp() override { fs::create_directories(directory); }
  void TearDown() override { fs::remove_all(directory); }

  void copy_boxes() const {
    for (const auto* name : {"fixed-box.obj", "moving-box.obj"})
      fs::copy_file(fs::path{OCCLUSION_MESH_FIXTURE_DIR} / name, directory / name);
  }
};
} // namespace

TEST_F(MeshFileTest, LoadsTwoClosedMeshesAndEvaluatesPoseAndSweep) {
  copy_boxes();
  std::string error;
  auto evaluator =
      occlusion::cli::load_mesh_pair(directory, "fixed-box.obj", "moving-box.obj", error);
  ASSERT_TRUE(evaluator) << error;
  EXPECT_EQ(evaluator->compilation_count(), 2);
  const auto pose = evaluator->evaluate(
      {occlusion::core::Meters{0}, occlusion::core::Meters{0}, occlusion::core::Meters{0}});
  ASSERT_TRUE(pose);
  EXPECT_EQ(pose.value().classification, occlusion::core::ContactClassification::separated);
  const auto sweep = evaluator->evaluate_sweep(occlusion::core::SweepPreset::closing, 11);
  ASSERT_TRUE(sweep);
  EXPECT_EQ(sweep.value().frames.size(), 11);
  EXPECT_EQ(evaluator->compilation_count(), 2);
  EXPECT_EQ(evaluator->query_count(), 12);
}

TEST_F(MeshFileTest, RejectsMissingDirectoryAndFiles) {
  std::string error;
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory / "missing", "a.obj", "b.obj", error));
  EXPECT_NE(error.find("directory"), std::string::npos);
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, "a.obj", "b.obj", error));
  EXPECT_NE(error.find("regular file"), std::string::npos);
}

TEST_F(MeshFileTest, RejectsTraversalAndAbsoluteNames) {
  copy_boxes();
  std::string error;
  for (const auto* name : {"../fixed-box.obj", "sub/fixed-box.obj", "\\fixed-box.obj",
                           "C:fixed-box.obj", "/tmp/fixed-box.obj", "fixed-box.txt"}) {
    EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, name, "moving-box.obj", error));
    EXPECT_NE(error.find("filenames"), std::string::npos);
  }
}

TEST_F(MeshFileTest, RejectsSymlinks) {
  copy_boxes();
  std::error_code failure;
  fs::create_symlink(directory / "fixed-box.obj", directory / "link.obj", failure);
  if (failure)
    GTEST_SKIP() << failure.message();
  std::string error;
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, "link.obj", "moving-box.obj", error));
  EXPECT_NE(error.find("symlink"), std::string::npos);
}

TEST_F(MeshFileTest, RejectsOversizedFileBeforeParsing) {
  copy_boxes();
  fs::resize_file(directory / "fixed-box.obj", occlusion::mesh_io::default_maximum_input_bytes + 1);
  std::string error;
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, "fixed-box.obj", "moving-box.obj", error));
  EXPECT_NE(error.find("16 MiB"), std::string::npos);
}

TEST_F(MeshFileTest, ReportsParserAndCollisionValidationFailures) {
  copy_boxes();
  {
    std::ofstream broken(directory / "fixed-box.obj", std::ios::trunc);
    broken << "v nan 0 0\n";
  }
  std::string error;
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, "fixed-box.obj", "moving-box.obj", error));
  EXPECT_NE(error.find("line 1, vertex"), std::string::npos);
  {
    std::ofstream degenerate(directory / "fixed-box.obj", std::ios::trunc);
    degenerate << "v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n";
  }
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, "fixed-box.obj", "moving-box.obj", error));
  EXPECT_NE(error.find("fixed.triangles[0]"), std::string::npos);
}
