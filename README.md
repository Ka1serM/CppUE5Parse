# CppUE5Parse

A C++23 reader and runtime for cooked Unreal Engine 5 games: archives, packages, assets, UObjects, the Blueprint VM
and the animation graph.

## Dependencies

- CMake 3.29+ and Ninja
- Python 3.10+ and libclang, for the UnrealHeaderTool step
- Clang 22+ with libc++ and LLD. The presets choose the compiler.
- Linux: Clang, LLD and the static libc++ and libc++abi
- Windows: [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) with its `bin` folder in PATH, and LLVM (for libclang)

Fedora:

```sh
sudo dnf install clang lld llvm libcxx-devel libcxx-static libcxxabi-static cmake ninja-build python3 clang-libs
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

## Usage

Mount the archives of the game's Paks folder, then load from a `FileProvider`:

```cpp
#include "ue5parse/crypto/Decryptor.h"
#include "ue5parse/io/FileSource.h"
#include "ue5parse/package/Package.h"
#include "ue5parse/provider/FileProvider.h"

using namespace ue5parse;

MountOptions options;
Result<std::vector<std::unique_ptr<FileSource>>> archives = openLocalFiles("ShooterGame/Content/Paks");
Result<AesKey> key = parseAesKey("0x<64 hex digits>");
Result<std::unique_ptr<FileSource>> mappings = openLocalFile("mappings.usmap");
if (!archives || !key || !mappings) return 1;

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

// A package and its exports, without loading them.
Result<const Package*> package = provider.loadPackage("/Game/Characters/Clay/Clay_PrimaryAsset");
if (package)
    for (std::size_t i = 0; i < (*package)->getExportCount(); i++) std::println("{}", (*package)->getExportPathName(i));

// The bytes of any file in the archives.
Result<std::vector<std::byte>> registry = provider.readFile("ShooterGame/AssetRegistry.bin");
```

To get objects of UE's own classes, start the runtime on the provider and load by object path:

```cpp
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Engine/StaticMesh.h"
#include "ue5parse/runtime/Engine/Engine/Texture2D.h"

namespace rt = ue5parse::runtime;

rt::FRuntime runtime(provider);

rt::UStaticMesh* mesh = rt::LoadObject<rt::UStaticMesh>(nullptr, rt::FString("/Game/Environment/Asset/Prop/Crate.Crate"));
rt::UTexture2D* texture = rt::LoadObject<rt::UTexture2D>(nullptr, rt::FString("/Game/UI/Icons/Logo.Logo"));
rt::UObject* any = rt::StaticLoadObject(nullptr, nullptr, rt::FString("/Game/Maps/Ascent/Ascent.Ascent"));
```

`LoadObject` returns null when the object does not load; `rt::SetLoadProblemHandler` reports why. The provider must
outlive every package that it gives out and the runtime. The objects are destroyed with the runtime. `tools/ls` and `tools/load` show more.
