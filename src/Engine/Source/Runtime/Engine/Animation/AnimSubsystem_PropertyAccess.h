#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimSubsystem.h"
#include "Engine/Source/Runtime/Engine/Animation/PropertyAccess.h"

#include "AnimSubsystem_PropertyAccess.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimSubsystem_PropertyAccess : public FAnimSubsystem {
    GENERATED_BODY()
    const FPropertyAccessLibrary& GetLibrary() const { return Library; }
    FPropertyAccessLibrary& GetLibrary() { return Library; }

private:
    UPROPERTY()
    FPropertyAccessLibrary Library;
};

}
