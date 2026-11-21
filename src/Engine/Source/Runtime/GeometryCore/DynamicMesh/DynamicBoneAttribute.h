#pragma once

#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

template <class AttribValueType>
class TDynamicBoneAttributeBase {
public:
    void Serialize(FArchive& Ar) { Ar << AttribValues; }

    const TArray<AttribValueType>& GetValues() const { return AttribValues; }

private:
    TArray<AttribValueType> AttribValues;
};

//! The bone names go through FNameAsStringProxyArchive, so they are strings on disk.
template <>
inline void TDynamicBoneAttributeBase<FName>::Serialize(FArchive& Ar) {
    TArray<FString> Names;
    Ar << Names;
    AttribValues.Empty(Names.Num());
    for (const FString& Name : Names)
        AttribValues.Add(MakeName(Name));
}

using FDynamicMeshBoneNameAttribute = TDynamicBoneAttributeBase<FName>;
using FDynamicMeshBoneParentIndexAttribute = TDynamicBoneAttributeBase<int32>;
using FDynamicMeshBoneColorAttribute = TDynamicBoneAttributeBase<FVector4f>;
using FDynamicMeshBonePoseAttribute = TDynamicBoneAttributeBase<FTransform>;

}
