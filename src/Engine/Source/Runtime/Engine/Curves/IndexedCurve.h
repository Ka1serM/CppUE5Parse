#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/Engine/Curves/KeyHandle.h"

#include "IndexedCurve.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FIndexedCurve {
    GENERATED_BODY()
    UPROPERTY()
    FKeyHandleMap KeyHandlesToIndices;

    virtual ~FIndexedCurve() = default;
    virtual int32 GetNumKeys() const { return 0; }
};

}
