
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/weas/src/*.cpp")
add_library(weas_lib ${SOURCE_LIST})
target_link_libraries(weas_lib PRIVATE pico_unique_id pico_stdlib hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi)
target_include_directories(weas_lib PUBLIC ../include)
