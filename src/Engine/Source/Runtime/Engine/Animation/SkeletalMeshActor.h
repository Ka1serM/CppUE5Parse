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
    bool bShouldDoAnimNotifies = true;
    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent = nullptr;

    explicit ASkeletalMeshActor(const FObjectInitializer& ObjectInitializer);

    USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
};

}
