add_library(ue5parse_test_deflate STATIC
    ${PROJECT_SOURCE_DIR}/external/libdeflate/lib/deflate_compress.c
    ${PROJECT_SOURCE_DIR}/external/libdeflate/lib/zlib_compress.c)
target_link_libraries(ue5parse_test_deflate PUBLIC ue5parse_deflate)
target_compile_options(ue5parse_test_deflate PRIVATE -w)

file(GLOB test_sources CONFIGURE_DEPENDS *.cpp)
if(NOT UE5PARSE_BUILD_CONVERSION)
    list(FILTER test_sources EXCLUDE REGEX "/ConversionTests\\.cpp$")
endif()
add_executable(ue5parse_tests ${test_sources})
target_link_libraries(ue5parse_tests PRIVATE CppUE5Parse::Core ue5parse_test_deflate ue5parse_aes ue5parse_lz4
    "$<$<BOOL:${UE5PARSE_BUILD_CONVERSION}>:CppUE5Parse::Conversion>")
ue5parse_set_library_options(ue5parse_tests)
add_test(NAME ue5parse_tests COMMAND ue5parse_tests)
add_test(NAME comments COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check_comments.py)
