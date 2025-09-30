cmake_minimum_required(VERSION 3.15)

project(cube96 VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

option(CUBE96_ENABLE_FAST_IMPL "Build the table-based fast implementation" ON)
option(CUBE96_FORCE_CONSTANT_TIME
       "Force the hardened implementation and disable fast tables" OFF)

if(CUBE96_FORCE_CONSTANT_TIME)
  set(CUBE96_ENABLE_FAST_IMPL OFF CACHE BOOL "Build the table-based fast implementation" FORCE)
endif()

set(CUBE96_LAYOUT "zslice" CACHE STRING "State layout mapping (zslice|rowmajor)")
set_property(CACHE CUBE96_LAYOUT PROPERTY STRINGS zslice rowmajor)

function(cube96_enable_strict_warnings target)
  target_compile_options(${target} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wpedantic>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Werror>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wshadow>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wcast-align>
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wformat=2>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->)
endfunction()

set(cube96_sources
  src/cipher.cpp
  src/endian.cpp
  src/impl_hardened.cpp
  src/key_schedule.cpp
  src/perm.cpp
  src/sbox.cpp
)

if(CUBE96_ENABLE_FAST_IMPL)
  list(APPEND cube96_sources src/impl_fast.cpp)
endif()

add_library(cube96 ${cube96_sources})
add_library(cube96::cube96 ALIAS cube96)

target_compile_features(cube96 PUBLIC cxx_std_17)

target_include_directories(cube96
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

cube96_enable_strict_warnings(cube96)

if(CUBE96_ENABLE_FAST_IMPL)
  target_compile_definitions(cube96 PUBLIC CUBE96_HAVE_FAST_IMPL=1)
else()
  target_compile_definitions(cube96 PUBLIC CUBE96_DISABLE_FAST_IMPL=1)
endif()

if(CUBE96_FORCE_CONSTANT_TIME)
  target_compile_definitions(cube96 PUBLIC CUBE96_FORCE_CONSTANT_TIME=1)
endif()

if(CUBE96_LAYOUT STREQUAL "rowmajor")
  target_compile_definitions(cube96 PUBLIC CUBE96_LAYOUT_ROWMAJOR)
elseif(CUBE96_LAYOUT STREQUAL "zslice")
  target_compile_definitions(cube96 PUBLIC CUBE96_LAYOUT_ZSLICE)
else()
  message(FATAL_ERROR "Unknown CUBE96_LAYOUT='${CUBE96_LAYOUT}'. Use 'zslice' or 'rowmajor'.")
endif()

set(CUBE96_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
if(CUBE96_LAYOUT STREQUAL "rowmajor")
  set(CUBE96_KAT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vectors/cube96_kats_rowmajor.csv")
else()
  set(CUBE96_KAT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vectors/cube96_kats_zslice.csv")
endif()

if(NOT EXISTS "${CUBE96_KAT_FILE}")
  message(FATAL_ERROR "Missing known-answer test file: ${CUBE96_KAT_FILE}")
endif()

file(STRINGS "${CUBE96_KAT_FILE}" _cube96_kat_lines)
foreach(line IN LISTS _cube96_kat_lines)
  if(line MATCHES "^[0-9A-Fa-f]")
    string(REPLACE "," ";" _cube96_kat_fields "${line}")
    list(GET _cube96_kat_fields 0 CUBE96_KAT_KEY)
    list(GET _cube96_kat_fields 1 CUBE96_KAT_PLAIN)
    list(GET _cube96_kat_fields 2 CUBE96_KAT_CIPHER)
    break()
  endif()
endforeach()
if(NOT DEFINED CUBE96_KAT_KEY)
  message(FATAL_ERROR "Failed to parse first entry from ${CUBE96_KAT_FILE}")
endif()

add_executable(cube96_cli tools/cube96_cli.cpp)
target_link_libraries(cube96_cli PRIVATE cube96)
cube96_enable_strict_warnings(cube96_cli)

add_executable(cube96_bench bench/bench_throughput.cpp)
target_link_libraries(cube96_bench PRIVATE cube96)
cube96_enable_strict_warnings(cube96_bench)

if(BUILD_TESTING)
  set(TEST_SOURCES
    tests/test_roundtrip.cpp
    tests/test_vectors.cpp
    tests/test_permutation.cpp
    tests/test_kdf.cpp
    tests/test_kdf_deterministic.cpp
    tests/test_avalanche.cpp
  )

  foreach(test_src IN LISTS TEST_SOURCES)
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE cube96)
    target_compile_definitions(${test_name} PRIVATE CUBE96_PROJECT_ROOT="${CUBE96_PROJECT_ROOT}")
    cube96_enable_strict_warnings(${test_name})
    add_test(NAME ${test_name} COMMAND ${test_name})

    set(test_labels "")
    if(test_name STREQUAL "test_vectors")
      list(APPEND test_labels KAT)
    elseif(test_name STREQUAL "test_permutation")
      list(APPEND test_labels PERM)
    elseif(test_name STREQUAL "test_kdf" OR test_name STREQUAL "test_kdf_deterministic")
      list(APPEND test_labels HKDF)
    elseif(test_name STREQUAL "test_roundtrip" OR test_name STREQUAL "test_avalanche")
      list(APPEND test_labels CT)
    endif()

    if(test_labels)
      set_property(TEST ${test_name} PROPERTY LABELS ${test_labels})
    endif()
  endforeach()

  add_test(
    NAME cli_integration
    COMMAND ${CMAKE_COMMAND}
      -DCLI_EXECUTABLE=$<TARGET_FILE:cube96_cli>
      -DKAT_KEY=${CUBE96_KAT_KEY}
      -DKAT_PLAIN=${CUBE96_KAT_PLAIN}
      -DKAT_CIPHER=${CUBE96_KAT_CIPHER}
      -P ${CUBE96_PROJECT_ROOT}/tests/cli_tests.cmake)
  set_property(TEST cli_integration PROPERTY LABELS CLI)
endif()

install(TARGETS cube96 cube96_cli cube96_bench
        EXPORT cube96Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT cube96Targets
        FILE cube96Targets.cmake
        NAMESPACE cube96::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cube96)

configure_package_config_file(
  cmake/cube96Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cube96Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cube96
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/cube96ConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/cube96Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cube96ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cube96)

export(EXPORT cube96Targets
       FILE ${CMAKE_CURRENT_BINARY_DIR}/cube96Targets.cmake
       NAMESPACE cube96::)
