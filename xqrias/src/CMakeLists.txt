
file(GLOB SOURCE_LIST CONFIGURE_DEPENDS "${cosas_SOURCE_DIR}/xqrias/src/*.cpp")
idf_component_register(SRCS ${SOURCE_LIST}
                       INCLUDE_DIRS ../include
                       REQUIRES)
