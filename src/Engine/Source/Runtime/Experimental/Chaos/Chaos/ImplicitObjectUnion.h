#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ChaosArchive.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitFwd.h"

namespace ue5parse::runtime::Chaos {

class FImplicitObjectUnion {
public:
    void Serialize(FChaosArchive& Ar);

    const TArray<FImplicitObjectPtr>& GetObjects() const { return Objects; }
    const FAABB3& GetLocalBoundingBox() const { return LocalBoundingBox; }
    int32 GetNumLeafObjects() const { return NumLeafObjects; }

private:
    static constexpr uint8 Flag_HasBVH = 1 << 1;

    TArray<FImplicitObjectPtr> Objects;
    FAABB3 LocalBoundingBox;
    uint8 Flags = 0;
    int32 NumLeafObjects = 0;
};

}
