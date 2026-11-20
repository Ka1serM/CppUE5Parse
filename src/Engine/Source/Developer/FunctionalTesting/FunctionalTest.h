#pragma once

#include "Engine/Source/Runtime/Core/Math/RandomStream.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "FunctionalTest.generated.h"

namespace ue5parse::runtime {

class UBillboardComponent;

UCLASS()
class AFunctionalTest : public AActor {
    GENERATED_BODY()
public:
    explicit AFunctionalTest(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FString TestLabel;
    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
    UPROPERTY()
    FRandomStream RandomNumbersStream;
};

}
