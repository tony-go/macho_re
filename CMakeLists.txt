cmake_minimum_required(VERSION 3.10)
project(macho_parser 
        VERSION 1.0
        LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Set C and C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11) # Needed for googletest 
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Needed for googletest

# Add libmachore library
add_subdirectory(lib)

# Add cli executable
add_executable(macho_re main.c)
target_link_libraries(macho_re PRIVATE libmachore)

# Add tests
add_subdirectory(test)
