
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/xqrias/src/*.cpp")
add_library(xqrias_lib ${SOURCE_LIST})
target_link_libraries(xqrias_lib PRIVATE cosas_lib)
target_include_directories(xqrias_lib PUBLIC ../include)
