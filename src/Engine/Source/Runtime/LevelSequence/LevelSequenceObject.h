#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LazyObjectPtr.h"

#include "LevelSequenceObject.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FLevelSequenceObject {
    GENERATED_BODY()

    UPROPERTY()
    TLazyObjectPtr<UObject> ObjectOrOwner;
    UPROPERTY()
    FString ComponentName;
};

}
