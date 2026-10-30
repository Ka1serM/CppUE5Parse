#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtrTemplates.h"
#include "ue5parse/runtime/Engine/Engine/World.h"

#include "LevelStreaming.generated.h"

namespace ue5parse::runtime {

// ULevelStreaming: a level that a world streams in, by the path of its world (UE's Engine/LevelStreaming.h).
UCLASS(Abstract)
class ULevelStreaming : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSoftObjectPtr<UWorld> WorldAsset;
    UPROPERTY()
    FName PackageNameToLoad;

    const TSoftObjectPtr<UWorld>& GetWorldAsset() const { return WorldAsset; }
    // The package of the streamed world, "/Game/Maps/Ascent/Ascent_Art".
    FName GetWorldAssetPackageFName() const;
};

}
