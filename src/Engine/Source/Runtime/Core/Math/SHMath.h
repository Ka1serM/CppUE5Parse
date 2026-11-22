#pragma once

#include <algorithm>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

template <int32 MaxSHOrder>
class TSHVector {
public:
    static constexpr int32 MaxSHBasis = MaxSHOrder * MaxSHOrder;

    float V[MaxSHBasis] = {};

    TSHVector() = default;
    template <int32 OtherOrder>
    explicit TSHVector(const TSHVector<OtherOrder>& Other) {
        std::copy_n(Other.V, std::min(MaxSHBasis, TSHVector<OtherOrder>::MaxSHBasis), V);
    }

    friend FArchive& operator<<(FArchive& Ar, TSHVector& SH) {
        for (float& Basis : SH.V)
            Ar << Basis;
        return Ar;
    }
};

template <int32 MaxSHOrder>
class TSHVectorRGB {
public:
    TSHVector<MaxSHOrder> R;
    TSHVector<MaxSHOrder> G;
    TSHVector<MaxSHOrder> B;

    TSHVectorRGB() = default;
    template <int32 OtherOrder>
    explicit TSHVectorRGB(const TSHVectorRGB<OtherOrder>& Other) : R(Other.R), G(Other.G), B(Other.B) {}

    friend FArchive& operator<<(FArchive& Ar, TSHVectorRGB& SH) { return Ar << SH.R << SH.G << SH.B; }
};

}
