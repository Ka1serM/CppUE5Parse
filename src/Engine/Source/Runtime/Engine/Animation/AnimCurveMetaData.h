#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneReference.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"

#include "AnimCurveMetaData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimCurveType {
    GENERATED_BODY()
    UPROPERTY()
    bool bMaterial = false;
    UPROPERTY()
    bool bMorphtarget = false;
};

USTRUCT()
struct FCurveMetaData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FBoneReference> LinkedBones;
    UPROPERTY()
    uint8 MaxLOD = 0xFF;
    UPROPERTY()
    FAnimCurveType Type;

    UE::Anim::ECurveElementFlags GetFlags() const;
};

UCLASS()
class UAnimCurveMetaData : public UAssetUserData {
    GENERATED_BODY()
public:
    UPROPERTY()
    TMap<FName, FCurveMetaData> CurveMetaData;
};

}
