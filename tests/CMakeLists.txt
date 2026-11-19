file(GLOB asset_sources CONFIGURE_DEPENDS Assets/*.cpp)
add_executable(ue5parse_asset_tests ${asset_sources})
target_include_directories(ue5parse_asset_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ue5parse_asset_tests PRIVATE CppUE5Parse::Core CppUE5Parse::AnimGraphRuntime CppUE5Parse::NavigationSystem CppUE5Parse::GameplayAbilities CppUE5Parse::ACLPlugin CppUE5Parse::Json)
ue5parse_set_library_options(ue5parse_asset_tests)

set(paks ${CMAKE_CURRENT_SOURCE_DIR}/Paks)
set(pak_folders ${paks}/Windows - ${paks}/Windows-Vulkan vulkan/)
file(GLOB pak_inputs CONFIGURE_DEPENDS ${paks}/*/*.pak ${paks}/*/*.json)
set(asset_test_file ${CMAKE_CURRENT_BINARY_DIR}/asset_tests.cmake)
add_custom_command(
    OUTPUT ${asset_test_file}
    COMMAND ue5parse_asset_tests --list ${asset_test_file} $<TARGET_FILE:ue5parse_asset_tests> ${pak_folders}
    DEPENDS ue5parse_asset_tests ${pak_inputs}
    COMMENT "Listing the asset tests"
    VERBATIM)
add_custom_target(ue5parse_asset_test_list ALL DEPENDS ${asset_test_file})
set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${asset_test_file})
