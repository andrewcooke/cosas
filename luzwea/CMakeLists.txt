
add_executable(luzwea luzwea.cpp)
target_compile_features(luzwea PRIVATE cxx_std_20)
target_link_libraries(luzwea PRIVATE pico_stdlib cosas_lib weas_lib)
pico_add_extra_outputs(luzwea)   # generate uf2

