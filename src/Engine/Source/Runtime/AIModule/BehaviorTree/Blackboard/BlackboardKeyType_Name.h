#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_Name.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Name : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName DefaultValue;
};

}
