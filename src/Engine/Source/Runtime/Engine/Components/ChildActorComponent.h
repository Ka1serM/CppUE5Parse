#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ChildActorComponent.generated.h"

namespace ue5parse::runtime {

class AActor;

UCLASS()
class UChildActorComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSubclassOf<AActor> ChildActorClass;
    UPROPERTY()
    TObjectPtr<AActor> ChildActor = nullptr;
    UPROPERTY()
    TObjectPtr<AActor> ChildActorTemplate = nullptr;

    explicit UChildActorComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
