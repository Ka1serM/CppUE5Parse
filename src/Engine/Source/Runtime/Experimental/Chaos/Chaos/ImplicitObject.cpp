#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObject.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime::Chaos {

void FImplicitObject::Serialize(FChaosArchive& Ar) {
    int8 ObjectType = 0;
    Ar << ObjectType;
    Type = static_cast<EImplicitObjectType>(ObjectType);
    SelectGeometry(Ar);
    if (Ar.IsError())
        return;
    SerializeImp(Ar);
    std::visit(
        [&Ar](auto& Shape) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(Shape)>, std::monostate>)
                Shape.Serialize(Ar);
        },
        Geometry);
}

void FImplicitObject::SelectGeometry(FChaosArchive& Ar) {
    const EImplicitObjectType InnerType = GetInnerType(Type);
    const bool bScaledIsConcrete =
        Ar.CustomVerAtLeast(FExternalPhysicsCustomObjectVersion::GUID, FExternalPhysicsCustomObjectVersion::ScaledGeometryIsConcrete, EGame::GAME_UE4_25);
    if (bScaledIsConcrete && IsScaled(Type)) {
        if (InnerType != ImplicitObjectType::Convex) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "FImplicitObject.Scaled");
            return;
        }
        Geometry.emplace<FImplicitObjectScaled>();
        return;
    }
    if (IsInstanced(Type)) {
        if (InnerType != ImplicitObjectType::Convex) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "FImplicitObject.Instanced");
            return;
        }
        Geometry.emplace<FImplicitObjectInstanced>();
        return;
    }
    switch (Type) {
    case ImplicitObjectType::Sphere:
        Geometry.emplace<FSphere>();
        return;
    case ImplicitObjectType::Box:
        Geometry.emplace<FImplicitBox3>();
        return;
    case ImplicitObjectType::Plane:
        Geometry.emplace<FImplicitPlane3>();
        return;
    case ImplicitObjectType::Capsule:
        Geometry.emplace<FCapsule>();
        return;
    case ImplicitObjectType::Transformed:
        Geometry.emplace<FImplicitObjectTransformed>();
        return;
    case ImplicitObjectType::Union:
        Geometry.emplace<FImplicitObjectUnion>();
        return;
    case ImplicitObjectType::Convex:
        Geometry.emplace<FConvex>();
        return;
    default:
        Ar.SetError(ErrorCode::UnsupportedVersion, "FImplicitObject.Type");
    }
}

void FImplicitObject::SerializeImp(FArchive& Ar) {
    if (Ar.CustomVerAtLeast(FDestructionObjectVersion::GUID, FDestructionObjectVersion::ChaosArchiveAdded, EGame::GAME_UE4_23))
        Ar << bIsConvex << bDoCollide;
    //! UE resets bDoCollide up to FDestructionObjectVersion::ImplicitObjectDoCollideAttribute, which is still the latest version.
    bDoCollide = true;
    if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::FFieldPathOwnerSerialization, EGame::GAME_UE4_25))
        Ar << CollisionType;
    else
        CollisionType = Type;
}

}
