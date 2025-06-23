
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/include/cosas/*.h")
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/src/*.cpp")
add_library(cosas_lib ${SOURCE_LIST} ${HEADER_LIST})
target_include_directories(cosas_lib PUBLIC ../include)
