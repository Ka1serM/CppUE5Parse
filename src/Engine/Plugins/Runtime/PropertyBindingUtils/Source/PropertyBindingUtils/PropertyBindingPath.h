#pragma once

#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingBindableStructDescriptor.h"
#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingTypes.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

#include "PropertyBindingPath.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPropertyBindingPathSegment {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    int32 ArrayIndex = INDEX_NONE;
    UPROPERTY()
    TObjectPtr<UStruct> InstanceStruct = nullptr;
    UPROPERTY()
    EPropertyBindingPropertyAccessType InstancedStructAccessType = EPropertyBindingPropertyAccessType::Unset;
};

USTRUCT()
struct FPropertyBindingPath {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPropertyBindingPathSegment> Segments;
};

USTRUCT()
struct FPropertyBindingCopyInfoBatch {
    GENERATED_BODY()

    UPROPERTY()
    TInstancedStruct<FPropertyBindingBindableStructDescriptor> TargetStruct;
    UPROPERTY()
    FPropertyBindingIndex16 BindingsBegin;
    UPROPERTY()
    FPropertyBindingIndex16 BindingsEnd;
    UPROPERTY()
    FPropertyBindingIndex16 PropertyFunctionsBegin;
    UPROPERTY()
    FPropertyBindingIndex16 PropertyFunctionsEnd;
};

}
