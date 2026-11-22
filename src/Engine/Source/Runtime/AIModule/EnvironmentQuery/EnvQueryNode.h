#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "EnvQueryNode.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UEnvQueryNode : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 VerNum = 0;
};

}
