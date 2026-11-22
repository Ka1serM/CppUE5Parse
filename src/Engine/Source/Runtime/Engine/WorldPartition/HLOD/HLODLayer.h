#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/HLOD/HLODBuilder.h"

#include "HLODLayer.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EHLODLayerType : uint8 {
    Instancing,
    MeshMerge,
    MeshSimplify,
    MeshApproximate,
    Custom,
    CustomHLODActor,
};

UENUM()
enum class EHLODLayerEditorLoadingBehavior : uint8 {
    Default,
    ForceLoaded,
    ForceNotLoaded,
};

UCLASS()
class UHLODLayer : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    EHLODLayerType LayerType = EHLODLayerType::Instancing;
    UPROPERTY()
    TSubclassOf<UHLODBuilder> HLODBuilderClass;
    UPROPERTY()
    TObjectPtr<UHLODBuilderSettings> HLODBuilderSettings = nullptr;
    UPROPERTY()
    uint32 bIsSpatiallyLoaded : 1 = true;
    UPROPERTY()
    int32 CellSize = 25600;
    UPROPERTY()
    double LoadingRange = 51200.0;
    UPROPERTY()
    TObjectPtr<UHLODLayer> ParentLayer = nullptr;
    UPROPERTY()
    TObjectPtr<UHLODLayer> LinkedLayer = nullptr;
    UPROPERTY()
    uint32 bForceRayTracingFarField : 1 = false;
    UPROPERTY()
    EHLODLayerEditorLoadingBehavior EditorLoadingBehavior = EHLODLayerEditorLoadingBehavior::Default;
    UPROPERTY()
    uint32 bAlwaysLoaded_DEPRECATED : 1 = false;
};

}
