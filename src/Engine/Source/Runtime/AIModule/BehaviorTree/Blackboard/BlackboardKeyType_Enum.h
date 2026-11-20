#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "BlackboardKeyType_Enum.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_Enum : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UEnum> EnumType = nullptr;
    UPROPERTY()
    FString EnumName;
    UPROPERTY()
    uint32 bIsEnumNameValid : 1 = false;
    UPROPERTY()
    uint8 DefaultValue = 0;
};

}
