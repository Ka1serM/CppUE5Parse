#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "PropertyBindingBindableStructDescriptor.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPropertyBindingBindableStructDescriptor {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UStruct> Struct = nullptr;
    UPROPERTY()
    FName Name;
};

}
