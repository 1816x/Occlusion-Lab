#include "mesh_inspection.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>

namespace {
namespace fs = std::filesystem;

class InspectionTest : public ::testing::Test {
protected:
  fs::path directory =
      fs::temp_directory_path() /
      ("occlusion-inspection-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  void SetUp() override { fs::create_directories(directory); }
  void TearDown() override { fs::remove_all(directory); }
};
} // namespace

TEST_F(InspectionTest, ReturnsVersionedSuccessForClosedBox) {
  fs::copy_file(fs::path{OCCLUSION_MESH_FIXTURE_DIR} / "fixed-box.obj", directory / "box.obj");
  const auto result = occlusion::cli::inspect_synthetic_obj(directory, "box.obj");
  ASSERT_TRUE(result.valid);
  EXPECT_EQ(
      result.json,
      "{\"schema\":\"occlusion-mesh-inspection\",\"schemaVersion\":1,\"status\":\"valid\"}\n");
  const auto json = nlohmann::json::parse(result.json);
  EXPECT_EQ(json.at("schema"), "occlusion-mesh-inspection");
  EXPECT_EQ(json.at("schemaVersion"), 1);
  EXPECT_EQ(json.at("status"), "valid");
  EXPECT_FALSE(json.contains("error"));
}

TEST_F(InspectionTest, ReportsSelfIntersectionWithoutChangingInput) {
  const auto source = fs::path{OCCLUSION_MESH_FIXTURE_DIR} / "intersecting-boxes.obj";
  fs::copy_file(source, directory / "overlap.obj");
  const auto original_size = fs::file_size(directory / "overlap.obj");
  const auto result = occlusion::cli::inspect_synthetic_obj(directory, "overlap.obj");
  ASSERT_FALSE(result.valid);
  EXPECT_EQ(result.json,
            "{\"schema\":\"occlusion-mesh-inspection\",\"schemaVersion\":1,\"status\":\"invalid\","
            "\"error\":{\"code\":\"self_intersection\",\"field\":\"mesh\",\"line\":0,"
            "\"message\":\"surface has intersecting triangles\"}}\n");
  const auto json = nlohmann::json::parse(result.json);
  EXPECT_EQ(json.at("status"), "invalid");
  EXPECT_EQ(json.at("error").at("code"), "self_intersection");
  EXPECT_EQ(json.at("error").at("field"), "mesh");
  EXPECT_EQ(json.at("error").at("line"), 0);
  EXPECT_EQ(fs::file_size(directory / "overlap.obj"), original_size);
}

TEST_F(InspectionTest, ReportsLineNumberAndTopologyErrors) {
  {
    std::ofstream malformed(directory / "malformed.obj");
    malformed << "v nan 0 0\n";
  }
  auto result = occlusion::cli::inspect_synthetic_obj(directory, "malformed.obj");
  ASSERT_FALSE(result.valid);
  auto json = nlohmann::json::parse(result.json);
  EXPECT_EQ(json.at("error").at("code"), "non_finite_coordinate");
  EXPECT_EQ(json.at("error").at("field"), "vertex");
  EXPECT_EQ(json.at("error").at("line"), 1);

  {
    std::ofstream open(directory / "open.obj");
    open << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
  }
  result = occlusion::cli::inspect_synthetic_obj(directory, "open.obj");
  ASSERT_FALSE(result.valid);
  json = nlohmann::json::parse(result.json);
  EXPECT_EQ(json.at("error").at("code"), "invalid_topology");
  EXPECT_EQ(json.at("error").at("line"), 0);
}

TEST_F(InspectionTest, AppliesSameFilePolicyAsEvaluation) {
  const auto traversal = occlusion::cli::inspect_synthetic_obj(directory, "../box.obj");
  ASSERT_FALSE(traversal.valid);
  EXPECT_EQ(nlohmann::json::parse(traversal.json).at("error").at("code"), "invalid_name");
  const auto missing = occlusion::cli::inspect_synthetic_obj(directory, "missing.obj");
  ASSERT_FALSE(missing.valid);
  EXPECT_EQ(nlohmann::json::parse(missing.json).at("error").at("code"), "invalid_file");
}
