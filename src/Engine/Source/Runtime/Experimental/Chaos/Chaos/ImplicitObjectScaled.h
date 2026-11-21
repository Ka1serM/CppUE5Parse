#pragma once

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/AABB.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ChaosArchive.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitFwd.h"

namespace ue5parse::runtime::Chaos {

class FImplicitObjectInstanced {
public:
    void Serialize(FChaosArchive& Ar) { Ar << Object; }

    const FImplicitObjectPtr& GetObject() const { return Object; }

private:
    FImplicitObjectPtr Object;
};

class FImplicitObjectScaled {
public:
    void Serialize(FChaosArchive& Ar);

    const FImplicitObjectPtr& GetObject() const { return Object; }
    const FVector& GetScale() const { return Scale; }
    const FVector& GetInvScale() const { return InvScale; }
    const FAABB3& GetLocalBoundingBox() const { return LocalBoundingBox; }

private:
    FImplicitObjectPtr Object;
    FVector Scale;
    FVector InvScale;
    FAABB3 LocalBoundingBox;
};

}
