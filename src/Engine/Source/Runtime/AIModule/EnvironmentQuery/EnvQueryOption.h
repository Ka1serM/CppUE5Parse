#pragma once

#include "Engine/Source/Runtime/AIModule/EnvironmentQuery/EnvQueryGenerator.h"
#include "Engine/Source/Runtime/AIModule/EnvironmentQuery/EnvQueryTest.h"

#include "EnvQueryOption.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UEnvQueryOption : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UEnvQueryGenerator> Generator = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UEnvQueryTest>> Tests;
};

}
