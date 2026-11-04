#pragma once

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "InstancedStaticMeshComponent.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FInstancedStaticMeshInstanceData {
    GENERATED_BODY()
    UPROPERTY()
    FMatrix Transform = FMatrix::Identity;
};

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
    bool GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform) const;
};

}
