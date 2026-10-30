# Each library compiles every source below its folders. Its precompiled header holds the headers that most of them
# include.

# The settings of every library, tool and test: no exceptions and no RTTI (as UE builds its modules), all warnings.
# offsetof on the native classes' members, as UE's STRUCT_OFFSET.
function(ue5parse_set_library_options Target)
    target_compile_options(${Target} PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-invalid-offsetof -fno-exceptions -fno-rtti)
endfunction()

# CppUE5Parse::Core: files, crypto, compression, the container formats and the packages: every source outside runtime/.
file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS *.cpp)
list(FILTER core_sources EXCLUDE REGEX "/runtime/")
add_library(CppUE5Parse_Core STATIC ${core_sources})
add_library(CppUE5Parse::Core ALIAS CppUE5Parse_Core)
target_include_directories(CppUE5Parse_Core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(CppUE5Parse_Core
    PUBLIC TBB::tbb
    PRIVATE ue5parse_ooz ue5parse_aes ue5parse_cityhash ue5parse_deflate ue5parse_zstd ue5parse_lz4 ue5parse_brotli
            ue5parse_unordered_dense)
target_compile_definitions(CppUE5Parse_Core PRIVATE TBB_USE_EXCEPTIONS=0)
# Every module and program sees the same value, so that the reflected types have one layout.
# -ffp-contract=off keeps float results bit-identical to CUE4Parse for the parity runs.
target_compile_options(CppUE5Parse_Core PRIVATE -ffp-contract=off)
ue5parse_set_library_options(CppUE5Parse_Core)
target_precompile_headers(CppUE5Parse_Core PRIVATE <algorithm> <cstdint> <cstring> <memory> <span> <string_view> <vector>
    <ue5parse/core/ByteReader.h> <ue5parse/core/FName.h> <ue5parse/core/Result.h>)

# A module of UE's runtime: the sources of runtime/<Folder> for each folder, and the reflection of each folder's headers
# (package /Script/<Folder>) when it is REFLECTED. Programs link it as CppUE5Parse::<Name>, whole-archive, so that the
# registration of every type is linked.
function(ue5parse_add_module Name)
    cmake_parse_arguments(PARSE_ARGV 1 Module "REFLECTED" "" "FOLDERS;DEPENDS")
    set(Sources)
    foreach(Folder IN LISTS Module_FOLDERS)
        file(GLOB_RECURSE FolderSources CONFIGURE_DEPENDS runtime/${Folder}/*.cpp)
        list(APPEND Sources ${FolderSources})
    endforeach()
    set(Target CppUE5Parse_${Name})
    add_library(${Target} STATIC ${Sources})
    target_link_libraries(${Target} PUBLIC ${Module_DEPENDS})
    if(Module_REFLECTED)
        foreach(Folder IN LISTS Module_FOLDERS)
            ue5parse_add_reflection(${Target} MODULE ${Folder} PACKAGE /Script/${Folder} INCLUDE_ROOT ${PROJECT_SOURCE_DIR}/include
                HEADER_DIRS ${PROJECT_SOURCE_DIR}/include/ue5parse/runtime/${Folder})
        endforeach()
    endif()
    ue5parse_set_library_options(${Target})
    target_precompile_headers(${Target} PRIVATE
        <ue5parse/runtime/Core/Containers/Array.h> <ue5parse/runtime/Core/Containers/Map.h>
        <ue5parse/runtime/Core/Containers/UnrealString.h> <ue5parse/runtime/Core/Math/UnrealMath.h>
        <ue5parse/runtime/CoreUObject/UObject/Class.h> <ue5parse/runtime/CoreUObject/UObject/Object.h>
        <ue5parse/runtime/CoreUObject/UObject/UnrealType.h>)
    ue5parse_add_module_alias(${Target} CppUE5Parse::${Name})
endfunction()

# UE's object system: reflected classes with property memory, the linker that turns cooked packages into runtime
# objects, and the Blueprint VM.
ue5parse_add_module(CoreUObject FOLDERS Core CoreUObject DEPENDS CppUE5Parse_Core PRIVATE ue5parse_unordered_dense)
# The bone indices and the IK and constraint solvers.
ue5parse_add_module(AnimationCore REFLECTED FOLDERS AnimationCore DEPENDS CppUE5Parse::CoreUObject)
# The engine classes and structs, and the GameplayTags types that they use.
ue5parse_add_module(Engine REFLECTED FOLDERS Engine GameplayTags DEPENDS CppUE5Parse::AnimationCore)
# The gameplay plugins' classes that cooked Blueprints derive from.
ue5parse_add_module(GameplayTasks REFLECTED FOLDERS GameplayTasks DEPENDS CppUE5Parse::Engine)
ue5parse_add_module(GameplayAbilities REFLECTED FOLDERS GameplayAbilities DEPENDS CppUE5Parse::Engine CppUE5Parse::GameplayTasks)
# The Wwise plugin's classes.
ue5parse_add_module(AkAudio REFLECTED FOLDERS AkAudio DEPENDS CppUE5Parse::Engine)
# The animation graph nodes.
ue5parse_add_module(AnimGraphRuntime REFLECTED FOLDERS AnimGraphRuntime DEPENDS CppUE5Parse::Engine)
# The ACL plugin's bone codecs.
ue5parse_add_module(ACLPlugin REFLECTED FOLDERS ACLPlugin DEPENDS CppUE5Parse::Engine PRIVATE ue5parse_acl)
# The KawaiiPhysics plugin's node, which passes its pose through.
ue5parse_add_module(KawaiiPhysics REFLECTED FOLDERS KawaiiPhysics DEPENDS CppUE5Parse::AnimGraphRuntime)
