#! UE builds its modules without exceptions and RTTI, and the reflection takes offsetof on the native classes' members.
function(ue5parse_set_library_options Target)
    target_compile_options(${Target} PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-invalid-offsetof -fno-exceptions -fno-rtti)
endfunction()

file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS CppUE5Parse/*.cpp)
add_library(CppUE5Parse_Core STATIC ${core_sources})
add_library(CppUE5Parse::Core ALIAS CppUE5Parse_Core)
target_include_directories(CppUE5Parse_Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CppUE5Parse_Core
    PUBLIC TBB::tbb
    PRIVATE ue5parse_ooz ue5parse_aes ue5parse_cityhash ue5parse_deflate ue5parse_zstd ue5parse_lz4 ue5parse_brotli
            ue5parse_unordered_dense)
target_compile_definitions(CppUE5Parse_Core PRIVATE TBB_USE_EXCEPTIONS=0)
#! -ffp-contract=off keeps the float results bit-identical to CUE4Parse for the parity runs.
target_compile_options(CppUE5Parse_Core PRIVATE -ffp-contract=off)
ue5parse_set_library_options(CppUE5Parse_Core)
target_precompile_headers(CppUE5Parse_Core PRIVATE <algorithm> <cstdint> <cstring> <memory> <span> <string_view> <vector>
    <CppUE5Parse/Core/ByteReader.h> <CppUE5Parse/Core/FName.h> <CppUE5Parse/Core/Result.h>)

#! Programs link a module whole-archive (ue5parse_add_module_alias): nothing names the types that each .gen.cpp registers.
function(ue5parse_add_module Name)
    cmake_parse_arguments(PARSE_ARGV 1 Module "REFLECTED" "" "DIRS;DEPENDS")
    set(Sources)
    foreach(Dir IN LISTS Module_DIRS)
        file(GLOB_RECURSE DirSources CONFIGURE_DEPENDS ${Dir}/*.cpp)
        list(APPEND Sources ${DirSources})
    endforeach()
    set(Target CppUE5Parse_${Name})
    add_library(${Target} STATIC ${Sources})
    target_link_libraries(${Target} PUBLIC ${Module_DEPENDS})
    if(Module_REFLECTED)
        foreach(Dir IN LISTS Module_DIRS)
            cmake_path(GET Dir FILENAME Folder)
            ue5parse_add_reflection(${Target} MODULE ${Folder} PACKAGE /Script/${Folder} INCLUDE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}
                HEADER_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/${Dir})
        endforeach()
    endif()
    ue5parse_set_library_options(${Target})
    target_precompile_headers(${Target} PRIVATE
        <Engine/Source/Runtime/Core/Containers/Array.h> <Engine/Source/Runtime/Core/Containers/Map.h>
        <Engine/Source/Runtime/Core/Containers/UnrealString.h> <Engine/Source/Runtime/Core/Math/UnrealMath.h>
        <Engine/Source/Runtime/CoreUObject/UObject/Class.h> <Engine/Source/Runtime/CoreUObject/UObject/Object.h>
        <Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h>)
    ue5parse_add_module_alias(${Target} CppUE5Parse::${Name})
endfunction()

set(runtime Engine/Source/Runtime)
set(plugins Engine/Plugins)
ue5parse_add_module(CoreUObject DIRS ${runtime}/Core ${runtime}/CoreUObject DEPENDS CppUE5Parse_Core PRIVATE ue5parse_unordered_dense)
ue5parse_add_module(RenderCore DIRS ${runtime}/RenderCore ${runtime}/D3D12RHI DEPENDS CppUE5Parse::CoreUObject)
ue5parse_add_module(AnimationCore REFLECTED DIRS ${runtime}/AnimationCore DEPENDS CppUE5Parse::CoreUObject)
ue5parse_add_module(Engine REFLECTED DIRS ${runtime}/Engine ${runtime}/GameplayTags DEPENDS CppUE5Parse::AnimationCore CppUE5Parse::RenderCore)
ue5parse_add_module(GameplayTasks REFLECTED DIRS ${runtime}/GameplayTasks DEPENDS CppUE5Parse::Engine)
ue5parse_add_module(GameplayAbilities REFLECTED DIRS ${plugins}/Runtime/GameplayAbilities/Source/GameplayAbilities
    DEPENDS CppUE5Parse::Engine CppUE5Parse::GameplayTasks)
ue5parse_add_module(AkAudio REFLECTED DIRS ${plugins}/Wwise/Source/AkAudio DEPENDS CppUE5Parse::Engine)
ue5parse_add_module(AnimGraphRuntime REFLECTED DIRS ${runtime}/AnimGraphRuntime DEPENDS CppUE5Parse::Engine)
ue5parse_add_module(ACLPlugin REFLECTED DIRS ${plugins}/Animation/ACLPlugin/Source/ACLPlugin DEPENDS CppUE5Parse::Engine PRIVATE ue5parse_acl)
ue5parse_add_module(KawaiiPhysics REFLECTED DIRS ${plugins}/KawaiiPhysics/Source/KawaiiPhysics DEPENDS CppUE5Parse::AnimGraphRuntime)
if(UE5PARSE_BUILD_CONVERSION)
    ue5parse_add_module(Conversion DIRS Conversion DEPENDS CppUE5Parse::Engine
        PRIVATE dxil-spirv-c-static SPIRV-Tools-opt SPIRV-Headers::SPIRV-Headers ue5parse_vgmstream)
endif()
