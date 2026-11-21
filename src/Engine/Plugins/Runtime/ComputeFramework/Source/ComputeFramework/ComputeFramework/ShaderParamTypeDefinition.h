#pragma once

#include <memory>

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "ShaderParamTypeDefinition.generated.h"

namespace ue5parse::runtime {

enum class EShaderFundamentalType : uint8 { Bool, Int, Uint, Float, Struct, None = 255 };
enum class EShaderFundamentalDimensionType : uint8 { Scalar, Vector, Matrix };

struct FShaderValueType;

USTRUCT()
struct FShaderValueTypeHandle {
    GENERATED_BODY()
    std::shared_ptr<const FShaderValueType> ValueType;

    bool Serialize(FArchive& Ar);
};

struct FShaderValueType {
    struct FStructElement {
        FName Name;
        FShaderValueTypeHandle Type;
    };

    EShaderFundamentalType Type = EShaderFundamentalType::None;
    EShaderFundamentalDimensionType DimensionType = EShaderFundamentalDimensionType::Scalar;
    bool bIsDynamicArray = false;
    uint8 VectorElemCount = 0;
    uint8 MatrixRowCount = 0;
    uint8 MatrixColumnCount = 0;
    FName Name;
    TArray<FStructElement> StructElements;
};

template <>
struct TStructOpsTypeTraits<FShaderValueTypeHandle> : TStructOpsTypeTraitsBase2<FShaderValueTypeHandle> {
    enum { WithSerializer = true };
};

}
