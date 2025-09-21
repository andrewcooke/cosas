
add_executable(dnl dnl.cpp)
target_compile_features(dnl PRIVATE cxx_std_20)
target_link_libraries(dnl PRIVATE pico_stdlib cosas_lib weas_lib pico_unique_id pico_stdlib hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi)
pico_add_extra_outputs(dnl)   # generate uf2

