
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/cosas/src/*.cpp")
add_library(cosas_lib ${SOURCE_LIST}
        ../include/cosas/buffer.h
        buffer.cpp)
target_include_directories(cosas_lib PUBLIC ../include)
