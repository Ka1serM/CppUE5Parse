#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "LODSyncComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULODSyncComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit ULODSyncComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
