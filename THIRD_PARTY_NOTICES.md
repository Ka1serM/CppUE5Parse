# Third-party software

CppUE5Parse builds these projects from `external/`. Each one keeps its own license, in its folder.

| Project | Use | License |
| --- | --- | --- |
| [oneTBB](https://github.com/uxlfoundation/oneTBB) | parallel algorithms | Apache-2.0 |
| [Mbed TLS](https://github.com/Mbed-TLS/mbedtls) | AES | Apache-2.0 or GPL-2.0-or-later |
| [zstd](https://github.com/facebook/zstd) | Zstandard | BSD-3-Clause or GPL-2.0 |
| [libdeflate](https://github.com/ebiggers/libdeflate) | zlib and deflate | MIT |
| [LZ4](https://github.com/lz4/lz4) | LZ4 | BSD-2-Clause (`lib/`) |
| [Brotli](https://github.com/google/brotli) | Brotli | MIT |
| [ooz](https://github.com/zao/ooz) | Oodle Kraken, Mermaid, Selkie, Leviathan, BitKnit and LZNA | **GPL-3.0** |
| [CityHash](https://github.com/google/cityhash) | name hashes | MIT |
| [unordered_dense](https://github.com/martinus/unordered_dense) | hash maps | MIT |
| [ACL](https://github.com/nfrechette/acl) | animation decompression | MIT |
| [dxil-spirv](https://github.com/HansKristian-Work/dxil-spirv) | D3D shaders to SPIR-V (`CppUE5Parse::Conversion`) | MIT |
| [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools) | SPIR-V optimization (`CppUE5Parse::Conversion`) | Apache-2.0 |
| [vgmstream](https://github.com/vgmstream/vgmstream) | audio decoding (`CppUE5Parse::Conversion`) | ISC |
| [Ogg](https://github.com/xiph/ogg) and [Vorbis](https://github.com/xiph/vorbis) | Vorbis audio (`CppUE5Parse::Conversion`) | BSD-3-Clause |
| [SIMDe](https://github.com/simd-everywhere/simde) | portable x86 intrinsics | MIT |
| `external/clang-python` (LLVM's libclang Python bindings) | UnrealHeaderTool, at build time only | Apache-2.0 WITH LLVM-exception |

## ooz and the GPL

ooz is licensed under the GPL-3.0. The CppUE5Parse sources are MIT, but a library or a program that links
`CppUE5Parse::Core` also links ooz. If you distribute such a binary, you must distribute it under the terms of the
GPL-3.0.
