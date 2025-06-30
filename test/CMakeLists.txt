
find_package(CppUTest REQUIRED)

file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "*.cpp")
add_executable(tests ${SOURCE_LIST})
target_compile_features(tests PRIVATE cxx_std_20)
target_link_libraries(tests PRIVATE cosas_lib CppUTest CppUTestExt)

add_custom_target(run_tests COMMAND tests) # Define a custom target to run tests
add_dependencies(run_tests tests)
