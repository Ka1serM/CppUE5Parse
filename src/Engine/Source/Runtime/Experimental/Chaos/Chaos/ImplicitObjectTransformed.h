#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ChaosArchive.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitFwd.h"

namespace ue5parse::runtime::Chaos {

class FImplicitObjectTransformed {
public:
    void Serialize(FChaosArchive& Ar);

    const FImplicitObjectPtr& GetObject() const { return Object; }
    const FTransform& GetTransform() const { return Transform; }
    const FAABB3& GetLocalBoundingBox() const { return LocalBoundingBox; }

private:
    FImplicitObjectPtr Object;
    FTransform Transform;
    FAABB3 LocalBoundingBox;
};

}
