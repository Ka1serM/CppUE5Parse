# ue5load: loads objects through the runtime on real game data. A manual check of the engine classes.
add_executable(ue5load main.cpp)
target_link_libraries(ue5load PRIVATE CppUE5Parse::AnimGraphRuntime CppUE5Parse::ACLPlugin)
