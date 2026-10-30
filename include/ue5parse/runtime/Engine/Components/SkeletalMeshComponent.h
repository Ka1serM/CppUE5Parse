#pragma once

#include "ue5parse/runtime/Engine/Components/SkinnedMeshComponent.h"

#include "SkeletalMeshComponent.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;

// USkeletalMeshComponent: a placed skeletal mesh (UE's Components/SkeletalMeshComponent.h).
UCLASS()
class USkeletalMeshComponent : public USkinnedMeshComponent {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    USkeletalMesh* GetSkeletalMeshAsset() const;
};

}
