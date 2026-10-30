#pragma once

#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "AnimInstance.generated.h"

namespace ue5parse::runtime {

class USkeleton;

namespace ERootMotionMode {
enum Type : int32 { NoRootMotionExtraction, IgnoreRootMotion, RootMotionFromEverything, RootMotionFromMontagesOnly };
}

// UE's animation instance. A cooked animation Blueprint derives from this class and stores its node structs in its
// reflected properties. The graph runtime reads those properties through the class, not through a second object model.
UCLASS()
class UAnimInstance : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    USkeleton* CurrentSkeleton = nullptr;
    UPROPERTY()
    TEnumAsByte<ERootMotionMode::Type> RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;

    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintInitializeAnimation();
    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintUpdateAnimation(float DeltaTimeX);
    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintPostEvaluateAnimation();
    UFUNCTION(BlueprintImplementableEvent)
    void BlueprintBeginPlay();
};

}
