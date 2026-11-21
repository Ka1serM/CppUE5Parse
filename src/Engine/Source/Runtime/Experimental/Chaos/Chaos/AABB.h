#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Vector.h"

namespace ue5parse::runtime::Chaos {

template <class T>
struct TAABB {
    TVector<T> Min;
    TVector<T> Max;

    void Serialize(FArchive& Ar) {
        SerializeReal(Ar, Min);
        SerializeReal(Ar, Max);
    }

    friend FArchive& operator<<(FArchive& Ar, TAABB& AABB) {
        AABB.Serialize(Ar);
        return Ar;
    }
};

using FAABB3 = TAABB<double>;
using FAABB3f = TAABB<float>;

template <class T>
void SerializeAsAABB(FArchive& Ar, TAABB<T>& AABB);

}
