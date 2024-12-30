add_library(libmachore libmachore.c libmachore.h cs_blobs_shim.h)
find_library(FOUNDATION_LIBRARY Foundation)
target_link_libraries(libmachore PRIVATE "-framework Foundation")

