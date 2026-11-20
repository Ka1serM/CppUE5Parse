#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_Float.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Float : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    float DefaultValue = 0.0f;
};

}
