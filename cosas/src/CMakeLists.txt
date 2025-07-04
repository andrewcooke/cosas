
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/cosas/src/*.cpp")
add_library(cosas_lib ${SOURCE_LIST}
        ../include/cosas/queue.h)
target_include_directories(cosas_lib PUBLIC ../include)
