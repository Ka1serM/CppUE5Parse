#pragma once

#include "Engine/Source/Runtime/AIModule/EnvironmentQuery/EnvQueryNode.h"

#include "EnvQueryGenerator.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UEnvQueryGenerator : public UEnvQueryNode {
    GENERATED_BODY()
};

}
