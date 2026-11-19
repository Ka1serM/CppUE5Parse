#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "AnimSubsystem.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimSubsystem {
    GENERATED_BODY()
    FAnimSubsystem() = default;
    FAnimSubsystem(const FAnimSubsystem&) = default;
    FAnimSubsystem& operator=(const FAnimSubsystem&) = default;
    virtual ~FAnimSubsystem() = default;

    virtual void OnPostLoadDefaults(UClass* InClass) {}
};

}
