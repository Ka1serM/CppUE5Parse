#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Engine/NetSerialization.h"

#include "GameplayPrediction.generated.h"

namespace ue5parse::runtime {

class UPackageMap;

USTRUCT()
struct FPredictionKey {
    GENERATED_BODY()
    UPROPERTY()
    int16 Current = 0;
    UPROPERTY()
    int16 Base = 0;
    UPROPERTY()
    TObjectPtr<UPackageMap> PredictiveConnection = nullptr;
    UPROPERTY()
    bool bIsStale = false;
    UPROPERTY()
    bool bIsServerInitiated = false;
};

USTRUCT()
struct FReplicatedPredictionKeyItem : public FFastArraySerializerItem {
    GENERATED_BODY()
    UPROPERTY()
    FPredictionKey PredictionKey;
};

USTRUCT()
struct FReplicatedPredictionKeyMap : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FReplicatedPredictionKeyItem> PredictionKeys;
};

}
