set(SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB livaut_source ${SRC}/*.c ${SRC}/**/*.c)

idf_component_register(SRCS ${livaut_source} INCLUDE_DIRS "." REQUIRES driver)

add_compile_definitions(LIVAUT_DEBUG _POSIX_C_SOURCE=200809L)
