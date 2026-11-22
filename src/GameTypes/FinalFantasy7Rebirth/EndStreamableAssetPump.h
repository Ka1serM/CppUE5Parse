#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "GameTypes/FinalFantasy7Rebirth/MemoryMappedAsset.h"

#include "EndStreamableAssetPump.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FEndStreamableAssetPumpSection {
    GENERATED_BODY()
    UPROPERTY()
    TMap<FString, uint16> Assets;
};

UCLASS()
class UEndStreamableAssetPump : public UMemoryMappedAsset {
    GENERATED_BODY()
public:
    explicit UEndStreamableAssetPump(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TMap<int32, FEndStreamableAssetPumpSection> AssetMap;
};

}
