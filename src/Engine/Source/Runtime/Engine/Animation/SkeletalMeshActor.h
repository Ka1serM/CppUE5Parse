#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "SkeletalMeshActor.generated.h"

namespace ue5parse::runtime {

class USkeletalMeshComponent;

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
