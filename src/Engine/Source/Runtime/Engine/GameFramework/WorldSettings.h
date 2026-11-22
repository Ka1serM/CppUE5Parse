#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/EngineDefines.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"
#include "Engine/Source/Runtime/Engine/Sound/AudioVolume.h"
#include "Engine/Source/Runtime/Engine/Sound/ReverbSettings.h"

#include "WorldSettings.generated.h"

namespace ue5parse::runtime {

class ADefaultPhysicsVolume;
class AGameModeBase;
class AGameNetworkManager;
class APlayerState;
class UAISystemBase;
class UBookMark;
class UBookmarkBase;
class UDamageType;
class UDataLayerAsset;
class ULevelStreaming;
class UNavigationSystemConfig;
class UNetConnection;
class UPhysicsCollisionHandler;
class USoundMix;
class UWorldPartition;

UENUM()
enum EVisibilityAggressiveness : int32 { VIS_LeastAggressive, VIS_ModeratelyAggressive, VIS_MostAggressive, VIS_Max };

UENUM()
enum EVolumeLightingMethod : int32 { VLM_VolumetricLightmap, VLM_SparseVolumeLightingSamples };

USTRUCT()
struct FLightmassWorldInfoSettings {
    GENERATED_BODY()
    UPROPERTY()
    float StaticLightingLevelScale = 1;
    UPROPERTY()
    int32 NumIndirectLightingBounces = 3;
    UPROPERTY()
    int32 NumSkyLightingBounces = 1;
    UPROPERTY()
    float IndirectLightingQuality = 1;
    UPROPERTY()
    float IndirectLightingSmoothness = 1;
    UPROPERTY()
    FColor EnvironmentColor = FColor(0, 0, 0, 0);
    UPROPERTY()
    float EnvironmentIntensity = 1.0f;
    UPROPERTY()
    float EmissiveBoost = 1.0f;
    UPROPERTY()
    float DiffuseBoost = 1.0f;
    UPROPERTY()
    TEnumAsByte<EVolumeLightingMethod> VolumeLightingMethod = VLM_VolumetricLightmap;
    UPROPERTY()
    uint8 bUseAmbientOcclusion : 1 = false;
    UPROPERTY()
    uint8 bGenerateAmbientOcclusionMaterialMask : 1 = false;
    UPROPERTY()
    uint8 bVisualizeMaterialDiffuse : 1 = false;
    UPROPERTY()
    uint8 bVisualizeAmbientOcclusion : 1 = false;
    UPROPERTY()
    uint8 bCompressLightmaps : 1 = true;
    UPROPERTY()
    float VolumetricLightmapDetailCellSize = 200;
    UPROPERTY()
    float VolumetricLightmapMaximumBrickMemoryMb = 30;
    UPROPERTY()
    float VolumetricLightmapLoadingCellSize = 3200;
    UPROPERTY()
    float VolumetricLightmapSphericalHarmonicSmoothing = .02f;
    UPROPERTY()
    float VolumeLightSamplePlacementScale = 1;
    UPROPERTY()
    float DirectIlluminationOcclusionFraction = 0.5f;
    UPROPERTY()
    float IndirectIlluminationOcclusionFraction = 1.0f;
    UPROPERTY()
    float OcclusionExponent = 1.0f;
    UPROPERTY()
    float FullyOccludedSamplesFraction = 1.0f;
    UPROPERTY()
    float MaxOcclusionDistance = 200.0f;
};

USTRUCT()
struct FGameModePrefix {
    GENERATED_BODY()
    UPROPERTY()
    FString Prefix;
    UPROPERTY()
    FString GameMode;
};

USTRUCT()
struct FNetViewer {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UNetConnection> Connection = nullptr;
    UPROPERTY()
    TObjectPtr<AActor> InViewer = nullptr;
    UPROPERTY()
    TObjectPtr<AActor> Viewer = nullptr;
    UPROPERTY()
    TObjectPtr<AActor> ViewTarget = nullptr;
    UPROPERTY()
    FVector ViewLocation = FVector(0, 0, 0);
    UPROPERTY()
    FVector ViewDir = FVector(0, 0, 0);
};

USTRUCT()
struct FNaniteSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bAllowMaskedMaterials = true;
};

USTRUCT()
struct FBroadphaseSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bUseMBPOnClient = false;
    UPROPERTY()
    bool bUseMBPOnServer = false;
    UPROPERTY()
    bool bUseMBPOuterBounds = false;
    UPROPERTY()
    FBox MBPBounds = FBox(FVector(0, 0, 0), FVector(0, 0, 0));
    UPROPERTY()
    FBox MBPOuterBounds = FBox(FVector(0, 0, 0), FVector(0, 0, 0));
    UPROPERTY()
    uint32 MBPNumSubdivs = 2;
};

UCLASS()
class AWorldSettings : public AInfo {
    GENERATED_BODY()
public:
    explicit AWorldSettings(const FObjectInitializer& ObjectInitializer);

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    uint8 bEnableWorldBoundsChecks : 1 = true;
    UPROPERTY()
    uint8 bEnableNavigationSystem : 1 = true;
    UPROPERTY()
    uint8 bEnableAISystem : 1 = true;
    UPROPERTY()
    uint8 bEnableWorldComposition : 1 = false;
    UPROPERTY()
    uint8 bUseClientSideLevelStreamingVolumes : 1 = 0;
    UPROPERTY()
    uint8 bEnableWorldOriginRebasing : 1 = false;
    UPROPERTY()
    uint8 bGlobalGravitySet : 1 = 0;
    UPROPERTY()
    uint8 bEnableLargeWorlds_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bMinimizeBSPSections : 1 = 0;
    UPROPERTY()
    uint8 bPrecomputeVisibility : 1 = 0;
    UPROPERTY()
    uint8 bPlaceCellsOnlyAlongCameraTracks : 1 = false;
    UPROPERTY()
    uint8 bForceNoPrecomputedLighting : 1 = 0;
    UPROPERTY()
    uint8 bForceVolumetricLightmapsOnly : 1 = 0;
    UPROPERTY()
    uint8 bHighPriorityLoading : 1 = 0;
    UPROPERTY()
    uint8 bHighPriorityLoadingLocal : 1 = 0;
    UPROPERTY()
    uint8 bOverrideDefaultBroadphaseSettings : 1 = 0;
    UPROPERTY()
    uint8 bGenerateSingleClusterForLevel : 1 = false;
    UPROPERTY()
    uint8 bReuseAddressAndPort : 1 = false;
    UPROPERTY()
    uint8 bUseAutomaticUGCNavigation : 1 = false;
    UPROPERTY()
    TObjectPtr<UWorldPartition> WorldPartition = nullptr;
    UPROPERTY()
    TObjectPtr<UNavigationSystemConfig> NavigationSystemConfig = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UDataLayerAsset>> BaseNavmeshDataLayers;
    UPROPERTY()
    TSoftClassPtr<UAISystemBase> AISystemClass = TSoftClassPtr<UAISystemBase>(FSoftObjectPath("/Script/AIModule.AISystem"));
    UPROPERTY()
    FVector LevelInstancePivotOffset = FVector(0, 0, 0);
    UPROPERTY()
    float KillZ = -1048575.0f;
    UPROPERTY()
    TSubclassOf<UDamageType> KillZDamageType;
    UPROPERTY()
    float GlobalGravityZ = 0;
    UPROPERTY()
    TSubclassOf<ADefaultPhysicsVolume> DefaultPhysicsVolumeClass;
    UPROPERTY()
    TSubclassOf<UPhysicsCollisionHandler> PhysicsCollisionHandlerClass;
    UPROPERTY()
    TSubclassOf<AGameModeBase> DefaultGameMode;
    UPROPERTY()
    TArray<FGameModePrefix> DefaultMapPrefixes;
    UPROPERTY()
    TSubclassOf<AGameNetworkManager> GameNetworkManagerClass;
    UPROPERTY()
    int32 PackedLightAndShadowMapTextureSize = 1024;
    UPROPERTY()
    FVector DefaultColorScale = FVector(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    float DefaultMaxDistanceFieldOcclusionDistance = 600;
    UPROPERTY()
    float GlobalDistanceFieldViewDistance = 20000;
    UPROPERTY()
    float DynamicIndirectShadowsSelfShadowingIntensity = .8f;
    UPROPERTY()
    float MonoCullingDistance = 0;
    UPROPERTY()
    int32 VisibilityCellSize = 200;
    UPROPERTY()
    TEnumAsByte<EVisibilityAggressiveness> VisibilityAggressiveness = VIS_LeastAggressive;
    UPROPERTY()
    float VolumetricLightmapLoadingRange = 6400;
    UPROPERTY()
    FLightmassWorldInfoSettings LightmassSettings;
    UPROPERTY()
    TEnumAsByte<ELightingBuildQuality> LevelLightingQuality = Quality_Preview;
    UPROPERTY()
    FNaniteSettings NaniteSettings;
    UPROPERTY()
    FReverbSettings DefaultReverbSettings;
    UPROPERTY()
    FInteriorSettings DefaultAmbientZoneSettings;
    UPROPERTY()
    TObjectPtr<USoundMix> DefaultBaseSoundMix = nullptr;
    UPROPERTY()
    FString Title;
    UPROPERTY()
    float WorldToMeters = 100.0f;
    UPROPERTY()
    TObjectPtr<UBookMark> BookMarks[10] = {};
    UPROPERTY()
    TArray<TObjectPtr<ULevelStreaming>> StreamingLevels_DEPRECATED;
    UPROPERTY(Replicated)
    float TimeDilation = 1.0f;
    UPROPERTY()
    float MatineeTimeDilation = 1.0f;
    UPROPERTY()
    float DemoPlayTimeDilation = 1.0f;
    UPROPERTY()
    float MinGlobalTimeDilation = 0.0001f;
    UPROPERTY()
    float MaxGlobalTimeDilation = 20.0f;
    UPROPERTY()
    float MinCinematicTimeDilation = 0.0001f;
    UPROPERTY()
    float MaxCinematicTimeDilation = 20.0f;
    UPROPERTY()
    float MinUndilatedFrameTime = 0.0005f;
    UPROPERTY()
    float MaxUndilatedFrameTime = 0.4f;
    UPROPERTY()
    FBroadphaseSettings BroadphaseSettings;
    UPROPERTY()
    TObjectPtr<APlayerState> Pauser = nullptr;
    UPROPERTY(Replicated)
    TObjectPtr<APlayerState> PauserPlayerState = nullptr;
    UPROPERTY()
    TArray<FNetViewer> ReplicationViewers;
    UPROPERTY()
    int32 MaxNumberOfBookmarks = 10;
    UPROPERTY()
    TSubclassOf<UBookmarkBase> DefaultBookmarkClass;
    UPROPERTY()
    TArray<TObjectPtr<UBookmarkBase>> BookmarkArray;
    UPROPERTY()
    TSubclassOf<UBookmarkBase> LastBookmarkClass;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;

    float GetEffectiveTimeDilation() const { return FMath::Clamp(TimeDilation * DemoPlayTimeDilation, MinGlobalTimeDilation, MaxGlobalTimeDilation); }
    float SetTimeDilation(float NewTimeDilation) {
        TimeDilation = FMath::Clamp(NewTimeDilation, MinGlobalTimeDilation, MaxGlobalTimeDilation);
        return TimeDilation;
    }
    APlayerState* GetPauserPlayerState() const { return PauserPlayerState; }
};

}
