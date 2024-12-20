add_executable(macho_re_test test_main.cc)

file(COPY ${CMAKE_SOURCE_DIR}/test/fixtures/test.o
     DESTINATION ${CMAKE_BINARY_DIR}/test/fixtures)

target_link_libraries(macho_re_test
  PRIVATE libmachore GTest::gtest GTest::gtest_main)

add_test(NAME macho_re_test COMMAND macho_re_test)
