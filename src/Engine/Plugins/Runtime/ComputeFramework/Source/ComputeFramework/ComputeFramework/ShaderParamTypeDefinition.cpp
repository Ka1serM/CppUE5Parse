#include "Engine/Plugins/Runtime/ComputeFramework/Source/ComputeFramework/ComputeFramework/ShaderParamTypeDefinition.h"

namespace ue5parse::runtime {

namespace {

void ReadShaderValueType(FArchive& Ar, FShaderValueTypeHandle& Handle, int32 Depth) {
    if (Depth > 64) {
        Ar.SetError(ErrorCode::InvalidData, "FShaderValueType.StructDepth");
        return;
    }
    constexpr FGuid ComputeVersion{0x6304A3E7, 0x00594F59, 0x8CFC21BD, 0x7721FD4E};
    FShaderValueType Value;
    Ar << Value.Type;
    if (Ar.CustomVerAtLeast(ComputeVersion, 0, EGame::GAME_UE5_1))
        Ar << Value.bIsDynamicArray;
    if (Value.Type == EShaderFundamentalType::Struct) {
        Ar << Value.Name;
        const int32 Count = Ar.ReadCount(9);
        Value.StructElements.SetNum(Count);
        for (auto& Element : Value.StructElements) {
            Ar << Element.Name;
            ReadShaderValueType(Ar, Element.Type, Depth + 1);
            if (Ar.IsError())
                return;
        }
    } else {
        Ar << Value.DimensionType;
        if (Value.DimensionType == EShaderFundamentalDimensionType::Vector)
            Ar << Value.VectorElemCount;
        else if (Value.DimensionType == EShaderFundamentalDimensionType::Matrix)
            Ar << Value.MatrixRowCount << Value.MatrixColumnCount;
    }
    if (!Ar.IsError())
        Handle.ValueType = std::make_shared<FShaderValueType>(std::move(Value));
}

}

bool FShaderValueTypeHandle::Serialize(FArchive& Ar) {
    ReadShaderValueType(Ar, *this, 0);
    return true;
}

}
