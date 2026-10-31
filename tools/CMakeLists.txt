# Each folder with a main.cpp is a command line tool, ue5<folder>, which checks the library on real game data.
file(GLOB tool_mains CONFIGURE_DEPENDS */main.cpp)
foreach(tool_main IN LISTS tool_mains)
    cmake_path(GET tool_main PARENT_PATH tool_dir)
    cmake_path(GET tool_dir FILENAME tool_name)
    add_executable(ue5${tool_name} ${tool_main})
    target_link_libraries(ue5${tool_name} PRIVATE CppUE5Parse::AnimGraphRuntime CppUE5Parse::ACLPlugin CppUE5Parse::KawaiiPhysics CppUE5Parse::AkAudio)
    ue5parse_set_library_options(ue5${tool_name})
endforeach()
