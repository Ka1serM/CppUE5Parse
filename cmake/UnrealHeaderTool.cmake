find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(UE5PARSE_LIBCLANG "" CACHE FILEPATH "The libclang library UnrealHeaderTool parses with (empty: found on the system)")

function(ue5parse_add_reflection Target)
    #! find_package() in CppUE5Parse's directory does not set the variable in a game module's sibling directory.
    if(NOT Python3_EXECUTABLE)
        find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    endif()
    cmake_parse_arguments(PARSE_ARGV 1 UHT "" "MODULE;PACKAGE;INCLUDE_ROOT" "HEADERS;HEADER_DIRS")
    foreach(Dir IN LISTS UHT_HEADER_DIRS)
        file(GLOB_RECURSE Candidates CONFIGURE_DEPENDS ${Dir}/*.h)
        foreach(Candidate IN LISTS Candidates)
            file(STRINGS ${Candidate} GeneratedInclude REGEX "#include \"[A-Za-z0-9_]+\\.generated\\.h\"")
            if(GeneratedInclude)
                list(APPEND UHT_HEADERS ${Candidate})
            endif()
        endforeach()
    endforeach()
    set(Script ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/UnrealHeaderTool/UnrealHeaderTool.py)
    set(OutputDir ${CMAKE_CURRENT_BINARY_DIR}/Generated/${UHT_MODULE})
    set(Headers)
    set(Outputs)
    set(Sources)
    foreach(Header IN LISTS UHT_HEADERS)
        cmake_path(ABSOLUTE_PATH Header BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE Header)
        cmake_path(GET Header STEM Stem)
        list(APPEND Headers ${Header})
        list(APPEND Outputs ${OutputDir}/${Stem}.generated.h ${OutputDir}/${Stem}.gen.cpp)
        list(APPEND Sources ${OutputDir}/${Stem}.gen.cpp)
    endforeach()
    set(Flags ${OutputDir}/UnrealHeaderTool-$<CONFIG>.rsp)
    set(Includes "$<TARGET_PROPERTY:${Target},INCLUDE_DIRECTORIES>")
    set(Definitions "$<TARGET_PROPERTY:${Target},COMPILE_DEFINITIONS>")
    file(GENERATE OUTPUT ${Flags} CONTENT "$<$<BOOL:${Includes}>:-I$<JOIN:${Includes},\n-I>>\n$<$<BOOL:${Definitions}>:-D$<JOIN:${Definitions},\n-D>>\n")
    set(Libclang)
    if(UE5PARSE_LIBCLANG)
        set(Libclang --libclang ${UE5PARSE_LIBCLANG})
    endif()
    add_custom_command(
        OUTPUT ${Outputs}
        COMMAND ${Python3_EXECUTABLE} ${Script} --module ${UHT_MODULE} --package ${UHT_PACKAGE}
                --include-root ${UHT_INCLUDE_ROOT} --output ${OutputDir} ${Libclang} @${Flags} ${Headers}
        DEPENDS ${Script} ${Headers} ${Flags}
        COMMENT "UnrealHeaderTool ${UHT_MODULE}"
        VERBATIM)
    target_sources(${Target} PRIVATE ${Sources})
    #! The generated sources build without RTTI, so they also skip the target's precompiled header.
    set_source_files_properties(${Sources} TARGET_DIRECTORY ${Target} PROPERTIES COMPILE_OPTIONS "-fno-rtti;-Wno-invalid-offsetof"
        SKIP_PRECOMPILE_HEADERS ON)
    target_include_directories(${Target} PUBLIC $<BUILD_INTERFACE:${OutputDir}>)
endfunction()

#! Whole-archive: the registration in every .gen.cpp must link even when no code names its class.
function(ue5parse_add_module_alias Target Alias)
    add_library(${Target}_WholeArchive INTERFACE)
    target_link_libraries(${Target}_WholeArchive INTERFACE $<LINK_LIBRARY:WHOLE_ARCHIVE,${Target}>)
    add_library(${Alias} ALIAS ${Target}_WholeArchive)
endfunction()
