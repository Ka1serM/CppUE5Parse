# Each dependency builds only the parts that the library uses, as a static library.

# oneTBB: the parallel algorithms.
if(NOT TARGET TBB::tbb)
    set(TBB_TEST OFF CACHE BOOL "" FORCE)
    set(TBB_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(TBB_STRICT OFF CACHE BOOL "" FORCE)
    set(TBBMALLOC_BUILD OFF CACHE BOOL "" FORCE)
    set(TBB_INSTALL OFF CACHE BOOL "" FORCE)
    # No hwloc: TBBBind and the Thread Composability Manager need it, and a second configure fails without it.
    set(TCM_BUILD OFF CACHE BOOL "" FORCE)
    set(TBB_DISABLE_HWLOC_AUTOMATIC_SEARCH ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(oneTBB EXCLUDE_FROM_ALL SYSTEM)
endif()

# SIMDe: x86 intrinsics on every target. A browser build runs them as WebAssembly SIMD. Headers only.
add_library(ue5parse_simde INTERFACE)
target_include_directories(ue5parse_simde SYSTEM INTERFACE simde)
add_library(simde::simde ALIAS ue5parse_simde)

# ooz: the Oodle decoders. OOZ_BUILD_DLL removes its main() and its DLL loading.
add_library(ue5parse_ooz STATIC ooz/kraken.cpp ooz/bitknit.cpp ooz/lzna.cpp)
target_include_directories(ue5parse_ooz SYSTEM PRIVATE ooz)
target_link_libraries(ue5parse_ooz PRIVATE simde::simde)
target_compile_definitions(ue5parse_ooz PRIVATE OOZ_BUILD_DLL=1)
# ooz reads unaligned integers through casts (fine on x86, ARM64 and wasm); a sanitized build would trap on them.
target_compile_options(ue5parse_ooz PRIVATE -w -fno-sanitize=all)
# ooz takes _rotl from <x86gprintrin.h>, which declares it only in GCC. Clang declares it in <x86intrin.h>.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT EMSCRIPTEN)
    target_compile_options(ue5parse_ooz PRIVATE -include x86intrin.h)
endif()
set_target_properties(ue5parse_ooz PROPERTIES CXX_STANDARD 17 POSITION_INDEPENDENT_CODE ON)

# mbedtls: only AES (with AES-NI on x86).
if(TARGET mbedcrypto)
    add_library(ue5parse_aes INTERFACE)
    target_link_libraries(ue5parse_aes INTERFACE mbedcrypto)
else()
    add_library(ue5parse_aes STATIC
        mbedtls/library/aes.c mbedtls/library/aesni.c mbedtls/library/aesce.c
        mbedtls/library/platform.c mbedtls/library/platform_util.c)
    target_include_directories(ue5parse_aes SYSTEM PUBLIC mbedtls/include PRIVATE mbedtls/library)
    target_compile_options(ue5parse_aes PRIVATE -w)
endif()

# CityHash64: the name and package hashes of UE.
add_library(ue5parse_cityhash STATIC cityhash/src/city.cc)
target_include_directories(ue5parse_cityhash SYSTEM PUBLIC cityhash/src)
target_compile_definitions(ue5parse_cityhash PRIVATE HAVE_BUILTIN_EXPECT=1)
target_compile_options(ue5parse_cityhash PRIVATE -w -include cstdint)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cityhash/config.h "// Generated: cityhash expects an autotools config.h.\n")
# MinGW has no glibc <byteswap.h>, which cityhash includes on every compiler other than MSVC.
if(WIN32)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cityhash/byteswap.h
        "#define bswap_32(x) __builtin_bswap32(x)\n#define bswap_64(x) __builtin_bswap64(x)\n")
endif()
target_include_directories(ue5parse_cityhash PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/cityhash)

# libdeflate: zlib, gzip and raw deflate, decompression only.
add_library(ue5parse_deflate STATIC
    libdeflate/lib/adler32.c libdeflate/lib/crc32.c libdeflate/lib/deflate_decompress.c
    libdeflate/lib/gzip_decompress.c libdeflate/lib/zlib_decompress.c libdeflate/lib/utils.c
    libdeflate/lib/x86/cpu_features.c libdeflate/lib/arm/cpu_features.c)
target_include_directories(ue5parse_deflate SYSTEM PUBLIC libdeflate)
target_compile_options(ue5parse_deflate PRIVATE -w)

# zstd: decompression only. Its x86-64 Huffman loop is assembly; other targets (wasm, Windows) use the C loop.
if(TARGET zstd)
    add_library(ue5parse_zstd INTERFACE)
    target_link_libraries(ue5parse_zstd INTERFACE zstd)
else()
    file(GLOB zstd_sources zstd/lib/common/*.c zstd/lib/decompress/*.c)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64" AND NOT WIN32 AND NOT EMSCRIPTEN)
        enable_language(ASM)
        list(APPEND zstd_sources zstd/lib/decompress/huf_decompress_amd64.S)
        set(zstd_asm ON)
    endif()
    add_library(ue5parse_zstd STATIC ${zstd_sources})
    target_include_directories(ue5parse_zstd SYSTEM PUBLIC zstd/lib)
    target_compile_definitions(ue5parse_zstd PRIVATE ZSTD_MULTITHREAD=0 ZSTD_LEGACY_SUPPORT=0)
    if(NOT zstd_asm)
        target_compile_definitions(ue5parse_zstd PRIVATE ZSTD_DISABLE_ASM=1)
    endif()
    target_compile_options(ue5parse_zstd PRIVATE -w)
endif()

# LZ4: the block decoder.
add_library(ue5parse_lz4 STATIC lz4/lib/lz4.c)
target_include_directories(ue5parse_lz4 SYSTEM PUBLIC lz4/lib)
target_compile_options(ue5parse_lz4 PRIVATE -w)

# Brotli: the decoder, for .usmap files.
file(GLOB brotli_sources brotli/c/common/*.c brotli/c/dec/*.c)
add_library(ue5parse_brotli STATIC ${brotli_sources})
target_include_directories(ue5parse_brotli SYSTEM PUBLIC brotli/c/include)
target_compile_options(ue5parse_brotli PRIVATE -w)

# unordered_dense: one header.
add_library(ue5parse_unordered_dense INTERFACE)
target_include_directories(ue5parse_unordered_dense SYSTEM INTERFACE unordered_dense/include)

# ACL (animation compression) and rtm (its math library, which the Script module uses for transforms): headers only.
add_library(ue5parse_rtm INTERFACE)
target_include_directories(ue5parse_rtm SYSTEM INTERFACE acl/external/rtm/includes)
add_library(ue5parse_acl INTERFACE)
target_include_directories(ue5parse_acl SYSTEM INTERFACE acl/includes)
target_link_libraries(ue5parse_acl INTERFACE ue5parse_rtm)
