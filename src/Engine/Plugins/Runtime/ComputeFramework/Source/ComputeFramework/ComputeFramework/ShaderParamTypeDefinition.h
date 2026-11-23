#pragma once

#include <memory>

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ShaderParamTypeDefinition.generated.h"

namespace ue5parse::runtime {

struct FShaderValueType;

UENUM()
enum class EShaderFundamentalType : uint8 {
    Bool = 0,
    Int = 1,
    Uint = 2,
    Float = 3,
    Struct = 4,
    None = 255u,
};

UENUM()
enum class EShaderFundamentalDimensionType : uint8 {
    Scalar,
    Vector,
    Matrix,
};

//! UE interns each value type for the process; the port lets each handle own its type, since a name of a type can point into its package.
USTRUCT()
struct FShaderValueTypeHandle {
    GENERATED_BODY()

    std::shared_ptr<const FShaderValueType> ValueTypePtr;

    bool IsValid() const { return ValueTypePtr != nullptr; }
    bool operator==(const FShaderValueTypeHandle& InOther) const;
    bool Serialize(FArchive& Ar);
};

FArchive& operator<<(FArchive& InArchive, FShaderValueTypeHandle& InHandle);

template <>
struct TStructOpsTypeTraits<FShaderValueTypeHandle> : TStructOpsTypeTraitsBase2<FShaderValueTypeHandle> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true, WithCopy = false };
};

USTRUCT()
struct FShaderValueType {
    GENERATED_BODY()

    struct FStructElement {
        bool operator==(const FStructElement& InOther) const { return Name == InOther.Name && Type == InOther.Type; }

        FName Name;
        FShaderValueTypeHandle Type;
    };

    bool operator==(const FShaderValueType& InOtherType) const;

    UPROPERTY()
    EShaderFundamentalType Type = EShaderFundamentalType::Bool;
    UPROPERTY()
    EShaderFundamentalDimensionType DimensionType = EShaderFundamentalDimensionType::Scalar;
    uint8 VectorElemCount = 0;
    uint8 MatrixRowCount = 0;
    uint8 MatrixColumnCount = 0;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    bool bIsDynamicArray = false;
    TArray<FStructElement> StructElements;
};

}
