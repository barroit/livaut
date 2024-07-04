cmake_minimum_required(VERSION 3.16)

# must set before ‘include($ENV{IDF_PATH}/tools/cmake/project.cmake)’
set(EXTRA_COMPONENT_DIRS src)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(livaut VERSION 0.1.0 LANGUAGES C)

# get c verison, for esp-idf 5.2, this value is gnu17
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(INPUT_FILE ${CMAKE_SOURCE_DIR}/frames)
# set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/frames.h)
# add_custom_command(OUTPUT ${OUTPUT_FILE}
# 		   COMMAND ${CMAKE_SOURCE_DIR}/genframe.sh
# 		   DEPENDS ${INPUT_FILE} ${CMAKE_SOURCE_DIR}/genframe.sh
# )
# add_custom_target(genframe DEPENDS ${OUTPUT_FILE})
# add_dependencies(${PROJECT_NAME} genframe)
