
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/cosas/src/*.cpp")
add_library(cosas_lib ${SOURCE_LIST})
if ($ENV{BUILD} STREQUAL "pico")
   target_link_libraries(cosas_lib PRIVATE RP2040Atomic)
endif()
target_include_directories(cosas_lib PUBLIC ../include)
