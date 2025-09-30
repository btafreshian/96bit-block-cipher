# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.12)
project(cube96 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cube96
  src/cipher.cpp
  src/endian.cpp
  src/impl_fast.cpp
  src/impl_hardened.cpp
  src/key_schedule.cpp
  src/perm.cpp
  src/sbox.cpp)

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
    $<$<CXX_COMPILER_ID:MSVC>:/WX>)
endfunction()

target_include_directories(cube96 PUBLIC include)

target_compile_features(cube96 PUBLIC cxx_std_17)

cube96_enable_strict_warnings(cube96)

set(CUBE96_LAYOUT "zslice" CACHE STRING "State layout mapping (zslice|rowmajor)")
set_property(CACHE CUBE96_LAYOUT PROPERTY STRINGS zslice rowmajor)

if(CUBE96_LAYOUT STREQUAL "rowmajor")
  target_compile_definitions(cube96 PUBLIC CUBE96_LAYOUT_ROWMAJOR)
elseif(CUBE96_LAYOUT STREQUAL "zslice")
  target_compile_definitions(cube96 PUBLIC CUBE96_LAYOUT_ZSLICE)
else()
  message(FATAL_ERROR "Unknown CUBE96_LAYOUT='${CUBE96_LAYOUT}'. Use 'zslice' or 'rowmajor'.")
endif()

add_executable(cube96_cli tools/cube96_cli.cpp)
target_link_libraries(cube96_cli PRIVATE cube96)
cube96_enable_strict_warnings(cube96_cli)

add_executable(cube96_gen_kats tools/gen_kats.cpp)
target_link_libraries(cube96_gen_kats PRIVATE cube96)
cube96_enable_strict_warnings(cube96_gen_kats)

add_executable(cube96_bench bench/bench_throughput.cpp)
target_link_libraries(cube96_bench PRIVATE cube96)
cube96_enable_strict_warnings(cube96_bench)

set(TEST_SOURCES
  tests/test_roundtrip.cpp
  tests/test_vectors.cpp
  tests/test_permutation.cpp
  tests/test_kdf.cpp
  tests/test_kdf_deterministic.cpp
  tests/test_avalanche.cpp)

include(CTest)

foreach(test_src IN LISTS TEST_SOURCES)
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_link_libraries(${test_name} PRIVATE cube96)
  cube96_enable_strict_warnings(${test_name})
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
