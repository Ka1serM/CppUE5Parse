#include "Engine/Plugins/Runtime/ComputeFramework/Source/ComputeFramework/ComputeFramework/ShaderParamTypeDefinition.h"

#include <utility>

#include "Engine/Plugins/Runtime/ComputeFramework/Source/ComputeFramework/ComputeFramework/ComputeFrameworkObjectVersion.h"

namespace ue5parse::runtime {

namespace {

constexpr int32 MaxStructDepth = 64;

void SerializeValueType(FArchive& InArchive, FShaderValueTypeHandle& InHandle, int32 Depth) {
    if (Depth > MaxStructDepth) {
        InArchive.SetError(ErrorCode::InvalidData, "FShaderValueType.Depth");
        return;
    }
    FShaderValueType ValueTypeTemp;
    InArchive << ValueTypeTemp.Type;
    if (InArchive.CustomVerAtLeast(FComputeFrameworkObjectVersion::GUID, FComputeFrameworkObjectVersion::InitialVersion, EGame::GAME_UE5_1))
        InArchive << ValueTypeTemp.bIsDynamicArray;
    if (ValueTypeTemp.Type == EShaderFundamentalType::Struct) {
        InArchive << ValueTypeTemp.Name;
        const int32 Num = InArchive.ReadCount();
        ValueTypeTemp.StructElements.SetNum(Num);
        for (FShaderValueType::FStructElement& Element : ValueTypeTemp.StructElements) {
            InArchive << Element.Name;
            SerializeValueType(InArchive, Element.Type, Depth + 1);
            if (InArchive.IsError())
                return;
        }
    } else {
        InArchive << ValueTypeTemp.DimensionType;
        if (ValueTypeTemp.DimensionType == EShaderFundamentalDimensionType::Vector)
            InArchive << ValueTypeTemp.VectorElemCount;
        else if (ValueTypeTemp.DimensionType == EShaderFundamentalDimensionType::Matrix)
            InArchive << ValueTypeTemp.MatrixRowCount << ValueTypeTemp.MatrixColumnCount;
    }
    if (InArchive.IsError())
        return;
    InHandle.ValueTypePtr = std::make_shared<const FShaderValueType>(std::move(ValueTypeTemp));
}

}

bool FShaderValueTypeHandle::operator==(const FShaderValueTypeHandle& InOther) const {
    if (ValueTypePtr == nullptr || InOther.ValueTypePtr == nullptr)
        return ValueTypePtr == InOther.ValueTypePtr;
    return *ValueTypePtr == *InOther.ValueTypePtr;
}

bool FShaderValueTypeHandle::Serialize(FArchive& Ar) {
    Ar << *this;
    return true;
}

FArchive& operator<<(FArchive& InArchive, FShaderValueTypeHandle& InHandle) {
    SerializeValueType(InArchive, InHandle, 0);
    return InArchive;
}

bool FShaderValueType::operator==(const FShaderValueType& InOtherType) const {
    if (Type != InOtherType.Type || bIsDynamicArray != InOtherType.bIsDynamicArray)
        return false;
    if (Type == EShaderFundamentalType::Struct)
        return Name == InOtherType.Name && StructElements == InOtherType.StructElements;
    if (DimensionType != InOtherType.DimensionType)
        return false;
    if (DimensionType == EShaderFundamentalDimensionType::Vector)
        return VectorElemCount == InOtherType.VectorElemCount;
    if (DimensionType == EShaderFundamentalDimensionType::Matrix)
        return MatrixRowCount == InOtherType.MatrixRowCount && MatrixColumnCount == InOtherType.MatrixColumnCount;
    return true;
}

}
