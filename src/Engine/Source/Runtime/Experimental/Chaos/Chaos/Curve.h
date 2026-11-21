#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime::Chaos {

struct FCurveKey {
    float Time = 0.0f;
    float Value = 0.0f;

    friend FArchive& operator<<(FArchive& Ar, FCurveKey& Key) { return Ar << Key.Time << Key.Value; }
};

class FLinearCurve {
public:
    friend FArchive& operator<<(FArchive& Ar, FLinearCurve& Curve) { return Ar << Curve.Keys; }

    const TArray<FCurveKey>& GetKeys() const { return Keys; }

private:
    TArray<FCurveKey> Keys;
};

}
