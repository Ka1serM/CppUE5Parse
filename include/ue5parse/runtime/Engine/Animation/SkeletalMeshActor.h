#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "SkeletalMeshActor.generated.h"

namespace ue5parse::runtime {

class USkeletalMeshComponent;

// ASkeletalMeshActor: an actor that shows one skeletal mesh (UE's Animation/SkeletalMeshActor.h). The component comes
// from the loaded actor; the constructor that makes it is not ported.
UCLASS()
class ASkeletalMeshActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bShouldDoAnimNotifies = false;
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent = nullptr;

    USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
};

}
