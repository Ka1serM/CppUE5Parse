# Compressors for the test writer only: the library itself decompresses.
add_library(ue5parse_test_deflate STATIC
    ${PROJECT_SOURCE_DIR}/external/libdeflate/lib/deflate_compress.c
    ${PROJECT_SOURCE_DIR}/external/libdeflate/lib/zlib_compress.c)
target_link_libraries(ue5parse_test_deflate PUBLIC ue5parse_deflate)
target_compile_options(ue5parse_test_deflate PRIVATE -w)

file(GLOB test_sources CONFIGURE_DEPENDS *.cpp)
add_executable(ue5parse_tests ${test_sources})
target_link_libraries(ue5parse_tests PRIVATE CppUE5Parse::Core ue5parse_test_deflate ue5parse_aes ue5parse_lz4)
ue5parse_set_library_options(ue5parse_tests)
add_test(NAME ue5parse_tests COMMAND ue5parse_tests)

