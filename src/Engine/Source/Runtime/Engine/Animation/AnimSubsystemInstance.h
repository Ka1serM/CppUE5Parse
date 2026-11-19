#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "AnimSubsystemInstance.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimSubsystemInstance {
    GENERATED_BODY()
    FAnimSubsystemInstance() = default;
    FAnimSubsystemInstance(const FAnimSubsystemInstance&) = default;
    FAnimSubsystemInstance& operator=(const FAnimSubsystemInstance&) = default;
    virtual ~FAnimSubsystemInstance() = default;
};

}
