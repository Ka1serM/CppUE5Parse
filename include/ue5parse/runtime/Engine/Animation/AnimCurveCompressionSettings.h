#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCurveCompressionCodec.h"

#include "AnimCurveCompressionSettings.generated.h"

namespace ue5parse::runtime {

// UAnimCurveCompressionSettings (UE's Animation/AnimCurveCompressionSettings.h).
UCLASS()
class UAnimCurveCompressionSettings : public UObject {
    GENERATED_BODY()
public:
    explicit UAnimCurveCompressionSettings(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    UAnimCurveCompressionCodec* Codec = nullptr;

    UAnimCurveCompressionCodec* GetCodec(const FString& Path) const { return Codec ? Codec->GetCodec(Path) : nullptr; }
};

}
