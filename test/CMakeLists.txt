
# i had to remove the check on 64bits from
# /usr/lib/x86_64-linux-gnu/CppUTest/cmake/CppUTestConfigVersion.cmake
# to avoid an error here (google ai asserts cpputest is cross-platform)
find_package(CppUTest 4.0 REQUIRED PATHS /usr/lib/x86_64-linux-gnu/CppUTest/cmake)

add_executable(tests control.cpp)
set_target_properties(tests PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_link_libraries(tests
                      CppUTest
                      CppUTestExt)

target_include_directories(tests PRIVATE
                           ${CMAKE_SOURCE_DIR}/include)

add_custom_target(run_tests COMMAND tests) # Define a custom target to run tests
add_dependencies(run_tests tests)
