# ue5inspect: checks each layer of the library on real game data (mappings, packages, objects).
add_executable(ue5inspect main.cpp)
target_link_libraries(ue5inspect PRIVATE CppUE5Parse::Core)
