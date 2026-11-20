#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

#include "BlackboardKeyType_Struct.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Struct : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    FInstancedStruct DefaultValue;
    UPROPERTY()
    FInstancedStruct Value;
};

}
