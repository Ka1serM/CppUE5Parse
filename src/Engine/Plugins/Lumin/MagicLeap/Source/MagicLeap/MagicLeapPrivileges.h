#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "MagicLeapPrivileges.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMagicLeapPrivileges : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapPrivileges(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
