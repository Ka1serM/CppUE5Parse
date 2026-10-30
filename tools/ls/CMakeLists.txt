# ue5ls: mounts a Paks folder and lists, finds or reads its files. A manual check on real game data.
add_executable(ue5ls main.cpp)
target_link_libraries(ue5ls PRIVATE CppUE5Parse::Core)
