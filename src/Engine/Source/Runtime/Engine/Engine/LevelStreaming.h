#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"

#include "LevelStreaming.generated.h"

namespace ue5parse::runtime {

class ALevelStreamingVolume;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLevelStreamingLoadedStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLevelStreamingVisibilityStatus);

UCLASS(Abstract)
class ULevelStreaming : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName PackageName_DEPRECATED;
    UPROPERTY()
    TSoftObjectPtr<UWorld> WorldAsset;
    UPROPERTY()
    int32 StreamingPriority = 0;
    UPROPERTY()
    FName PackageNameToLoad;
    UPROPERTY()
    TArray<FName> LODPackageNames;
    UPROPERTY()
    FTransform LevelTransform = FTransform::Identity;
    UPROPERTY()
    bool bClientOnlyVisible = false;
    UPROPERTY()
    uint8 bShouldBeVisibleInEditor : 1 = true;
    UPROPERTY()
    uint8 bLocked : 1 = false;
    UPROPERTY()
    uint8 bIsStatic : 1 = false;
    UPROPERTY()
    uint8 bShouldBeLoaded : 1 = false;
    UPROPERTY()
    uint8 bShouldBeVisible : 1 = false;
    UPROPERTY()
    uint8 bShouldBlockOnLoad : 1 = false;
    UPROPERTY()
    uint8 bShouldBlockOnUnload : 1 = false;
    UPROPERTY()
    uint8 bDrawOnLevelStatusMap : 1 = true;
    UPROPERTY()
    FColor DrawColor_DEPRECATED;
    UPROPERTY()
    FLinearColor LevelColor = FLinearColor::White;
    UPROPERTY()
    TArray<TObjectPtr<ALevelStreamingVolume>> EditorStreamingVolumes;
    UPROPERTY()
    float MinTimeBetweenVolumeUnloadRequests = 2.0f;
    UPROPERTY()
    TArray<FString> Keywords;
    UPROPERTY()
    FLevelStreamingLoadedStatus OnLevelLoaded;
    UPROPERTY()
    FLevelStreamingLoadedStatus OnLevelUnloaded;
    UPROPERTY()
    FLevelStreamingVisibilityStatus OnLevelShown;
    UPROPERTY()
    FLevelStreamingVisibilityStatus OnLevelHidden;

    const TSoftObjectPtr<UWorld>& GetWorldAsset() const { return WorldAsset; }
    FName GetWorldAssetPackageFName() const;
};

}
