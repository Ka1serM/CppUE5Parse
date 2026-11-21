#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ChaosArchive.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitFwd.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Vector.h"

namespace ue5parse::runtime::Chaos {

struct FSegment {
    FVector Point;
    FVector Axis;
    float Length = 0.0f;

    friend FArchive& operator<<(FArchive& Ar, FSegment& Segment) {
        SerializeReal(Ar, Segment.Point);
        return SerializeReal(Ar, Segment.Axis) << Segment.Length;
    }
};

class FCapsule {
public:
    void Serialize(FChaosArchive& Ar);

    const FSegment& GetSegment() const { return Segment; }
    float GetRadius() const { return Radius; }

private:
    FSegment Segment;
    float Radius = 0.0f;
};

}
