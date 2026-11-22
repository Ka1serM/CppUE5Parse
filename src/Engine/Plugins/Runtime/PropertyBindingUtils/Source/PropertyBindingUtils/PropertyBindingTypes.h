#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "PropertyBindingTypes.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EPropertyBindingPropertyAccessType : uint8 { Offset, Object, WeakObject, SoftObject, ObjectInstance, StructInstance, IndexArray, SharedStruct, StructInstanceContainer, Unset };

USTRUCT()
struct FPropertyBindingIndex16 {
    GENERATED_BODY()

    static constexpr uint16 InvalidValue = 0xFFFF;

    UPROPERTY()
    uint16 Value = InvalidValue;

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FPropertyBindingIndex16> : TStructOpsTypeTraitsBase2<FPropertyBindingIndex16> {
    enum { WithSerializeFromMismatchedTag = true };
};

}
