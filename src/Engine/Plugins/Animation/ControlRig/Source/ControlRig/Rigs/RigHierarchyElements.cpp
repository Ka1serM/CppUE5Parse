#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyElements.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"

namespace ue5parse::runtime {

using Version = FControlRigObjectVersion;

void FRigConnectorSettings::Load(FArchive& Ar) {
    Ar << Description;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::ConnectorsWithType, EGame::GAME_UE5_4))
        Ar << Type << bOptional;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyArrayConnectors, EGame::GAME_UE5_6))
        Ar << bIsArray;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyPostConstructionConnectors, EGame::GAME_UE5_7))
        Ar << bPostConstruction;
    Ar << Rules;
}

void FRigHierarchyElement::LoadStatic(FArchive& Ar, const FRigHierarchySerializationSettings& Settings) {
    FRigElementKey Stored;
    Ar << Stored;
    if (Stored.Type != Key.Type) {
        Ar.SetError(ErrorCode::InvalidData, "FRigHierarchyElement.Type");
        return;
    }
    Key = Stored;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::HierarchyElementMetadata, EGame::GAME_UE5_1)
        && !Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyStoresElementMetadata, EGame::GAME_UE5_4))
        Ar << Metadata;
    auto Type = Key.Type.getText();
    if (Type.starts_with("ERigElementType::"))
        Type.remove_prefix(sizeof("ERigElementType::") - 1);
    if (Type == "Bone") {
        auto& Bone = Data.emplace<FRigBoneElement>();
        Bone.Pose.Load(Ar, Settings);
        Ar << Bone.BoneType;
    } else if (Type == "RigidBody" || Type == "Physics") {
        auto& Body = Data.emplace<FRigRigidBodyElement>();
        Body.Pose.Load(Ar, Settings);
        if (Type == "Physics")
            Ar << Body.Solver;
        Ar << Body.Mass;
    } else if (Type == "Reference" || Type == "Socket")
        Data.emplace<FRigSingleParentElement>().Pose.Load(Ar, Settings);
    else if (Type == "Curve") {
        auto& Curve = Data.emplace<FRigCurveElement>();
        if (Ar.CustomVerAtLeast(Version::GUID, Version::CurveElementValueStateFlag, EGame::GAME_UE5_1))
            Ar << Curve.bIsValueSet;
        Ar << Curve.Value;
    } else if (Type == "Connector")
        Data.emplace<FRigConnectorSettings>().Load(Ar);
    else if (Type == "Null" || Type == "Space")
        Data.emplace<FRigMultiParentElement>().Load(Ar, Settings);
    else if (Type == "Control")
        Data.emplace<FRigControlElement>().Load(Ar, Settings);
    else
        Ar.SetError(ErrorCode::InvalidData, "FRigHierarchyElement.UnsupportedType");
}

void FRigHierarchyElement::LoadParents(FArchive& Ar) {
    if (auto* Bone = std::get_if<FRigBoneElement>(&Data))
        Ar << Bone->Parent;
    else if (auto* SingleParent = std::get_if<FRigSingleParentElement>(&Data))
        Ar << SingleParent->Parent;
    else if (auto* MultiParent = std::get_if<FRigMultiParentElement>(&Data))
        MultiParent->LoadParents(Ar);
    else if (auto* Control = std::get_if<FRigControlElement>(&Data))
        Control->LoadParents(Ar);
    else if (auto* Body = std::get_if<FRigRigidBodyElement>(&Data))
        Ar << Body->Parent;
}

}
