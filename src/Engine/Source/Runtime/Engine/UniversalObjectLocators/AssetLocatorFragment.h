#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "AssetLocatorFragment.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAssetLocatorFragment {
    GENERATED_BODY()

    UPROPERTY()
    FTopLevelAssetPath Path;
};

}
