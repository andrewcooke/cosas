
add_executable(hola hello-world.cpp)
target_compile_features(hola PRIVATE cxx_std_20)
target_link_libraries(hola PRIVATE pico_stdlib cosas_lib)
pico_add_extra_outputs(hola)   # generate uf2
pico_enable_stdio_usb(hola 1)  # allow serial over usb

