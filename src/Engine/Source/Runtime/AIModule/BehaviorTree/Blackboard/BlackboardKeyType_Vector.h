#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_Vector.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Vector : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bUseDefaultValue = false;
    UPROPERTY()
    FVector DefaultValue = FVector::ZeroVector;
};

}
