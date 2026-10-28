#include "mesh_files.hpp"

#include "occlusion/evaluation/mesh_session.hpp"

namespace occlusion::cli {
std::optional<occlusion::evaluation::PoseEvaluator>
load_mesh_pair(const std::filesystem::path& directory, std::string_view fixed_name,
               std::string_view moving_name, std::string& error) {
  mesh_io::ObjFileError problem;
  auto session =
      evaluation::load_mesh_evaluation_session(directory, fixed_name, moving_name, problem);
  if (!session) {
    error = problem.filename.empty() ? problem.field + ": " + problem.message
                                     : mesh_io::describe_obj_file_error(problem);
    return std::nullopt;
  }
  return session->evaluator;
}
} // namespace occlusion::cli
