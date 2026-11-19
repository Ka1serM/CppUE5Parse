#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "SkeletalMeshVertexAttribute.generated.h"

namespace ue5parse::runtime {

//! Valorant stores it in an int32; the uint8 of the other games binds into the low byte.
UENUM()
enum class ESkeletalMeshVertexAttributeDataType : int32 { Float, HalfFloat, NUInt8 };

USTRUCT()
struct FSkeletalMeshVertexAttributeInfo {
    GENERATED_BODY()
    UPROPERTY()
    FPerPlatformBool EnabledForRender;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    ESkeletalMeshVertexAttributeDataType DataType = ESkeletalMeshVertexAttributeDataType::Float;
};

}
