# CppUE5Parse

A C++23 reader and runtime for cooked Unreal Engine 5 games: archives, packages, assets, UObjects, the Blueprint VM
and the animation graph.

## Dependencies

- CMake 3.29+ and Ninja
- Python 3.10+ and libclang, for the UnrealHeaderTool step
- Clang 22+ with libc++, LLVM's runtime (compiler-rt and libunwind) and LLD. The presets choose the compiler. No part
  of GCC is used.
- Linux: Clang, LLD, compiler-rt, and the static libc++, libc++abi and LLVM libunwind
- Windows: [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) with its `bin` folder in PATH, and LLVM (for libclang)

Fedora:

```sh
sudo dnf install clang lld llvm libcxx-devel libcxx-static libcxxabi-static compiler-rt llvm-libunwind-devel \
    llvm-libunwind-static cmake ninja-build python3 clang-libs
```

Windows (PowerShell):

```powershell
winget install -e --id Git.Git
winget install -e --id Kitware.CMake
winget install -e --id Ninja-build.Ninja
winget install -e --id MartinStorsjo.LLVM-MinGW.UCRT
winget install -e --id LLVM.LLVM
winget install -e --id Python.Python.3.12
```

Git submodules in `external/`, built from source: oneTBB, ACL (with RTM), SIMDe, ooz (GPL-3.0), Mbed TLS (AES only),
CityHash, libdeflate, zstd, LZ4, Brotli and unordered_dense. `external/clang-python` holds the libclang Python
bindings.

## Build

```sh
git submodule update --init --recursive
cmake --workflow --preset linux-release
```

The presets are `<platform>-<type>`: the platform `linux` or `windows`, the type `debug`, `relwithdebinfo` or
`release`. Each one configures, builds and tests in `build-<preset>`.

## Layout

`src/` is the include root. Each header has its source next to it.

| Folder | Contents | CMake target |
| --- | --- | --- |
| `src/CppUE5Parse/` | Files, crypto, compression, IoStore and pak containers, packages, mappings, the asset registry | `CppUE5Parse::Core` |
| `src/Engine/Source/Runtime/<Module>/` | UE's runtime modules: Core, CoreUObject, RenderCore, AnimationCore, Engine, GameplayTags, GameplayTasks, AnimGraphRuntime | `CppUE5Parse::<Module>` |
| `src/Engine/Plugins/.../Source/<Module>/` | Plugin modules: GameplayAbilities, ACLPlugin, AkAudio (Wwise), KawaiiPhysics | `CppUE5Parse::<Module>` |
| `tools/` | `UnrealHeaderTool`, the comment check, and the command line tools `ue5ls`, `ue5load`, `ue5inspect`, `ue5script` | |
| `tests/` | The unit tests and the comment check | |

The runtime folders have the paths of UE's source tree, so that a file of UE's source has its port at the same path.
Link a runtime module by its `CppUE5Parse::<Module>` name: that target links the library whole-archive, so that the
registration of every reflected type is linked.

A game module adds the reflection of its own headers with UnrealHeaderTool:

```cmake
ue5parse_add_reflection(<target> MODULE ShooterGame PACKAGE /Script/ShooterGame INCLUDE_ROOT <dir> HEADER_DIRS <dirs...>)
```

This adds a `.gen.cpp` for each header below `HEADER_DIRS` that includes its `.generated.h`, and puts the directory of
the `.generated.h` files on the target's include path. `UE5PARSE_LIBCLANG` selects the libclang library.

## Comments

Code is the documentation: use names that explain it. A comment is permitted only for behavior that the code cannot
show, such as an OS quirk, a thread rule or a file format fact. It is one line, and it has a mark: `//!` in C++ and
`#!` in CMake and Python. `tools/check_comments.py` rejects all other comments. The tests run it.

## Usage

Mount the archives of the game's Paks folder, then load from a `FileProvider`:

```cpp
#include "CppUE5Parse/Crypto/Decryptor.h"
#include "CppUE5Parse/IO/FileSource.h"
#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"

using namespace ue5parse;

MountOptions options;
Result<std::vector<std::unique_ptr<FileSource>>> archives = openLocalFiles("ShooterGame/Content/Paks");
Result<AesKey> key = parseAesKey("0x<64 hex digits>");
Result<std::unique_ptr<FileSource>> mappings = openLocalFile("mappings.usmap");
if (!archives || !key || !mappings)
    return 1;

KeyRing keys;
keys.add(FGuid{}, *key);
options.archives = std::move(*archives);
options.decryptor = std::make_unique<AesDecryptor>(keys);
options.mappings = std::move(*mappings);
options.game = EGame::GAME_Valorant;

Result<MountedGame> game = mount(std::move(options));
if (!game) {
    std::println("{} at {}", getErrorName(game.error().code), game.error().where);
    return 1;
}
FileProvider provider(std::move(*game));

Result<const Package*> package = provider.loadPackage("/Game/Characters/Clay/Clay_PrimaryAsset");
if (package)
    for (std::size_t i = 0; i < (*package)->getExportCount(); i++)
        std::println("{}", (*package)->getExportPathName(i));

Result<std::vector<std::byte>> registry = provider.readFile("ShooterGame/AssetRegistry.bin");
```

`loadPackage` reads a package and its exports, but does not load the exports. `readFile` gives the bytes of any file in
the archives.

To get objects of UE's own classes, start the runtime on the provider and load by object path:

```cpp
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"

namespace rt = ue5parse::runtime;

rt::FRuntime runtime(provider);

rt::UStaticMesh* mesh = rt::LoadObject<rt::UStaticMesh>(nullptr, rt::FString("/Game/Environment/Asset/Prop/Crate.Crate"));
rt::UTexture2D* texture = rt::LoadObject<rt::UTexture2D>(nullptr, rt::FString("/Game/UI/Icons/Logo.Logo"));
rt::UObject* any = rt::StaticLoadObject(nullptr, nullptr, rt::FString("/Game/Maps/Ascent/Ascent.Ascent"));
```

`LoadObject` returns null when the object does not load; `rt::SetLoadProblemHandler` reports why. The provider must
outlive every package that it gives out and the runtime. The objects are destroyed with the runtime. `tools/ls` and
`tools/load` show more.
