# Compressors for the test writer only: the library itself decompresses.
add_library(ue5parse_test_deflate STATIC
    ${PROJECT_SOURCE_DIR}/external/libdeflate/lib/deflate_compress.c
    ${PROJECT_SOURCE_DIR}/external/libdeflate/lib/zlib_compress.c)
target_link_libraries(ue5parse_test_deflate PUBLIC ue5parse_deflate)
target_compile_options(ue5parse_test_deflate PRIVATE -w)

add_executable(ue5parse_tests Main.cpp CoreTests.cpp VfsTests.cpp PakTests.cpp)
target_link_libraries(ue5parse_tests PRIVATE CppUE5Parse::Core ue5parse_test_deflate ue5parse_aes ue5parse_lz4)
target_compile_options(ue5parse_tests PRIVATE -Wall -Wextra)
add_test(NAME ue5parse_tests COMMAND ue5parse_tests)

# A game module outside the library, as Verylia's: a VALORANT native class that registers itself, checked on the
# game's data (needs the game, so it is not part of ue5parse_tests): ue5parse_shootergame_check <Paks> <file.usmap>.
add_library(ue5parse_test_shootergame STATIC)
target_include_directories(ue5parse_test_shootergame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ue5parse_test_shootergame PUBLIC CppUE5Parse::CoreUObject)
ue5parse_add_reflection(ue5parse_test_shootergame MODULE ShooterGame PACKAGE /Script/ShooterGame
    INCLUDE_ROOT ${CMAKE_CURRENT_SOURCE_DIR} HEADERS ShooterGame/AresWallPenetration.h)
ue5parse_add_module_alias(ue5parse_test_shootergame ue5parse::test::ShooterGame)

add_executable(ue5parse_shootergame_check ShooterGame/ShooterGameCheck.cpp)
target_link_libraries(ue5parse_shootergame_check PRIVATE ue5parse::test::ShooterGame)
if(NOT MSVC)
    target_compile_options(ue5parse_shootergame_check PRIVATE -Wno-invalid-offsetof)
endif()
