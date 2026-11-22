#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectDebugRenderingComponent.h"

#include "SmartObjectTestingActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USmartObjectTestRenderingComponent : public USmartObjectDebugRenderingComponent {
    GENERATED_BODY()
public:
    explicit USmartObjectTestRenderingComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class ASmartObjectTestingActor : public AActor {
    GENERATED_BODY()
public:
    explicit ASmartObjectTestingActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<TObjectPtr<UObject>> Tests;
    UPROPERTY()
    TObjectPtr<USmartObjectTestRenderingComponent> RenderingComponent = nullptr;
    UPROPERTY()
    bool bRunTestsEachFrame = false;
};

}
