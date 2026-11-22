#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "StaticMeshComponent_TA.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UStaticMeshComponent_TA : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UStaticMeshComponent_TA(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    bool bIgnoreScriptAttach = false;
};

}
