#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GenericSmartObject.generated.h"

namespace ue5parse::runtime {

class USmartObjectComponent;
class UBillboardComponent;

UCLASS()
class AGenericSmartObject : public AActor {
    GENERATED_BODY()
public:
    explicit AGenericSmartObject(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USmartObjectComponent> SOComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
};

}
