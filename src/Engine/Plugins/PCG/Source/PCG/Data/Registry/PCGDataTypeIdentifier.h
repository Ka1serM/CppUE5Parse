#pragma once

#include "Engine/Plugins/PCG/Source/PCG/Data/Registry/PCGDataType.h"
#include "Engine/Plugins/PCG/Source/PCG/PCGCommon.h"

#include "PCGDataTypeIdentifier.generated.h"

namespace ue5parse::runtime {

struct FPropertyTag;

USTRUCT()
struct FPCGDataTypeIdentifier {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPCGDataTypeBaseId> Ids;
    UPROPERTY()
    int32 CustomSubtype = -1;
    //! The EPCGDataType flags of a pin saved before 5.7, which UE maps to registry ids on load.
    EPCGDataType LegacyDataType = EPCGDataType::None;

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FPCGDataTypeIdentifier> : TStructOpsTypeTraitsBase2<FPCGDataTypeIdentifier> {
    enum { WithSerializeFromMismatchedTag = true };
};

}
