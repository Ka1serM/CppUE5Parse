#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"

#include "LionAnimSequenceAssetUserData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FLionAnimTransformKey {
    GENERATED_BODY()
    UPROPERTY()
    double Time = 0;
    UPROPERTY()
    FTransform Transform;
};

USTRUCT()
struct FLionAnimTransformTrack {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FLionAnimTransformKey> Keys;
};

USTRUCT()
struct FLionAnimVectorKey {
    GENERATED_BODY()
    UPROPERTY()
    double Time = 0;
    UPROPERTY()
    FVector Value;
};

//! 2XKO saves fixed point numbers with 32 fraction bits; the keys of the maps are times.
UCLASS()
class ULionAnimSequenceAssetUserData : public UAssetUserData {
    GENERATED_BODY()
public:
    explicit ULionAnimSequenceAssetUserData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TMap<FName, FLionAnimTransformTrack> Data;
    UPROPERTY()
    TArray<FLionAnimVectorKey> Vectors;
};

}
