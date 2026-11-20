#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "BlackboardKeyType_Object.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Object : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UClass> BaseClass = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> DefaultValue = nullptr;
};

}
