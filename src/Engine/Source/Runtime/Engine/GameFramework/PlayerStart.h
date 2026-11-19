#pragma once

#include "Engine/Source/Runtime/Engine/Engine/NavigationObjectBase.h"

#include "PlayerStart.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APlayerStart : public ANavigationObjectBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName PlayerStartTag;

    explicit APlayerStart(const FObjectInitializer& ObjectInitializer);
};

}
