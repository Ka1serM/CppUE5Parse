#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"

#include "ControlRigControlActor.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UCLASS()
class AControlRigControlActor : public AActor {
    GENERATED_BODY()
public:
    explicit AControlRigControlActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<AActor> ActorToTrack = nullptr;
    UPROPERTY()
    TSubclassOf<UObject> ControlRigClass;
    UPROPERTY()
    bool bRefreshOnTick = true;
    UPROPERTY()
    bool bIsSelectable = false;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaterialOverride = nullptr;
    UPROPERTY()
    FString ColorParameter;
    UPROPERTY()
    bool bCastShadows = false;
    UPROPERTY()
    TObjectPtr<USceneComponent> ActorRootComponent = nullptr;
};

}
