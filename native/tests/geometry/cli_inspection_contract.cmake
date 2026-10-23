function(expect_inspection mesh expected_exit expected_status expected_code)
  execute_process(
    COMMAND "${CLI}" --inspect-mesh "${MESH_DIR}" "${mesh}"
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
  if(NOT exit_code STREQUAL expected_exit)
    message(FATAL_ERROR "${mesh}: expected exit ${expected_exit}, got ${exit_code}: ${error}")
  endif()
  if(NOT error STREQUAL "")
    message(FATAL_ERROR "${mesh}: unexpected stderr: ${error}")
  endif()
  if(NOT output MATCHES "\n$")
    message(FATAL_ERROR "${mesh}: expected one newline-terminated JSON document")
  endif()
  string(JSON schema GET "${output}" schema)
  string(JSON version GET "${output}" schemaVersion)
  string(JSON status GET "${output}" status)
  if(NOT schema STREQUAL "occlusion-mesh-inspection" OR NOT version EQUAL 1 OR
     NOT status STREQUAL expected_status)
    message(FATAL_ERROR "${mesh}: unexpected inspection envelope: ${output}")
  endif()
  if(expected_code STREQUAL "")
    string(JSON error_type ERROR_VARIABLE json_error TYPE "${output}" error)
    if(NOT json_error)
      message(FATAL_ERROR "${mesh}: valid response unexpectedly has an error object")
    endif()
  else()
    string(JSON code GET "${output}" error code)
    if(NOT code STREQUAL expected_code)
      message(FATAL_ERROR "${mesh}: expected ${expected_code}, got ${code}")
    endif()
  endif()
endfunction()

expect_inspection("fixed-box.obj" 0 "valid" "")
expect_inspection("intersecting-boxes.obj" 2 "invalid" "self_intersection")
expect_inspection("missing.obj" 2 "invalid" "invalid_file")
expect_inspection("../fixed-box.obj" 2 "invalid" "invalid_name")

execute_process(
  COMMAND "${CLI}" --inspect-mesh "${MESH_DIR}"
  RESULT_VARIABLE usage_exit
  OUTPUT_VARIABLE usage_output
  ERROR_VARIABLE usage_error)
if(NOT usage_exit STREQUAL "2" OR NOT usage_output STREQUAL "" OR
   NOT usage_error MATCHES "^Usage: occlusion-cli")
  message(FATAL_ERROR "wrong-argument usage contract failed: ${usage_exit}, ${usage_output}, ${usage_error}")
endif()
