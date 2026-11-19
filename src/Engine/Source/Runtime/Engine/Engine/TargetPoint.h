#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "TargetPoint.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ATargetPoint : public AActor {
    GENERATED_BODY()
public:
    explicit ATargetPoint(const FObjectInitializer& ObjectInitializer);
};

}
