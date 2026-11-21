#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyPose.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"

namespace ue5parse::runtime {

namespace {

enum class EScaleRepresentation : uint8 { One, Uniform, NonUniform };

void LoadCompactTransform(FArchive& Ar, FTransform& Transform) {
    constexpr uint8 PositionIdentityOne = 6;
    constexpr uint8 ZeroQuatOne = 3;
    constexpr uint8 DoubleComplete = 12;
    uint8 State = 0;
    Ar << State;
    if (State == DoubleComplete) {
        Ar << Transform;
        return;
    }
    if (State > DoubleComplete) {
        Ar.SetError(ErrorCode::InvalidData, "FRigCompactTransform.State");
        return;
    }
    Transform = FTransform{};
    if (State >= PositionIdentityOne) {
        FVector3f Position;
        Ar << Position;
        Transform.Translation = FVector(Position);
    }
    if (State % PositionIdentityOne >= ZeroQuatOne) {
        FQuat4f Rotation;
        Ar << Rotation;
        Transform.Rotation = FQuat(Rotation);
    }
    switch (static_cast<EScaleRepresentation>(State % ZeroQuatOne)) {
    case EScaleRepresentation::One:
        break;
    case EScaleRepresentation::Uniform: {
        float Scale = 1;
        Ar << Scale;
        Transform.Scale3D = {Scale, Scale, Scale};
        break;
    }
    case EScaleRepresentation::NonUniform: {
        FVector3f Scale;
        Ar << Scale;
        Transform.Scale3D = FVector(Scale);
        break;
    }
    }
}

}

void FRigComputedTransform::Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings) {
    if (Settings.ControlRigVersion < FControlRigObjectVersion::RigHierarchyCompactTransformSerialization) {
        Ar << Transform << bDirty;
        return;
    }
    Ar << bDirty;
    if (!bDirty)
        LoadCompactTransform(Ar, Transform);
}

void FRigLocalAndGlobalTransform::Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings) {
    if (Settings.bSerializeLocalTransform)
        Local.Load(Ar, Settings);
    else
        Local.bDirty = true;
    if (Settings.bSerializeGlobalTransform)
        Global.Load(Ar, Settings);
    else
        Global.bDirty = true;
}

void FRigCurrentAndInitialTransform::Load(FArchive& Ar, const FRigHierarchySerializationSettings& Settings) {
    if (Settings.bSerializeCurrentTransform)
        Current.Load(Ar, Settings);
    if (Settings.bSerializeInitialTransform) {
        Initial.Load(Ar, Settings);
        if (!Settings.bSerializeCurrentTransform)
            Current = Initial;
    }
}

}
