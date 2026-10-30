# The UnrealHeaderTool step of UE's build: tools/UnrealHeaderTool reads a module's reflected headers with libclang and
# writes Foo.generated.h and Foo.gen.cpp for each of them. It needs Python 3 and the libclang library (found on the
# system; UE5PARSE_LIBCLANG chooses one).
#
#   ue5parse_add_reflection(<target> MODULE Engine PACKAGE /Script/Engine INCLUDE_ROOT <dir> HEADERS <headers...>)
#
# adds the .gen.cpp files to the target and the directory of the .generated.h files to its include path. Each
# .gen.cpp registers its types when the program starts, so link the target whole-archive (ue5parse_add_module_alias).

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(UE5PARSE_LIBCLANG "" CACHE FILEPATH "The libclang library UnrealHeaderTool parses with (empty: found on the system)")

function(ue5parse_add_reflection Target)
    cmake_parse_arguments(PARSE_ARGV 1 UHT "" "MODULE;PACKAGE;INCLUDE_ROOT" "HEADERS;HEADER_DIRS")
    # HEADER_DIRS: every header below these directories that includes its .generated.h (as UE's UHT finds a module's
    # reflected headers).
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
    # The target's include directories and definitions, so that libclang reads the headers as the compiler does.
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
    # offsetof on UObject classes, as UE's STRUCT_OFFSET: valid on the compilers we support, but not standard layout.
    if(NOT MSVC)
        set_source_files_properties(${Sources} TARGET_DIRECTORY ${Target} PROPERTIES COMPILE_OPTIONS -Wno-invalid-offsetof)
    endif()
    target_include_directories(${Target} PUBLIC $<BUILD_INTERFACE:${OutputDir}>)
    # Reflected classes derive from UObject, which the library builds without RTTI (as UE builds its modules): the
    # module must too, or it needs UObject's type info.
    if(MSVC)
        target_compile_options(${Target} PRIVATE /GR-)
    else()
        target_compile_options(${Target} PRIVATE -fno-rtti)
    endif()
endfunction()

# The name a module is linked by (CppUE5Parse::Engine): the library whole-archive, so that the registration in every
# .gen.cpp is linked even when no code names its class.
function(ue5parse_add_module_alias Target Alias)
    add_library(${Target}_WholeArchive INTERFACE)
    target_link_libraries(${Target}_WholeArchive INTERFACE $<LINK_LIBRARY:WHOLE_ARCHIVE,${Target}>)
    add_library(${Alias} ALIAS ${Target}_WholeArchive)
endfunction()
