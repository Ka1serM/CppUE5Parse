#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "SceneTypes.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EShadowCacheInvalidationBehavior : uint8 { Auto, Always, Rigid, Static };

USTRUCT()
struct FCustomPrimitiveData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<float> Data;
};

}
