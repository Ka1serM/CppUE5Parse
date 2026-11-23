#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyMetadata.h"

namespace ue5parse::runtime {

FRigBaseMetadata* FRigBaseMetadata::MakeMetadata(const FName& InName, ERigMetadataType InType) {
    FRigBaseMetadata* Md = nullptr;
    switch (InType) {
    case ERigMetadataType::Bool:
        Md = new FRigBoolMetadata();
        break;
    case ERigMetadataType::BoolArray:
        Md = new FRigBoolArrayMetadata();
        break;
    case ERigMetadataType::Float:
        Md = new FRigFloatMetadata();
        break;
    case ERigMetadataType::FloatArray:
        Md = new FRigFloatArrayMetadata();
        break;
    case ERigMetadataType::Int32:
        Md = new FRigInt32Metadata();
        break;
    case ERigMetadataType::Int32Array:
        Md = new FRigInt32ArrayMetadata();
        break;
    case ERigMetadataType::Name:
        Md = new FRigNameMetadata();
        break;
    case ERigMetadataType::NameArray:
        Md = new FRigNameArrayMetadata();
        break;
    case ERigMetadataType::Vector:
        Md = new FRigVectorMetadata();
        break;
    case ERigMetadataType::VectorArray:
        Md = new FRigVectorArrayMetadata();
        break;
    case ERigMetadataType::Rotator:
        Md = new FRigRotatorMetadata();
        break;
    case ERigMetadataType::RotatorArray:
        Md = new FRigRotatorArrayMetadata();
        break;
    case ERigMetadataType::Quat:
        Md = new FRigQuatMetadata();
        break;
    case ERigMetadataType::QuatArray:
        Md = new FRigQuatArrayMetadata();
        break;
    case ERigMetadataType::Transform:
        Md = new FRigTransformMetadata();
        break;
    case ERigMetadataType::TransformArray:
        Md = new FRigTransformArrayMetadata();
        break;
    case ERigMetadataType::LinearColor:
        Md = new FRigLinearColorMetadata();
        break;
    case ERigMetadataType::LinearColorArray:
        Md = new FRigLinearColorArrayMetadata();
        break;
    case ERigMetadataType::RigElementKey:
        Md = new FRigRigElementKeyMetadata();
        break;
    case ERigMetadataType::RigElementKeyArray:
        Md = new FRigRigElementKeyArrayMetadata();
        break;
    case ERigMetadataType::Invalid:
        return nullptr;
    }
    if (Md == nullptr)
        return nullptr;
    Md->Name = InName;
    Md->Type = InType;
    return Md;
}

void FRigBaseMetadata::Serialize(FArchive& Ar) {
    Ar << Name;
    Ar << Type;
}

}
