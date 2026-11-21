#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"

namespace ue5parse::runtime::Chaos {

using EImplicitObjectType = uint8;

namespace ImplicitObjectType {

enum : EImplicitObjectType {
    Sphere = 0,
    Box,
    Plane,
    Capsule,
    Transformed,
    Union,
    LevelSet,
    Unknown,
    Convex,
    TaperedCylinder,
    Cylinder,
    TriangleMesh,
    HeightField,
    DEPRECATED_Scaled,
    Triangle,
    UnionClustered,
    TaperedCapsule,
    IsWeightedLattice = 1 << 5,
    IsInstanced = 1 << 6,
    IsScaled = 1 << 7
};

}

inline bool IsInstanced(EImplicitObjectType Type) {
    return (Type & ImplicitObjectType::IsInstanced) != 0;
}

inline bool IsScaled(EImplicitObjectType Type) {
    return (Type & ImplicitObjectType::IsScaled) != 0;
}

inline EImplicitObjectType GetInnerType(EImplicitObjectType Type) {
    return Type & ~(ImplicitObjectType::IsWeightedLattice | ImplicitObjectType::IsScaled | ImplicitObjectType::IsInstanced);
}

}
