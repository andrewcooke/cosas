
add_executable(luz luz.cpp)
target_compile_features(luz PRIVATE cxx_std_20)
target_link_libraries(luz PRIVATE pico_stdlib cosas_lib weas_lib)
pico_add_extra_outputs(luz)   # generate uf2

