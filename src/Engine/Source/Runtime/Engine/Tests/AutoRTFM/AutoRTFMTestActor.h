#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "AutoRTFMTestActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AAutoRTFMTestActor : public AActor {
    GENERATED_BODY()
public:
    explicit AAutoRTFMTestActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    int32 MyProperty = 0;
};

}
