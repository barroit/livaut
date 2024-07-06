set(SRC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB livaut_source ${SRC}/*.c ${SRC}/action/*.c)

idf_component_register(SRCS ${livaut_source}
		       INCLUDE_DIRS "." ${CMAKE_CURRENT_BINARY_DIR}
		       REQUIRES driver esp_timer esp_wifi nvs_flash)

add_compile_definitions(LIVAUT_DEBUG _POSIX_C_SOURCE=200809L)

configure_file(../jumper.conf jumper-layout.h)
configure_file(../wifi.conf wifi-cred.h)

include(../schedule.cmake)
