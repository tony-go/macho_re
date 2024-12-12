find_package(GoogleTest REQUIRED)
enable_testing()

add_executable(macho_re_test test_main.cc)

target_link_libraries(macho_re_test
  PRIVATE libmachore GTest::gtest GTest::gtest_main)

add_test(NAME macho_re_test COMMAND macho_re_test)

