
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "*.cpp")
add_executable(dump ${SOURCE_LIST})
target_compile_features(dump PRIVATE cxx_std_20)
target_link_libraries(dump PRIVATE cosas_lib)

add_custom_target(run_dump COMMAND dump > dump.out) # Define a custom target to run tests
add_dependencies(run_dump dump)
