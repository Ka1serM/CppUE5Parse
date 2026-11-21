#pragma once

#include <variant>

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ConvexHalfEdgeStructureData.h"

namespace ue5parse::runtime::Chaos {

class FConvexStructureData {
public:
    enum class EIndexType : int8 { None, Small, Medium, Large };

    using FConvexHalfEdgeStructureDataS = TConvexHalfEdgeStructureData<uint8>;
    using FConvexHalfEdgeStructureDataM = TConvexHalfEdgeStructureData<int16>;
    using FConvexHalfEdgeStructureDataL = TConvexHalfEdgeStructureData<int32>;
    using FData = std::variant<std::monostate, FConvexHalfEdgeStructureDataS, FConvexHalfEdgeStructureDataM, FConvexHalfEdgeStructureDataL>;

    void Serialize(FArchive& Ar);

    EIndexType GetIndexType() const { return IndexType; }
    const FData& GetData() const { return Data; }

private:
    EIndexType IndexType = EIndexType::None;
    FData Data;
};

}
