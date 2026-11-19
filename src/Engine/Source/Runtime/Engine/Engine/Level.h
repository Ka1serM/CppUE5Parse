#pragma once

#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Level.generated.h"

namespace ue5parse::runtime {

class ALevelScriptActor;
class ANavigationObjectBase;
class AWorldDataLayers;
class AWorldSettings;
class UMapBuildDataRegistry;
class UModel;
class UModelComponent;
class UNavigationDataChunk;
class UTexture2D;
class UWorld;
class UWorldPartitionRuntimeCell;

struct FStreamableTextureInstance {
    FBoxSphereBounds Bounds;
    float MinDistance = 0;
    float MaxDistance = 0;
    float TexelFactor = 0;

    friend FArchive& operator<<(FArchive& Ar, FStreamableTextureInstance& Instance);
};

USTRUCT()
struct FDynamicTextureInstance : public FStreamableTextureInstance {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UTexture2D> Texture = nullptr;
    UPROPERTY()
    bool bAttached = false;
    UPROPERTY()
    float OriginalRadius = 0;

    friend FArchive& operator<<(FArchive& Ar, FDynamicTextureInstance& Instance);
};

USTRUCT()
struct FReplicatedStaticActorDestructionInfo {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UClass> ObjClass = nullptr;
};

UCLASS()
class ULevel : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY(Transient)
    TObjectPtr<UWorld> OwningWorld = nullptr;
    UPROPERTY()
    TObjectPtr<UModel> Model = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UModelComponent>> ModelComponents;
    UPROPERTY()
    int32 NumTextureStreamingUnbuiltComponents = 0;
    UPROPERTY()
    int32 NumTextureStreamingDirtyResources = 0;
    UPROPERTY()
    TObjectPtr<ALevelScriptActor> LevelScriptActor = nullptr;
    UPROPERTY()
    TObjectPtr<ANavigationObjectBase> NavListStart = nullptr;
    UPROPERTY()
    TObjectPtr<ANavigationObjectBase> NavListEnd = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UNavigationDataChunk>> NavDataChunks;
    UPROPERTY()
    float LightmapTotalSize = 0;
    UPROPERTY()
    float ShadowmapTotalSize = 0;
    UPROPERTY()
    TArray<FVector> StaticNavigableGeometry;
    UPROPERTY()
    TArray<FGuid> StreamingTextureGuids;
    UPROPERTY()
    TArray<FName> StreamingTextures;
    UPROPERTY()
    uint32 PackedTextureStreamingQualityLevelFeatureLevel = 0;
    UPROPERTY()
    FGuid LevelBuildDataId;
    UPROPERTY()
    TObjectPtr<UMapBuildDataRegistry> MapBuildData = nullptr;
    UPROPERTY()
    FIntVector LightBuildLevelOffset = FIntVector(0, 0, 0);
    UPROPERTY()
    uint8 bIsLightingScenario : 1 = 0;
    UPROPERTY()
    uint8 bTextureStreamingRotationChanged : 1 = 0;
    UPROPERTY()
    uint8 bLocked : 1 = 0;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> MovieSceneBindingsArray;
    UPROPERTY()
    uint8 bIsPartitioned : 1 = 0;
    UPROPERTY()
    uint8 bIsWorldPartitionRuntimeCell : 1 = 0;
    UPROPERTY()
    TObjectPtr<AWorldSettings> WorldSettings = nullptr;
    UPROPERTY()
    TObjectPtr<AWorldDataLayers> WorldDataLayers = nullptr;
    UPROPERTY()
    TSoftObjectPtr<UWorldPartitionRuntimeCell> WorldPartitionRuntimeCell;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;
    UPROPERTY()
    TArray<FReplicatedStaticActorDestructionInfo> DestroyedReplicatedStaticActors;

    TArray<AActor*> Actors;
    FURL URL;

    void Serialize(FArchive& Ar) override;
    void PostLoad() override;
};

}
