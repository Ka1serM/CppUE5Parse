#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchy.h"

#include <optional>
#include <vector>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

using Version = FControlRigObjectVersion;

namespace {

class FRigHierarchyMemoryReader : public FMemoryReaderView {
public:
    FRigHierarchyMemoryReader(std::span<const std::byte> InBuffer, const TArray<FName>& InNames) : FMemoryReaderView(InBuffer), Names(InNames) {}

    using FMemoryReaderView::operator<<;

    FArchive& operator<<(FName& Value) override {
        int32 NameIndex = INDEX_NONE;
        *this << NameIndex;
        if (IsError())
            return *this;
        if (!Names.IsValidIndex(NameIndex)) {
            SetError(ErrorCode::InvalidData, "FRigHierarchyMemoryReader.Name");
            return *this;
        }
        Value = Names[NameIndex];
        return *this;
    }

    FArchive& operator<<(FText& Value) override {
        FString ValueString;
        *this << ValueString;
        Value = FText::FromString(ValueString);
        return *this;
    }

private:
    const TArray<FName>& Names;
};

}

URigHierarchy::~URigHierarchy() {
    Reset();
}

void URigHierarchy::Serialize(FArchive& Ar) {
    Load(Ar);
}

void URigHierarchy::Reset() {
    for (FRigBaseElement* Element : Elements)
        delete Element;
    Elements.Reset();
    ElementIndexLookup.Reset();
    ElementMetadata.Reset();
    ComponentScriptStructNames.Reset();
    PreviousHierarchyNameMap.Reset();
    PreviousHierarchyParentMap.Reset();
}

FRigBaseElement* URigHierarchy::Find(const FRigElementKey& InKey) const {
    const int32* Index = ElementIndexLookup.Find(InKey);
    return Index == nullptr ? nullptr : Get(*Index);
}

FRigBaseElement* URigHierarchy::MakeElement(FArchive& Ar, ERigElementType InElementType) {
    switch (InElementType) {
    case ERigElementType::Bone:
        return new FRigBoneElement();
    case ERigElementType::Null:
        return new FRigNullElement();
    case ERigElementType::Control:
        return new FRigControlElement();
    case ERigElementType::Curve:
        return new FRigCurveElement();
    case ERigElementType::Physics:
        if (Ar.GetGame() < EGame::GAME_UE5_5)
            return new FRigRigidBodyElement();
        if (Ar.GetGame() < EGame::GAME_UE5_6)
            return new FRigPhysicsElement();
        break;
    case ERigElementType::Reference:
        return new FRigReferenceElement();
    case ERigElementType::Connector:
        return new FRigConnectorElement();
    case ERigElementType::Socket:
        return new FRigSocketElement();
    default:
        break;
    }
    Ar.SetError(ErrorCode::InvalidData, "URigHierarchy.MakeElement");
    return nullptr;
}

FRigBaseMetadata* URigHierarchy::GetMetadataForElement(FRigBaseElement* InElement, const FName& InName, ERigMetadataType InType) {
    if (!ElementMetadata.IsValidIndex(InElement->MetadataStorageIndex))
        InElement->MetadataStorageIndex = ElementMetadata.Add(FMetadataStorage());

    FMetadataStorage& Storage = ElementMetadata[InElement->MetadataStorageIndex];
    if (std::shared_ptr<FRigBaseMetadata>* Existing = Storage.MetadataMap.Find(InName); Existing != nullptr && (*Existing)->GetType() == InType)
        return Existing->get();

    std::shared_ptr<FRigBaseMetadata> Metadata(FRigBaseMetadata::MakeMetadata(InName, InType));
    if (Metadata == nullptr)
        return nullptr;
    Storage.MetadataMap.Add(InName, Metadata);
    return Metadata.get();
}

void URigHierarchy::FMetadataStorage::Reset() {
    MetadataMap.Reset();
}

void URigHierarchy::FMetadataStorage::Serialize(FArchive& Ar) {
    static const UEnum* MetadataTypeEnum = StaticEnum<ERigMetadataType>();

    Reset();

    const int32 NumEntries = Ar.ReadCount(8);
    for (int32 Index = 0; Index < NumEntries && !Ar.IsError(); Index++) {
        FName ItemName, TypeName;
        Ar << ItemName;
        Ar << TypeName;
        if (Ar.IsError())
            return;

        const int64 Type = MetadataTypeEnum->GetValueByName(TypeName);
        std::shared_ptr<FRigBaseMetadata> Metadata(Type == INDEX_NONE ? nullptr : FRigBaseMetadata::MakeMetadata(ItemName, static_cast<ERigMetadataType>(Type)));
        if (Metadata == nullptr) {
            Ar.SetError(ErrorCode::InvalidData, "URigHierarchy.FMetadataStorage.Type");
            return;
        }
        Metadata->Serialize(Ar);

        MetadataMap.Add(ItemName, Metadata);
    }
}

void URigHierarchy::Load(FArchive& Ar) {
    Reset();

    FRigHierarchySerializationSettings SerializationSettings(Ar);
    if (Version::AtLeast(Ar, Version::RigHierarchyCompactTransformSerialization))
        SerializationSettings.Load(Ar);
    else if (Version::AtLeast(Ar, Version::RigHierarchyCompressElements))
        Ar << SerializationSettings.bUseCompressedArchive;
    SerializationSettings.SerializationPhase = FRigHierarchySerializationSettings::StaticData;

    FArchive* ArchiveForElements = &Ar;

    std::optional<FRigHierarchyMemoryReader> MemoryReader;
    std::vector<std::byte> UncompressedBytes;
    TArray<uint8> CompressedBytes;
    TArray<FName> UniqueNames;
    if (SerializationSettings.bUseCompressedArchive) {
        Ar << UniqueNames;

        int32 UncompressedSize = 0;
        Ar << UncompressedSize;

        bool bStoreCompressedBytes = false;
        Ar << bStoreCompressedBytes;

        Ar << CompressedBytes;
        if (Ar.IsError())
            return;

        std::span<const std::byte> ElementBytes = std::as_bytes(std::span(CompressedBytes.GetData(), static_cast<std::size_t>(CompressedBytes.Num())));
        if (bStoreCompressedBytes) {
            const Decompressor* Oodle = GetRuntimeProvider().getFiles().getCodecs().find(FName("Oodle"));
            if (Oodle == nullptr) {
                Ar.SetError(ErrorCode::UnknownCompression, "URigHierarchy.Load.Oodle");
                return;
            }
            if (UncompressedSize < 0) {
                Ar.SetError(ErrorCode::InvalidData, "URigHierarchy.Load.UncompressedSize");
                return;
            }
            std::vector<std::byte> Input(ElementBytes.size() + Oodle->getInputSlack());
            std::copy(ElementBytes.begin(), ElementBytes.end(), Input.begin());
            UncompressedBytes.resize(static_cast<std::size_t>(UncompressedSize) + Oodle->getOutputSlack());
            if (!Oodle->decompress(std::span(Input).first(ElementBytes.size()), std::span(UncompressedBytes).first(static_cast<std::size_t>(UncompressedSize)))) {
                Ar.SetError(ErrorCode::Decompression, "URigHierarchy.Load.Elements");
                return;
            }
            ElementBytes = std::span<const std::byte>(UncompressedBytes).first(static_cast<std::size_t>(UncompressedSize));
        }

        MemoryReader.emplace(ElementBytes, UniqueNames);
        MemoryReader->CopyStateFrom(Ar);
        ArchiveForElements = &*MemoryReader;
    }

    FArchive& ElementsAr = *ArchiveForElements;

    const int32 ElementCount = ElementsAr.ReadCount(8);

    const bool bAllocateStoragePerElement = !Version::AtLeast(ElementsAr, Version::RigHierarchyIndirectElementStorage);
    for (int32 ElementIndex = 0; ElementIndex < ElementCount && !ElementsAr.IsError(); ElementIndex++) {
        FRigElementKey Key;
        ElementsAr << Key;
        if (ElementsAr.IsError())
            break;

        FRigBaseElement* Element = MakeElement(ElementsAr, Key.Type);
        if (Element == nullptr)
            break;

        Element->Key = Key;
        Element->Owner = this;
        Element->SubIndex = 0;
        for (const FRigBaseElement* Other : Elements)
            Element->SubIndex += Other->GetType() == Key.Type ? 1 : 0;
        Element->Index = Elements.Add(Element);
        ElementIndexLookup.Add(Key, Element->Index);

        if (bAllocateStoragePerElement)
            Element->Load(ElementsAr, SerializationSettings);
    }

    if (!bAllocateStoragePerElement) {
        for (int32 ElementIndex = 0; ElementIndex < Elements.Num() && !ElementsAr.IsError(); ElementIndex++)
            Elements[ElementIndex]->Load(ElementsAr, SerializationSettings);
    }

    SerializationSettings.SerializationPhase = FRigHierarchySerializationSettings::InterElementData;
    for (int32 ElementIndex = 0; ElementIndex < Elements.Num() && !ElementsAr.IsError(); ElementIndex++)
        Elements[ElementIndex]->Load(ElementsAr, SerializationSettings);

    if (Version::AtLeast(ElementsAr, Version::RigHierarchyStoringPreviousNames)) {
        if (Version::AtLeast(ElementsAr, Version::RigHierarchyPreviousNameAndParentMapUsingHierarchyKey)) {
            ElementsAr << PreviousHierarchyNameMap;
            ElementsAr << PreviousHierarchyParentMap;
        } else {
            TMap<FRigElementKey, FRigElementKey> PreviousNameMap;
            TMap<FRigElementKey, FRigElementKey> PreviousParentMap;
            ElementsAr << PreviousNameMap;
            ElementsAr << PreviousParentMap;

            for (const TPair<FRigElementKey, FRigElementKey>& Pair : PreviousNameMap)
                PreviousHierarchyNameMap.Add(Pair.Key, Pair.Value);
            for (const TPair<FRigElementKey, FRigElementKey>& Pair : PreviousParentMap)
                PreviousHierarchyParentMap.Add(Pair.Key, Pair.Value);
        }
    }

    if (Version::AtLeast(ElementsAr, Version::RigHierarchyStoresElementMetadata)) {
        TMap<FRigElementKey, FMetadataStorage> LoadedElementMetadata;
        ElementsAr << LoadedElementMetadata;
        for (TPair<FRigElementKey, FMetadataStorage>& Entry : LoadedElementMetadata) {
            FRigBaseElement* Element = Find(Entry.Key);
            if (Element == nullptr) {
                ElementsAr.SetError(ErrorCode::InvalidData, "URigHierarchy.ElementMetadata");
                break;
            }
            Element->MetadataStorageIndex = ElementMetadata.Add(Entry.Value);
        }
    }

    if (Version::AtLeast(ElementsAr, Version::RigHierarchyStoresComponents)) {
        const int32 NumComponents = ElementsAr.ReadCount(12);
        if (NumComponents > 0) {
            ElementsAr << ComponentScriptStructNames;

            for (int32 ComponentIndex = 0; ComponentIndex < NumComponents && !ElementsAr.IsError(); ComponentIndex++) {
                int32 IndexOfScriptStruct = 0;
                ElementsAr << IndexOfScriptStruct;
                if (!ComponentScriptStructNames.IsValidIndex(IndexOfScriptStruct)) {
                    ElementsAr.SetError(ErrorCode::InvalidData, "URigHierarchy.Components");
                    break;
                }

                int64 ArchivePositionAfterComponent = 0;
                ElementsAr << ArchivePositionAfterComponent;
                if (ArchivePositionAfterComponent < ElementsAr.Tell() || ArchivePositionAfterComponent > ElementsAr.TotalSize()) {
                    ElementsAr.SetError(ErrorCode::InvalidData, "URigHierarchy.Components");
                    break;
                }
                ElementsAr.Seek(ArchivePositionAfterComponent);
            }
        }
    }

    if (MemoryReader.has_value() && MemoryReader->IsError())
        Ar.SetError(MemoryReader->GetError().code, "URigHierarchy.Load.Elements");
}

}
