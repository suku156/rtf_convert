cmake_minimum_required(VERSION 3.16)

project(rtfconvert LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_CLI "Build CLI target" ON)
option(BUILD_GUI "Build GUI target" ON)

add_subdirectory(src)

if(BUILD_CLI)
    add_subdirectory(src/Cli_Module)
endif()

if(BUILD_GUI)
    add_subdirectory(src/Qt_gui_Module)
endif()