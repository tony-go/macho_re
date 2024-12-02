cmake_minimum_required(VERSION 3.10)
project(macho_parser 
        VERSION 1.0
        LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(libmachore libmachore.c libmachore.h)
find_library(FOUNDATION_LIBRARY Foundation)
target_link_libraries(libmachore PRIVATE "-framework Foundation")

add_executable(machore cli.c)
target_link_libraries(machore PRIVATE libmachore)

