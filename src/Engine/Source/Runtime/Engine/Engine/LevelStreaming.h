#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"

#include "LevelStreaming.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class ULevelStreaming : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSoftObjectPtr<UWorld> WorldAsset;
    UPROPERTY()
    FName PackageNameToLoad;

    const TSoftObjectPtr<UWorld>& GetWorldAsset() const { return WorldAsset; }
    FName GetWorldAssetPackageFName() const;
};

}
