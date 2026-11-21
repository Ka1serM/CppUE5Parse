#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Vector.h"

namespace ue5parse::runtime::Chaos {

class FSphere {
public:
    void Serialize(FArchive& Ar) { SerializeReal(Ar, Center) << Radius; }

    const FVector& GetCenter() const { return Center; }
    float GetRadius() const { return Radius; }

private:
    FVector Center;
    float Radius = 0.0f;
};

}
