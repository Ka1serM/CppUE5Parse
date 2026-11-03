#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveTypes.h"
#include "ue5parse/runtime/Engine/Engine/AssetUserData.h"

#include "AnimCurveMetaData.generated.h"

namespace ue5parse::runtime {

// UAnimCurveMetaData: the curve metadata of a skeleton or a skeletal mesh, kept as asset user data (UE's
// Animation/AnimCurveMetadata.h). The mapped CurveMetaData map has no C++ view; its material and morph target types are
// read from it after the properties.
UCLASS()
class UAnimCurveMetaData : public UAssetUserData {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    // The flags of the curves that drive a material parameter or a morph target.
    const TMap<FName, UE::Anim::ECurveElementFlags>& GetCurveFlags() const { return CurveFlags; }

private:
    TMap<FName, UE::Anim::ECurveElementFlags> CurveFlags;
};

}
