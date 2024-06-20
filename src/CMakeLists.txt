set(SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB livaut_source ${SRC}/*.c ${SRC}/**/*.c)

idf_component_register(SRCS ${livaut_source} INCLUDE_DIRS "." REQUIRES driver)
