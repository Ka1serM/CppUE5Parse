#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_String.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_String : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString StringValue;
    UPROPERTY()
    FString DefaultValue;
};

}
