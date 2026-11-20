#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavigationTypes.h"
#include "Engine/Source/Runtime/NavigationSystem/NavRelevantComponent.h"

#include "NavLinkCustomComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNavLinkCustomComponent : public UNavRelevantComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    uint32 NavLinkUserId = 0;
    UPROPERTY()
    FNavLinkId CustomLinkId;
    UPROPERTY()
    FNavLinkAuxiliaryId AuxiliaryCustomLinkId;

    explicit UNavLinkCustomComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
