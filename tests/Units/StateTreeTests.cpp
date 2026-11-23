#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/StateTreeInstanceData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Test.h"

namespace {

namespace rt = ue5parse::runtime;
using ue5parse::EGame;

ue5parse::MountedGame emptyGame() {
    ue5parse::MountOptions options;
    options.game = EGame::GAME_UE5_4;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

std::vector<std::byte> emptyInstanceStorage() {
    std::vector<std::byte> bytes(5);
    const std::int32_t Count = 0;
    std::memcpy(bytes.data() + 1, &Count, sizeof(Count));
    return bytes;
}

}

TEST(stateTreeInstanceDataReadsTheNativeStorageFromUE54) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const std::vector<std::byte> bytes = emptyInstanceStorage();
    rt::FMemoryReaderView Ar(bytes);
    Ar.SetGame(EGame::GAME_UE5_4);
    Ar.SetUnversioned(true);
    Ar.SetUseRawPropertySerialization(true);
    rt::FStateTreeInstanceData Data;
    CHECK(Data.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    const rt::FStateTreeInstanceStorage* Storage = Data.InstanceStorage.GetPtr();
    REQUIRE(Storage != nullptr);
    CHECK(Storage->InstanceStructs.Num() == 0);
}

TEST(stateTreeInstanceDataUsesTaggedPropertiesBeforeCustomSerialization) {
    ue5parse::FileProvider provider(emptyGame());
    rt::FRuntime runtime(provider);
    const std::vector<std::byte> bytes = emptyInstanceStorage();
    rt::FMemoryReaderView Unversioned(bytes);
    Unversioned.SetGame(EGame::GAME_UE5_3);
    Unversioned.SetUnversioned(true);
    rt::FStateTreeInstanceData Data;
    CHECK(!Data.Serialize(Unversioned));
    CHECK(Unversioned.Tell() == 0);
    rt::FMemoryReaderView Versioned(bytes);
    Versioned.SetGame(EGame::GAME_UE5_4);
    const ue5parse::FCustomVersion Before{rt::FStateTreeInstanceStorageCustomVersion::GUID, rt::FStateTreeInstanceStorageCustomVersion::BeforeCustomVersionWasAdded};
    Versioned.SetCustomVersions({&Before, 1});
    CHECK(!Data.Serialize(Versioned));
    CHECK(Versioned.Tell() == 0);
    CHECK(!Data.InstanceStorage.IsValid());
}
