#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "NavRelevantComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNavRelevantComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UNavRelevantComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
