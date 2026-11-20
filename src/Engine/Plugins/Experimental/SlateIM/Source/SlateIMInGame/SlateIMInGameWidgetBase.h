#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "SlateIMInGameWidgetBase.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ASlateIMInGameWidgetBase : public AActor {
    GENERATED_BODY()
public:
    explicit ASlateIMInGameWidgetBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
