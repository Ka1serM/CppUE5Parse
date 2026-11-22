#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_Int.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Int : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 DefaultValue = 0;
};

}
