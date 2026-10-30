#pragma once

#include "ue5parse/runtime/Engine/Components/StaticMeshComponent.h"

#include "InstancedStaticMeshComponent.generated.h"

namespace ue5parse::runtime {

// FInstancedStaticMeshInstanceData: the transform of one instance, relative to its component.
USTRUCT()
struct FInstancedStaticMeshInstanceData {
    GENERATED_BODY()
    UPROPERTY()
    FMatrix Transform = FMatrix::Identity;
};

// UInstancedStaticMeshComponent: one static mesh drawn at many transforms (UE's
// Components/InstancedStaticMeshComponent.h). The instances are serialized natively.
UCLASS()
class UInstancedStaticMeshComponent : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FInstancedStaticMeshInstanceData> PerInstanceSMData;
    UPROPERTY()
    TArray<float> PerInstanceSMCustomData;

    void Serialize(FArchive& Ar) override;
    UFUNCTION(BlueprintCallable)
    int32 GetInstanceCount() const { return PerInstanceSMData.Num(); }
    // The transform of an instance relative to the component; false for an index that is not there.
    bool GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform) const;
};

}
