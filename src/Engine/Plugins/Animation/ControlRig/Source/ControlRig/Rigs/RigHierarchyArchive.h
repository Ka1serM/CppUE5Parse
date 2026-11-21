#pragma once

#include <vector>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FRigHierarchySerializationSettings {
    int32 ControlRigVersion = -1;
    bool bIsSerializingToPackage = true;
    bool bUseCompressedArchive = false;
    bool bStoreCompactTransforms = true;
    bool bSerializeLocalTransform = true;
    bool bSerializeGlobalTransform = true;
    bool bSerializeInitialTransform = true;
    bool bSerializeCurrentTransform = true;
    int32 SerializationPhase = 0;

    void Load(FArchive& Ar);
};

struct FRigHierarchyElementArchive {
    TArray<FName> UniqueNames;
    std::vector<std::byte> Bytes;

    void Load(FArchive& Ar);
};

class FRigHierarchyMemoryReader : public FMemoryReaderView {
public:
    FRigHierarchyMemoryReader(const FRigHierarchyElementArchive& Data, const FArchive& Outer)
        : FMemoryReaderView(Data.Bytes), Names(Data.UniqueNames) { CopyStateFrom(Outer); }

    using FMemoryReaderView::operator<<;
    FArchive& operator<<(FName& Name) override;

private:
    const TArray<FName>& Names;
};

}
