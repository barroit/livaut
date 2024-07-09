set(SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB livaut_source ${SRC}/*.c ${SRC}/action/*.c)

idf_component_register(SRCS ${livaut_source}
		       INCLUDE_DIRS "." ${CMAKE_CURRENT_BINARY_DIR}
		       REQUIRES driver nvs_flash esp_timer esp_wifi esp_pm)

add_compile_definitions(LIVAUT_DEBUG _POSIX_C_SOURCE=200809L)

include(../schedule.cmake)
