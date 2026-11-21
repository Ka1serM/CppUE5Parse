#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyArchive.h"

#include <algorithm>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"

namespace ue5parse::runtime {

void FRigHierarchySerializationSettings::Load(FArchive& Ar) {
    using Version = FControlRigObjectVersion;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyCompactTransformSerialization, EGame::GAME_UE5_6)) {
        Ar << ControlRigVersion << bIsSerializingToPackage << bUseCompressedArchive << bStoreCompactTransforms;
        Ar << bSerializeLocalTransform << bSerializeGlobalTransform << bSerializeInitialTransform << bSerializeCurrentTransform << SerializationPhase;
    } else if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyCompressElements, EGame::GAME_UE5_6))
        Ar << bUseCompressedArchive;
}

void FRigHierarchyElementArchive::Load(FArchive& Ar) {
    int32 UncompressedSize = 0;
    bool bStoreCompressedBytes = false;
    Ar << UniqueNames << UncompressedSize << bStoreCompressedBytes;
    const int32 StoredSize = Ar.ReadCount(1);
    if (Ar.IsError())
        return;
    if (UncompressedSize < 0 || (!bStoreCompressedBytes && StoredSize != UncompressedSize)) {
        Ar.SetError(ErrorCode::InvalidData, "FRigHierarchyElementArchive.Size");
        return;
    }
    const auto Stored = Ar.ReadView(StoredSize);
    if (Ar.IsError())
        return;
    if (!bStoreCompressedBytes) {
        Bytes.assign(Stored.begin(), Stored.end());
        return;
    }
    static const Codecs Compression;
    const Decompressor* Codec = Compression.find("Oodle");
    std::vector<std::byte> Input(Stored.size() + Codec->getInputSlack());
    std::copy(Stored.begin(), Stored.end(), Input.begin());
    Bytes.resize(static_cast<size_t>(UncompressedSize) + Codec->getOutputSlack());
    if (!Codec->decompress(std::span(Input).first(StoredSize), std::span(Bytes).first(UncompressedSize))) {
        Bytes.clear();
        Ar.SetError(ErrorCode::Decompression, "FRigHierarchyElementArchive.Oodle");
        return;
    }
    Bytes.resize(UncompressedSize);
}

FArchive& FRigHierarchyMemoryReader::operator<<(FName& Name) {
    int32 NameIndex = -1;
    *this << NameIndex;
    if (!Names.IsValidIndex(NameIndex)) {
        SetError(ErrorCode::InvalidData, "FRigHierarchyMemoryReader.Name");
        return *this;
    }
    Name = Names[NameIndex];
    return *this;
}

}
