#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "BlackboardKeyType_Class.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Class : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UClass> BaseClass = nullptr;
    UPROPERTY()
    TObjectPtr<UClass> DefaultValue = nullptr;
};

}
