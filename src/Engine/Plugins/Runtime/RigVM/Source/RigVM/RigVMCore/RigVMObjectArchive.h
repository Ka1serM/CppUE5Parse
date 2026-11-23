#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RigVMObjectArchive.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigVMObjectArchive {
    GENERATED_BODY()
    UPROPERTY()
    TMap<FGuid, int32> Versions;
    UPROPERTY()
    TArray<uint8> Buffer;
    UPROPERTY()
    int32 UncompressedSize = -1;
    UPROPERTY()
    int32 CompressedSize = -1;
    UPROPERTY()
    bool bIsCompressed = false;

    friend FArchive& operator<<(FArchive& Ar, FRigVMObjectArchive& Data);
};

}
