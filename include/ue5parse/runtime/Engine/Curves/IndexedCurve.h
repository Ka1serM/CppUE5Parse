#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/Engine/Curves/KeyHandle.h"

#include "IndexedCurve.generated.h"

namespace ue5parse::runtime {

// FIndexedCurve: the base of curves with keys addressed by index (UE's Curves/IndexedCurve.h).
USTRUCT()
struct FIndexedCurve {
    GENERATED_BODY()
    UPROPERTY()
    FKeyHandleMap KeyHandlesToIndices;

    virtual ~FIndexedCurve() = default;
    virtual int32 GetNumKeys() const { return 0; }
    bool IsKeyHandleValid(int32 KeyIndex) const { return KeyIndex >= 0 && KeyIndex < GetNumKeys(); }
};

}
