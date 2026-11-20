#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NavLinkProxy.generated.h"

namespace ue5parse::runtime {

class UNavLinkCustomComponent;

UCLASS()
class ANavLinkProxy : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UNavLinkCustomComponent> SmartLinkComp = nullptr;

    explicit ANavLinkProxy(const FObjectInitializer& ObjectInitializer);
};

}
