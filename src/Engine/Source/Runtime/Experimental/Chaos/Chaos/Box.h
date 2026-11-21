#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"

namespace ue5parse::runtime::Chaos {

class FImplicitBox3 {
public:
    void Serialize(FArchive& Ar);

    const FAABB3& GetAABB() const { return AABB; }
    float GetMargin() const { return Margin; }

private:
    FAABB3 AABB;
    float Margin = 0.0f;
};

}
