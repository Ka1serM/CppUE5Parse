#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyDefines.h"

namespace ue5parse::runtime {

FRigHierarchySerializationSettings::FRigHierarchySerializationSettings(const FArchive& InArchive)
    : ControlRigVersion(FControlRigObjectVersion::Get(InArchive)) {}

void FRigHierarchySerializationSettings::Load(FArchive& InArchive) {
    int32 ControlRigVersionInt = INDEX_NONE;
    int32 SerializationPhaseInt = INDEX_NONE;

    InArchive << ControlRigVersionInt;
    InArchive << bIsSerializingToPackage;
    InArchive << bUseCompressedArchive;
    InArchive << bStoreCompactTransforms;
    InArchive << bSerializeLocalTransform;
    InArchive << bSerializeGlobalTransform;
    InArchive << bSerializeInitialTransform;
    InArchive << bSerializeCurrentTransform;
    InArchive << SerializationPhaseInt;

    ControlRigVersion = static_cast<FControlRigObjectVersion::Type>(ControlRigVersionInt);
    SerializationPhase = static_cast<ESerializationPhase>(SerializationPhaseInt);
}

void FRigControlLimitEnabled::Serialize(FArchive& Ar) {
    Ar << bMinimum;
    Ar << bMaximum;
}

FArchive& operator<<(FArchive& Ar, FRigControlValue& Value) {
    FRigControlValueStorage& Storage = Value.FloatStorage;
    for (float* Float : {&Storage.Float00,   &Storage.Float01,   &Storage.Float02,   &Storage.Float03,   &Storage.Float10,   &Storage.Float11,
                         &Storage.Float12,   &Storage.Float13,   &Storage.Float20,   &Storage.Float21,   &Storage.Float22,   &Storage.Float23,
                         &Storage.Float30,   &Storage.Float31,   &Storage.Float32,   &Storage.Float33,   &Storage.Float00_2, &Storage.Float01_2,
                         &Storage.Float02_2, &Storage.Float03_2, &Storage.Float10_2, &Storage.Float11_2, &Storage.Float12_2, &Storage.Float13_2,
                         &Storage.Float20_2, &Storage.Float21_2, &Storage.Float22_2, &Storage.Float23_2, &Storage.Float30_2, &Storage.Float31_2,
                         &Storage.Float32_2, &Storage.Float33_2})
        Ar << *Float;
    return Ar;
}

void FRigElementKey::Serialize(FArchive& Ar) {
    Load(Ar);
}

void FRigElementKey::Load(FArchive& Ar) {
    static const UEnum* ElementTypeEnum = StaticEnum<ERigElementType>();

    FName TypeName;
    Ar << TypeName;

    const int64 TypeValue = ElementTypeEnum->GetValueByName(TypeName);
    if (TypeValue == INDEX_NONE) {
        Ar.SetError(ErrorCode::InvalidData, "FRigElementKey.Type");
        return;
    }
    Type = static_cast<ERigElementType>(TypeValue);

    Ar << Name;
}

void FRigComponentKey::Serialize(FArchive& Ar) {
    Load(Ar);
}

void FRigComponentKey::Load(FArchive& Ar) {
    Ar << Name;
    Ar << ElementKey;
}

void FRigHierarchyKey::Serialize(FArchive& Ar) {
    bool bIsElement = false;
    Ar << bIsElement;
    if (bIsElement)
        Ar << Element.Emplace();
    else
        Element.Reset();

    bool bIsComponent = false;
    Ar << bIsComponent;
    if (bIsComponent)
        Ar << Component.Emplace();
    else
        Component.Reset();
}

}
