#pragma once

#include "Engine/Source/Runtime/AIModule/EnvironmentQuery/EnvQueryOption.h"
#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"

#include "EnvQuery.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UEnvQuery : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName QueryName;
    UPROPERTY()
    TArray<TObjectPtr<UEnvQueryOption>> Options;
    UPROPERTY()
    uint8 bStripFromClientBuilds : 1 = false;
};

}
