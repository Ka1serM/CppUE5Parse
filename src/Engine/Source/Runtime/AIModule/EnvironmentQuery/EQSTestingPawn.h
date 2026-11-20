#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Character.h"

#include "EQSTestingPawn.generated.h"

namespace ue5parse::runtime {

class UEnvQuery;

UCLASS()
class AEQSTestingPawn : public ACharacter {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UEnvQuery> QueryTemplate = nullptr;

    explicit AEQSTestingPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
