cmake_minimum_required(VERSION 3.10)
project(macho_parser 
        VERSION 1.0
        LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(quickmacho quickmacho.c quickmacho.h)

add_executable(macho_helper_cli main.c)
target_link_libraries(macho_helper_cli PRIVATE quickmacho)

find_library(FOUNDATION_LIBRARY Foundation)
target_link_libraries(quickmacho PRIVATE "-framework Foundation")
