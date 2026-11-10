#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MaterialTypes.generated.h"

namespace ue5parse::runtime {

enum EMaterialParameterAssociation : int32 { LayerParameter, BlendParameter, GlobalParameter };

enum class EMaterialParameterType : uint8 {
    Scalar,
    Vector,
    DoubleVector,
    Texture,
    Font,
    RuntimeVirtualTexture,
    SparseVolumeTexture,
    StaticSwitch,
    NumRuntime = StaticSwitch,
};

USTRUCT()
struct FMaterialParameterInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TEnumAsByte<EMaterialParameterAssociation> Association = GlobalParameter;
    UPROPERTY()
    int32 Index = INDEX_NONE;

    FMaterialParameterInfo() = default;
    FMaterialParameterInfo(FName InName, EMaterialParameterAssociation InAssociation = GlobalParameter, int32 InIndex = INDEX_NONE)
        : Name(InName), Association(InAssociation), Index(InIndex) {}
    bool operator==(const FMaterialParameterInfo& Other) const { return Name == Other.Name && Association == Other.Association && Index == Other.Index; }
    friend uint32 GetTypeHash(const FMaterialParameterInfo& Info) {
        return HashCombine(GetTypeHash(Info.Name), static_cast<uint32>(Info.Index) * 3u + Info.Association.GetIntValue());
    }
};

}
