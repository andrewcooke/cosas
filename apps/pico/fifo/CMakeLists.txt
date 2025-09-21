
add_executable(fifo fifo.cpp)
target_compile_features(fifo PRIVATE cxx_std_20)
target_link_libraries(fifo PRIVATE pico_stdlib cosas_lib weas_lib pico_unique_id pico_stdlib pico_multicore hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi)
pico_add_extra_outputs(fifo)   # generate uf2
pico_enable_stdio_usb(fifo 1)  # allow serial over usb
