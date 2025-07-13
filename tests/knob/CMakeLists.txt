
add_executable(knob knob.cpp)
target_compile_features(knob PRIVATE cxx_std_20)
target_link_libraries(knob PRIVATE pico_stdlib cosas_lib weas_lib)
pico_add_extra_outputs(knob)   # generate uf2

