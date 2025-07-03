
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/weas/src/*.cpp")
add_library(weas_lib ${SOURCE_LIST})
target_include_directories(weas_lib PUBLIC ../include)
