cmake_minimum_required(VERSION 3.16)

# must set before ‘include($ENV{IDF_PATH}/tools/cmake/project.cmake)’
set(EXTRA_COMPONENT_DIRS src)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(livaut VERSION 0.1.0 LANGUAGES C)
