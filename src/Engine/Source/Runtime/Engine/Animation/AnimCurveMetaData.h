#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"

#include "AnimCurveMetaData.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCurveMetaData : public UAssetUserData {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    const TMap<FName, UE::Anim::ECurveElementFlags>& GetCurveFlags() const { return CurveFlags; }

private:
    TMap<FName, UE::Anim::ECurveElementFlags> CurveFlags;
};

}
