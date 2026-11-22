#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavLinkDefinition.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NavLinkProxy.generated.h"

namespace ue5parse::runtime {

class UNavLinkCustomComponent;

UCLASS()
class ANavLinkProxy : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FNavigationLink> PointLinks;
    UPROPERTY()
    TArray<FNavigationSegmentLink> SegmentLinks;
    UPROPERTY()
    TObjectPtr<UNavLinkCustomComponent> SmartLinkComp = nullptr;
    UPROPERTY()
    bool bSmartLinkIsRelevant = false;

    explicit ANavLinkProxy(const FObjectInitializer& ObjectInitializer);
};

}
