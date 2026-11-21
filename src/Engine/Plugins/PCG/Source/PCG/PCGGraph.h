#pragma once

#include "Engine/Plugins/PCG/Source/PCG/PCGCommon.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PCGGraph.generated.h"

namespace ue5parse::runtime {

class UPCGGraphCompilationData;
class UPCGNode;

UCLASS(abstract)
class UPCGGraphInterface : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGGraphInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UPCGGraph : public UPCGGraphInterface {
    GENERATED_BODY()
public:
    explicit UPCGGraph(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<TObjectPtr<UPCGNode>> Nodes;
    UPROPERTY()
    TObjectPtr<UPCGNode> InputNode = nullptr;
    UPROPERTY()
    TObjectPtr<UPCGNode> OutputNode = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UPCGGraph>> EmbeddedSubgraphs;
    UPROPERTY()
    bool bLandscapeUsesMetadata = true;
    UPROPERTY()
    FInstancedPropertyBag UserParameters;
    UPROPERTY()
    bool bUseHierarchicalGeneration = false;
    UPROPERTY()
    EPCGHiGenGrid HiGenGridSize = EPCGHiGenGrid::Grid256;
    UPROPERTY()
    uint32 HiGenExponential = 0;
    UPROPERTY()
    double HiGenGridSizeMultiplier = 1.0;
    UPROPERTY()
    bool bUse2DGrid = true;
    UPROPERTY()
    bool bIsEditorOnly = false;
    UPROPERTY()
    FPerPlatformBool ShouldCook{true};
    UPROPERTY()
    TObjectPtr<UPCGGraphCompilationData> CookedCompilationData = nullptr;
    UPROPERTY()
    FPCGRuntimeGenerationRadii GenerationRadii;
    UPROPERTY()
    bool bUseActorComponentlessGeneration = false;
    UPROPERTY()
    TArray<FString> ConsoleVariablesTriggeringRefresh;
    UPROPERTY()
    bool bAutoRefreshOnRenderStateRefresh = true;
    UPROPERTY()
    bool bAutoRefreshOnQualitySettingsChanged = true;
    UPROPERTY()
    bool bHasDefaultConstructedInputs = true;
};

UCLASS()
class UPCGGraphInstance : public UPCGGraphInterface {
    GENERATED_BODY()
public:
    explicit UPCGGraphInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UPCGGraphInterface> Graph = nullptr;
};

}
