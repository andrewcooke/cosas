
add_executable(knob knob.cpp)
target_compile_features(knob PRIVATE cxx_std_20)
target_link_libraries(knob PRIVATE weas_lib cosas_lib pico_unique_id pico_stdlib hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi)
pico_add_extra_outputs(knob)   # generate uf2

