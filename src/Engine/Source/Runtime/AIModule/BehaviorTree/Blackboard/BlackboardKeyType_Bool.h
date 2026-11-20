#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_Bool.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Bool : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bDefaultValue = false;
};

}
