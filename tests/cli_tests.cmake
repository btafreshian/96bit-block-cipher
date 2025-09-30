# SPDX-License-Identifier: MIT
# Integration tests for cube96_cli exit codes and hex parsing.

include(CMakeParseArguments)

if(NOT DEFINED CLI_EXECUTABLE)
  message(FATAL_ERROR "CLI_EXECUTABLE not provided")
endif()
if(NOT DEFINED KAT_KEY OR NOT DEFINED KAT_PLAIN OR NOT DEFINED KAT_CIPHER)
  message(FATAL_ERROR "KAT vectors not provided")
endif()

set(_cases)

function(run_cli_case name expected_exit)
  cmake_parse_arguments(_CASE "" "" "ARGS;EXPECT_STDOUT;EXPECT_STDERR" ${ARGN})
  execute_process(
    COMMAND "${CLI_EXECUTABLE}" ${_CASE_ARGS}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
  )
  if(NOT _result EQUAL ${expected_exit})
    message(FATAL_ERROR "${name}: exit ${_result} != ${expected_exit}\nstdout=${_stdout}\nstderr=${_stderr}")
  endif()
  if(_CASE_EXPECT_STDOUT)
    foreach(_needle IN LISTS _CASE_EXPECT_STDOUT)
      string(FIND "${_stdout}" "${_needle}" _pos)
      if(_pos EQUAL -1)
        message(FATAL_ERROR "${name}: stdout missing '${_needle}'\nstdout=${_stdout}")
      endif()
    endforeach()
  endif()
  if(_CASE_EXPECT_STDERR)
    foreach(_needle IN LISTS _CASE_EXPECT_STDERR)
      string(FIND "${_stderr}" "${_needle}" _pos)
      if(_pos EQUAL -1)
        message(FATAL_ERROR "${name}: stderr missing '${_needle}'\nstderr=${_stderr}")
      endif()
    endforeach()
  endif()
endfunction()

run_cli_case("usage" 64 ARGS "enc" EXPECT_STDERR "Usage:")
run_cli_case("invalid-mode" 66 ARGS "foo" "${KAT_KEY}" "${KAT_PLAIN}" EXPECT_STDERR "Unknown mode")
run_cli_case("bad-key-length" 65 ARGS "enc" "${KAT_KEY}00" "${KAT_PLAIN}" EXPECT_STDERR "Invalid key")
run_cli_case("bad-data-length" 65 ARGS "enc" "${KAT_KEY}" "${KAT_PLAIN}00" EXPECT_STDERR "Invalid plaintext")
run_cli_case("bad-hex" 65 ARGS "enc" "${KAT_KEY}" "${KAT_PLAIN}GG" EXPECT_STDERR "Invalid plaintext")
run_cli_case("encrypt" 0 ARGS "enc" "${KAT_KEY}" "${KAT_PLAIN}" EXPECT_STDOUT "${KAT_CIPHER}" EXPECT_STDERR "Research cipher")
run_cli_case("decrypt" 0 ARGS "dec" "${KAT_KEY}" "${KAT_CIPHER}" EXPECT_STDOUT "${KAT_PLAIN}" EXPECT_STDERR "Research cipher")
