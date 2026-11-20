#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "SlateBrush.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSlateBrush {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> ResourceObject = nullptr;
};

}
