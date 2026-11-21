#pragma once

#include "Engine/Source/Runtime/UniversalObjectLocator/UniversalObjectLocatorFragment.h"

#include "UniversalObjectLocator.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FUniversalObjectLocator {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FUniversalObjectLocatorFragment> Fragments;
};

}
