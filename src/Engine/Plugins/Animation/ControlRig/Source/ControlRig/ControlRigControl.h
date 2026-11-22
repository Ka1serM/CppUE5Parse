#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/Math/Transform.h"

#include "ControlRigControl.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class AControlRigControl : public AActor {
    GENERATED_BODY()
public:
    explicit AControlRigControl(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FString PropertyPath;
    UPROPERTY()
    FTransform Transform;
    UPROPERTY()
    bool bEnabled = false;
    UPROPERTY()
    bool bSelected = false;
    UPROPERTY()
    bool bHovered = false;
    UPROPERTY()
    bool bManipulating = false;
};

}
