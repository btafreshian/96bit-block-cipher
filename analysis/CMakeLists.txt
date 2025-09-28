add_executable(cube96_ddt_lat ddt_lat.cpp)
target_link_libraries(cube96_ddt_lat PRIVATE cube96)

add_executable(cube96_diff_trails diff_trail.cpp)
target_link_libraries(cube96_diff_trails PRIVATE cube96)

add_executable(cube96_linear_bias linear_bias.cpp)
target_link_libraries(cube96_linear_bias PRIVATE cube96)
