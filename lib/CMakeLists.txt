add_library(libmachore libmachore.c libmachore.h)
find_library(FOUNDATION_LIBRARY Foundation)
target_link_libraries(libmachore PRIVATE "-framework Foundation")

