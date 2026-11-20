#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "HLODBuilder.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UHLODBuilderSettings : public UObject {
    GENERATED_BODY()
};

UCLASS(Abstract)
class UHLODBuilder : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UHLODBuilderSettings> HLODBuilderSettings = nullptr;
};

UCLASS()
class UNullHLODBuilder : public UHLODBuilder {
    GENERATED_BODY()
};

}
