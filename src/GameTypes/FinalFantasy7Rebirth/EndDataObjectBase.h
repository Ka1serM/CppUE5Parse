#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "GameTypes/FinalFantasy7Rebirth/MemoryMappedAsset.h"

#include "EndDataObjectBase.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UEndDataObjectBase : public UMemoryMappedAsset {
    GENERATED_BODY()
public:
    explicit UEndDataObjectBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TMap<FName, FInstancedStruct> DataTable;
};

}
