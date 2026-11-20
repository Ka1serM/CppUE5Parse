#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "Engine/Source/Runtime/Core/Math/Rotator.h"

#include "BlackboardKeyType_Rotator.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Rotator : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bUseDefaultValue = false;
    UPROPERTY()
    FRotator DefaultValue = FRotator::ZeroRotator;
};

}
