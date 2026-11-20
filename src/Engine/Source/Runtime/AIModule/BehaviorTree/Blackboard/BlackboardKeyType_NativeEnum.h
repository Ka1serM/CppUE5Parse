#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "BlackboardKeyType_NativeEnum.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlackboardKeyType_NativeEnum : public UBlackboardKeyType {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString EnumName;
    UPROPERTY()
    TObjectPtr<UEnum> EnumType = nullptr;
    UPROPERTY()
    bool bIsEnumNameValid = false;
};

}
