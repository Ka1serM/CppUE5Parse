#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "SceneCaptureComponent.generated.h"

namespace ue5parse::runtime {

class UPrimitiveComponent;
class UMaterialParameterCollection;

USTRUCT()
struct FEngineShowFlagsSetting {
    GENERATED_BODY()
    UPROPERTY()
    FString ShowFlagName;
    UPROPERTY()
    bool Enabled = false;
};

UENUM()
enum class ESceneCapturePrimitiveRenderMode : uint8 { PRM_LegacySceneCapture, PRM_RenderScenePrimitives, PRM_UseShowOnlyList };

UCLASS(Abstract)
class USceneCaptureComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    ESceneCapturePrimitiveRenderMode PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_LegacySceneCapture;
    UPROPERTY()
    TEnumAsByte<ESceneCaptureSource> CaptureSource = SCS_SceneColorHDR;
    UPROPERTY()
    uint8 bCaptureEveryFrame : 1 = true;
    UPROPERTY()
    uint8 bCaptureOnMovement : 1 = true;
    UPROPERTY()
    uint8 bExcludeFromSceneTextureExtents : 1 = false;
    UPROPERTY()
    uint8 bOverrideVirtualTextureThrottle : 1 = false;
    UPROPERTY()
    uint8 bFiniteFarPlane : 1 = false;
    UPROPERTY()
    bool bAlwaysPersistRenderingState = false;
    UPROPERTY()
    TArray<TWeakObjectPtr<UPrimitiveComponent>> HiddenComponents;
    UPROPERTY()
    TArray<TObjectPtr<AActor>> HiddenActors;
    UPROPERTY()
    TArray<TWeakObjectPtr<UPrimitiveComponent>> ShowOnlyComponents;
    UPROPERTY()
    TArray<TObjectPtr<AActor>> ShowOnlyActors;
    UPROPERTY()
    float LODDistanceFactor = 1.0f;
    UPROPERTY()
    float MaxViewDistanceOverride = -1.0f;
    UPROPERTY()
    int32 CaptureSortPriority = 0;
    UPROPERTY()
    bool bUseRayTracingIfEnabled = false;
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> CollectionTransform = nullptr;
    UPROPERTY()
    FName CollectionTransformWorldToLocal;
    UPROPERTY()
    FName CollectionTransformProjection;
    UPROPERTY()
    FViewLightingChannels ViewLightingChannels;
    UPROPERTY()
    TArray<FEngineShowFlagsSetting> ShowFlagSettings;
    UPROPERTY()
    FString ProfilingEventName;

    explicit USceneCaptureComponent(const FObjectInitializer& ObjectInitializer);
};

}
