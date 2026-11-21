#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Vector.h"

namespace ue5parse::runtime::Chaos {

template <class T>
struct TPlaneConcrete {
    TVector<T> X;
    TVector<T> Normal;

    friend FArchive& operator<<(FArchive& Ar, TPlaneConcrete& Plane) {
        SerializeReal(Ar, Plane.X);
        return SerializeReal(Ar, Plane.Normal);
    }
};

using FPlaneConcrete3f = TPlaneConcrete<float>;

class FImplicitPlane3 {
public:
    void Serialize(FArchive& Ar) { Ar << PlaneConcrete; }

    const TPlaneConcrete<double>& GetPlane() const { return PlaneConcrete; }

private:
    TPlaneConcrete<double> PlaneConcrete;
};

}
