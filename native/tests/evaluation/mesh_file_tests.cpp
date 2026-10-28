#include "mesh_files.hpp"
#include "occlusion/evaluation/mesh_session.hpp"
#include "occlusion/mesh_io/mesh_io.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
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

TEST_F(MeshFileTest, SharedSessionOwnsExactMeshesAfterSourceFilesDisappear) {
  copy_boxes();
  occlusion::mesh_io::ObjFileError error;
  auto session = occlusion::evaluation::load_mesh_evaluation_session(directory, "fixed-box.obj",
                                                                     "moving-box.obj", error);
  ASSERT_TRUE(session) << error.message;
  EXPECT_EQ(session->fixed_mesh.vertices_meters.size(), 8);
  EXPECT_EQ(session->fixed_mesh.triangles.size(), 12);
  EXPECT_EQ(session->moving_mesh.vertices_meters.size(), 8);
  fs::remove_all(directory);
  const auto pose = session->evaluator.evaluate(
      {occlusion::core::Meters{0}, occlusion::core::Meters{0}, occlusion::core::Meters{0}});
  ASSERT_TRUE(pose);
  EXPECT_EQ(pose.value().classification, occlusion::core::ContactClassification::separated);
  const auto sweep = session->evaluator.evaluate_sweep(occlusion::core::SweepPreset::closing, 11);
  ASSERT_TRUE(sweep);
  EXPECT_EQ(sweep.value().frames.size(), 11);
  EXPECT_EQ(session->evaluator.compilation_count(), 2);
}

TEST_F(MeshFileTest, SharedSessionReportsStructuredLoadError) {
  occlusion::mesh_io::ObjFileError error;
  EXPECT_FALSE(occlusion::evaluation::load_mesh_evaluation_session(directory, "../fixed-box.obj",
                                                                   "moving-box.obj", error));
  EXPECT_EQ(error.code, "invalid_name");
  EXPECT_EQ(error.field, "filename");
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
  // Preserve the closed face graph, but collapse a vertex onto its neighbor.
  std::ifstream fixture(fs::path{OCCLUSION_MESH_FIXTURE_DIR} / "fixed-box.obj");
  ASSERT_TRUE(fixture);
  std::string contents{std::istreambuf_iterator<char>{fixture}, std::istreambuf_iterator<char>{}};
  const auto vertex = contents.find("v 0.05 -0.05 -0.05");
  ASSERT_NE(vertex, std::string::npos);
  contents.replace(vertex, sizeof("v 0.05 -0.05 -0.05") - 1, "v -0.05 -0.05 -0.05");
  {
    std::ofstream degenerate(directory / "fixed-box.obj", std::ios::trunc);
    degenerate << contents;
  }
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, "fixed-box.obj", "moving-box.obj", error));
  EXPECT_NE(error.find("fixed.triangles[0]"), std::string::npos);
}

TEST_F(MeshFileTest, RejectsOpenMeshBeforeCollisionCompilation) {
  copy_boxes();
  std::ifstream fixture(directory / "fixed-box.obj");
  ASSERT_TRUE(fixture);
  std::string contents{std::istreambuf_iterator<char>{fixture}, std::istreambuf_iterator<char>{}};
  const auto face = contents.find("f 2 7 6\n");
  ASSERT_NE(face, std::string::npos);
  contents.erase(face, sizeof("f 2 7 6\n") - 1);
  {
    std::ofstream open(directory / "fixed-box.obj", std::ios::trunc);
    open << contents;
  }
  std::string error;
  EXPECT_FALSE(occlusion::cli::load_mesh_pair(directory, "fixed-box.obj", "moving-box.obj", error));
  EXPECT_NE(error.find("fixed-box.obj: edges["), std::string::npos);
  EXPECT_NE(error.find("two incident faces"), std::string::npos);
}
