cmake_minimum_required(VERSION 3.16)

# must set before ‘include($ENV{IDF_PATH}/tools/cmake/project.cmake)’
set(EXTRA_COMPONENT_DIRS src)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(livaut VERSION 0.2.1 LANGUAGES C)

# get c verison, for esp-idf 5.2, this value is gnu17
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
