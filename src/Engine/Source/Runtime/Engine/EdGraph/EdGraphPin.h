#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "EdGraphPin.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSimpleMemberReference {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> MemberParent = nullptr;
    UPROPERTY()
    FName MemberName;
    UPROPERTY()
    FGuid MemberGuid;
};

}
