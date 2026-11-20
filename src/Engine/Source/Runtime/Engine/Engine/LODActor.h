#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LODActor.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS()
class ALODActor : public AActor {
    GENERATED_BODY()
public:
    explicit ALODActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<AActor>> SubActors;
    UPROPERTY()
    float LODDrawDistance = 0.0f;
    UPROPERTY()
    int32 LODLevel = 0;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> SubObjects;
};

}
