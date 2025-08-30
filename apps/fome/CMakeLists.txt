
add_executable(fome fome.cpp)
target_compile_features(fome PRIVATE cxx_std_20)
target_link_libraries(fome PRIVATE pico_stdlib cosas_lib weas_lib pico_unique_id pico_stdlib pico_multicore hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi RP2040Atomic)
pico_add_extra_outputs(fome)   # generate uf2
pico_enable_stdio_usb(fome 1)  # allow serial over usb
