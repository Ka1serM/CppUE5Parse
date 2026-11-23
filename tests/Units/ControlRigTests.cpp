#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchy.h"
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

    void append(const Writer& other) { bytes.insert(bytes.end(), other.bytes.begin(), other.bytes.end()); }

    std::vector<std::byte> bytes;
};

class HierarchyArchive : public rt::FMemoryReaderView {
public:
    using rt::FMemoryReaderView::FMemoryReaderView;
    using rt::FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = rt::MakeName(Text);
        return *this;
    }
};

enum Name : std::int32_t { NameNone, NameBone, NameRoot, NameChild, NameImported, NameSide, NameNameType, NameLeft, NameCount };

constexpr std::string_view Names[NameCount] = {"None", "Bone", "root", "child", "Imported", "Side", "Name", "Left"};

void writeKey(Writer& out, Name type, Name name) {
    out.put<std::int32_t>(type);
    out.put<std::int32_t>(name);
}

//! Initial local transforms only, as a cooked 5.8 hierarchy stores them.
void writeBoneStaticData(Writer& out, Name name, bool bWithPosition) {
    writeKey(out, NameBone, name);
    out.put<std::uint32_t>(0);
    if (bWithPosition) {
        out.put<std::uint8_t>(6);
        for (float Value : {1.f, 2.f, 3.f})
            out.put(Value);
    } else
        out.put<std::uint8_t>(0);
    out.put<std::int32_t>(NameImported);
}

Writer writeElements58() {
    Writer out;
    out.put<std::int32_t>(2);
    writeKey(out, NameBone, NameRoot);
    writeKey(out, NameBone, NameChild);
    writeBoneStaticData(out, NameRoot, false);
    writeBoneStaticData(out, NameChild, true);
    writeKey(out, NameNone, NameNone);
    writeKey(out, NameBone, NameRoot);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(1);
    writeKey(out, NameBone, NameChild);
    out.put<std::int32_t>(1);
    out.put<std::int32_t>(NameSide);
    out.put<std::int32_t>(NameNameType);
    out.put<std::int32_t>(NameSide);
    out.put<std::uint8_t>(6);
    out.put<std::int32_t>(NameLeft);
    out.put<std::int32_t>(0);
    return out;
}

Writer writeHierarchy58(bool bStoreCompressedBytes) {
    Writer out;
    out.put<std::int32_t>(43);
    for (std::uint32_t Flag : {1u, 1u, 1u, 1u, 0u, 1u, 0u})
        out.put(Flag);
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(NameCount);
    for (std::string_view Text : Names)
        out.string(Text);
    Writer elements = writeElements58();
    out.put<std::int32_t>(static_cast<std::int32_t>(elements.bytes.size()));
    out.put<std::uint32_t>(bStoreCompressedBytes ? 1 : 0);
    out.put<std::int32_t>(static_cast<std::int32_t>(elements.bytes.size()));
    out.append(elements);
    return out;
}

void writeTransform(Writer& out, double X, double Y, double Z) {
    for (double Value : {0.0, 0.0, 0.0, 1.0, X, Y, Z, 1.0, 1.0, 1.0})
        out.put(Value);
}

void writeBoneStaticData53(Writer& out, std::string_view name, double X) {
    out.string("Bone");
    out.string(name);
    out.put<std::int32_t>(0);
    for (int Index = 0; Index < 4; Index++) {
        writeTransform(out, X, 0.0, 0.0);
        out.put<std::uint32_t>(0);
    }
    out.string("Imported");
}

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

}

TEST(rigHierarchy58ReadsItsElementArchive) {
    Writer out = writeHierarchy58(false);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE5_8));
    rt::FRuntime runtime(provider);
    HierarchyArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_8);
    rt::URigHierarchy* Hierarchy = rt::NewObject<rt::URigHierarchy>(rt::GetTransientPackage());
    Hierarchy->Serialize(Ar);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    REQUIRE(Hierarchy->Num() == 2);
    rt::FRigElementKey RootKey;
    RootKey.Type = rt::ERigElementType::Bone;
    RootKey.Name = rt::FName("root");
    rt::FRigElementKey ChildKey;
    ChildKey.Type = rt::ERigElementType::Bone;
    ChildKey.Name = rt::FName("child");
    rt::FRigBoneElement* Root = Hierarchy->Find<rt::FRigBoneElement>(RootKey);
    rt::FRigBoneElement* Child = Hierarchy->Find<rt::FRigBoneElement>(ChildKey);
    REQUIRE(Root != nullptr);
    REQUIRE(Child != nullptr);
    CHECK(Root->ParentElement == nullptr);
    CHECK(Child->ParentElement == Root);
    CHECK(Child->SubIndex == 1);
    CHECK(Child->BoneType == rt::ERigBoneType::Imported);
    CHECK(Child->GetTransform().Initial.Local.Get().GetTranslation().Y == 2.0);
    CHECK(Hierarchy->Find<rt::FRigControlElement>(ChildKey) == nullptr);
    REQUIRE(Hierarchy->ElementMetadata.IsValidIndex(Child->MetadataStorageIndex));
    auto* Side = Hierarchy->ElementMetadata[Child->MetadataStorageIndex].MetadataMap.Find(rt::FName("Side"));
    REQUIRE(Side != nullptr);
    REQUIRE((*Side)->GetType() == rt::ERigMetadataType::Name);
    CHECK(static_cast<rt::FRigNameMetadata*>(Side->get())->GetValue() == rt::FName("Left"));
}

TEST(rigHierarchy58FailsOnBytesThatAreNotOodle) {
    Writer out = writeHierarchy58(true);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE5_8));
    rt::FRuntime runtime(provider);
    HierarchyArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_8);
    rt::URigHierarchy* Hierarchy = rt::NewObject<rt::URigHierarchy>(rt::GetTransientPackage());
    Hierarchy->Serialize(Ar);
    CHECK(Ar.IsError());
}

TEST(rigHierarchy53StoresStaticDataAfterEachKey) {
    Writer out;
    out.put<std::int32_t>(2);
    out.string("Bone");
    out.string("root");
    writeBoneStaticData53(out, "root", 0.0);
    out.string("Bone");
    out.string("child");
    writeBoneStaticData53(out, "child", 5.0);
    out.string("None");
    out.string("None");
    out.string("Bone");
    out.string("root");
    out.put<std::int32_t>(0);
    out.put<std::int32_t>(0);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE5_3));
    rt::FRuntime runtime(provider);
    HierarchyArchive Ar(out.bytes);
    Ar.SetGame(EGame::GAME_UE5_3);
    rt::URigHierarchy* Hierarchy = rt::NewObject<rt::URigHierarchy>(rt::GetTransientPackage());
    Hierarchy->Serialize(Ar);
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    REQUIRE(Hierarchy->Num() == 2);
    auto* Child = static_cast<rt::FRigBoneElement*>(Hierarchy->Get(1));
    CHECK(Child->ParentElement == Hierarchy->Get(0));
    CHECK(Child->GetTransform().Current.Global.Get().GetTranslation().X == 5.0);
}
