#pragma once

#include <variant>

#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Box.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Capsule.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Convex.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObjectScaled.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObjectTransformed.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObjectType.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObjectUnion.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Plane.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Sphere.h"

namespace ue5parse::runtime::Chaos {

class FImplicitObject {
public:
    using FGeometry = std::variant<std::monostate, FSphere, FImplicitBox3, FImplicitPlane3, FCapsule, FImplicitObjectTransformed, FImplicitObjectUnion, FConvex,
        FImplicitObjectInstanced, FImplicitObjectScaled>;

    void Serialize(FChaosArchive& Ar);

    EImplicitObjectType GetType() const { return Type; }
    EImplicitObjectType GetCollisionType() const { return CollisionType; }
    bool IsConvex() const { return bIsConvex; }
    bool GetDoCollide() const { return bDoCollide; }
    const FGeometry& GetGeometry() const { return Geometry; }

private:
    void SelectGeometry(FChaosArchive& Ar);
    void SerializeImp(FArchive& Ar);

    EImplicitObjectType Type = ImplicitObjectType::Unknown;
    EImplicitObjectType CollisionType = ImplicitObjectType::Unknown;
    bool bIsConvex = false;
    bool bDoCollide = true;
    FGeometry Geometry;
};

}
