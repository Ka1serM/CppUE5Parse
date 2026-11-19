#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "SkeletalMeshVertexAttribute.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ESkeletalMeshVertexAttributeDataType : uint8 { Float, HalfFloat, NUInt8 };

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
