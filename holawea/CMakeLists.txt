
add_executable(holawea hello-world.cpp)
target_compile_features(holawea PRIVATE cxx_std_20)
target_link_libraries(holawea PRIVATE pico_stdlib cosas_lib)
pico_add_extra_outputs(holawea)   # generate uf2
pico_enable_stdio_usb(holawea 1)  # allow serial over usb

