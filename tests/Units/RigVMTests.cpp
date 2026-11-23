#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVM.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMRegistry.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Test.h"

namespace {

namespace rt = ue5parse::runtime;
using ue5parse::EGame;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    void string(std::string_view text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        for (char c : text)
            put(c);
        put('\0');
    }

    void object() { put<std::int32_t>(0); }

    std::vector<std::byte> bytes;
};

class RigVMArchive : public rt::FMemoryReaderView {
public:
    using rt::FMemoryReaderView::FMemoryReaderView;
    using rt::FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = rt::MakeName(Text);
        return *this;
    }

    rt::FArchive& operator<<(rt::UObject*& Value) override {
        std::int32_t Index = 0;
        Serialize(&Index, sizeof(Index));
        Value = nullptr;
        return *this;
    }
};

void writeType(Writer& out, std::string_view cppType) {
    out.string(cppType);
    out.object();
}

void writeRegistry(Writer& out) {
    out.put<std::int32_t>(1);
    out.object();
    out.put<std::int32_t>(2);
    writeType(out, "float");
    writeType(out, "TArray<float>");
    out.put<std::int32_t>(1);
    out.object();
    out.put<std::int32_t>(1);
    out.string("Value");
    out.put<std::int32_t>(2);
    out.put<std::int32_t>(2);
    writeType(out, "float");
    writeType(out, "TArray<float>");
    out.put<std::int32_t>(1);
    out.string("FRigUnit_Add::Execute");
    out.object();
    out.object();
    out.put<std::int32_t>(2);
    out.string("A");
    out.string("float");
    out.put<std::int32_t>(0);
    out.string("Result");
    out.string("float");
    out.put<std::int32_t>(1);
}

void writeEmptyMemoryStorage(Writer& out) {
    out.put<std::uint32_t>(0);
    out.put<std::uint8_t>(0);
    out.put<std::int32_t>(0);
}

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

}

TEST(rigVMRegistryReadsTypesFactoriesAndFunctionsInOrder) {
    Writer out;
    writeRegistry(out);
    RigVMArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_8);
    rt::FRigVMRegistry_NoLock Registry;
    Registry.Serialize_NoLock(Ar);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    CHECK(Registry.AllowedClasses.Num() == 1);
    REQUIRE(Registry.Types.Num() == 2);
    CHECK(Registry.Types[1].Type.CPPType == rt::FName("TArray<float>"));
    REQUIRE(Registry.Factories.Num() == 1);
    REQUIRE(Registry.Factories[0].FlattenedArguments.Num() == 1);
    const rt::FRigVMTemplateArgumentInfo& Argument = Registry.Factories[0].FlattenedArguments[0];
    CHECK(Argument.Name == rt::FName("Value"));
    CHECK(Argument.Direction == rt::ERigVMPinDirection::IO);
    REQUIRE(Argument.Types.Num() == 2);
    CHECK(Argument.Types[0].CPPType == rt::FName("float"));
    REQUIRE(Registry.Functions.Num() == 1);
    const rt::FRigVMFunction& Function = Registry.Functions[0];
    CHECK(Function.Name.ToUtf8() == "FRigUnit_Add::Execute");
    REQUIRE(Function.Arguments.Num() == 2);
    CHECK(Function.Arguments[1].Name.ToUtf8() == "Result");
    CHECK(Function.Arguments[1].Type.ToUtf8() == "float");
    CHECK(Function.Arguments[1].Direction == rt::ERigVMFunctionArgumentDirection::Output);
}

TEST(rigVMRegistryStopsAtTruncatedFunctions) {
    Writer out;
    writeRegistry(out);
    out.bytes.resize(out.bytes.size() - 4);
    RigVMArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_8);
    rt::FRigVMRegistry_NoLock Registry;
    Registry.Serialize_NoLock(Ar);
    CHECK(Ar.IsError());
}

TEST(rigVM57ReadsItsStoredLocalizedRegistry) {
    Writer out;
    out.put<std::uint32_t>(0x1234);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    out.string("None");
    out.string("None");
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    writeEmptyMemoryStorage(out);
    writeEmptyMemoryStorage(out);
    writeEmptyMemoryStorage(out);
    out.put<std::uint32_t>(1);
    out.put<std::int64_t>(0);
    writeRegistry(out);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE5_7));
    rt::FRuntime runtime(provider);
    RigVMArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_7);
    rt::URigVM* VM = rt::NewObject<rt::URigVM>(rt::GetTransientPackage());
    VM->Serialize(Ar);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    CHECK(VM->CachedVMHash == 0x1234);
    REQUIRE(VM->LocalizedRegistry.IsSet());
    CHECK(VM->LocalizedRegistry->Functions.Num() == 1);
    CHECK(VM->LocalizedRegistry->Factories.Num() == 1);
}
