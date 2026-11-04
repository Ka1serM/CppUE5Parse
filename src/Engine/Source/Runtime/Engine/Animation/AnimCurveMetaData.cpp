#include "Engine/Source/Runtime/Engine/Animation/AnimCurveMetaData.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

void UAnimCurveMetaData::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || HasAnyFlags(RF_ClassDefaultObject)) return;
    const FMapProperty* Map = FindFProperty<FMapProperty>(GetClass(), FName("CurveMetaData"));
    const FStructProperty* MetaData = Map ? CastField<FStructProperty>(Map->ValueProp) : nullptr;
    const FStructProperty* Type = MetaData ? FindFProperty<FStructProperty>(MetaData->Struct, FName("Type")) : nullptr;
    if (Type == nullptr || CastField<FNameProperty>(Map->KeyProp) == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "UAnimCurveMetaData.CurveMetaData");
        return;
    }
    FScriptMapHelper Helper(Map, Map->ContainerPtrToValuePtr<void>(this));
    for (int32 i = 0; i < Helper.GetMaxIndex(); i++) {
        if (!Helper.IsValidIndex(i)) continue;
        const void* CurveType = Type->ContainerPtrToValuePtr<void>(Helper.GetValuePtr(i));
        bool bMaterial = false;
        bool bMorphtarget = false;
        GetPropertyValue_InContainer(Type->Struct, CurveType, FName("bMaterial"), bMaterial);
        GetPropertyValue_InContainer(Type->Struct, CurveType, FName("bMorphtarget"), bMorphtarget);
        UE::Anim::ECurveElementFlags Flags = UE::Anim::ECurveElementFlags::None;
        if (bMaterial) Flags |= UE::Anim::ECurveElementFlags::Material;
        if (bMorphtarget) Flags |= UE::Anim::ECurveElementFlags::MorphTarget;
        if (Flags != UE::Anim::ECurveElementFlags::None) CurveFlags.Add(*reinterpret_cast<const FName*>(Helper.GetKeyPtr(i)), Flags);
    }
}

}
