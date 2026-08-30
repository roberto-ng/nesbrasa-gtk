cmake_minimum_required(VERSION 3.28)

project(nesbrasa VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(core)
option(BUILD_WEB "Build the Emscripten web frontend" OFF)
if(BUILD_WEB)
    add_subdirectory(frontends/web)
else()
    add_subdirectory(frontends/gtk)
endif()

