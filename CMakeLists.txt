cmake_minimum_required(VERSION 3.12)
project(cube96 LANGUAGES CXX)

set(CUBE96_LAYOUT "zslice" CACHE STRING "State layout mapping (zslice or rowmajor)")
string(TOLOWER "${CUBE96_LAYOUT}" CUBE96_LAYOUT_LOWER)
set(CUBE96_VALID_LAYOUTS zslice rowmajor)
list(FIND CUBE96_VALID_LAYOUTS "${CUBE96_LAYOUT_LOWER}" CUBE96_LAYOUT_INDEX)
if(CUBE96_LAYOUT_INDEX EQUAL -1)
  message(FATAL_ERROR "Unsupported CUBE96_LAYOUT '${CUBE96_LAYOUT}'. Choose zslice or rowmajor.")
endif()

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

target_include_directories(cube96 PUBLIC include)

target_compile_features(cube96 PUBLIC cxx_std_17)

if(CUBE96_LAYOUT_LOWER STREQUAL "rowmajor")
  target_compile_definitions(cube96 PUBLIC CUBE96_LAYOUT_ROWMAJOR)
else()
  target_compile_definitions(cube96 PUBLIC CUBE96_LAYOUT_ZSLICE)
endif()

add_executable(cube96_cli tools/cube96_cli.cpp)
target_link_libraries(cube96_cli PRIVATE cube96)

add_executable(cube96_bench bench/bench_throughput.cpp)
target_link_libraries(cube96_bench PRIVATE cube96)

add_subdirectory(analysis)

set(TEST_SOURCES
  tests/test_roundtrip.cpp
  tests/test_vectors.cpp
  tests/test_permutation.cpp
  tests/test_kdf.cpp
  tests/test_avalanche.cpp)

include(CTest)

foreach(test_src IN LISTS TEST_SOURCES)
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_link_libraries(${test_name} PRIVATE cube96)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
