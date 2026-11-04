#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCurveCompressionCodec.h"

#include "AnimCurveCompressionSettings.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCurveCompressionSettings : public UObject {
    GENERATED_BODY()
public:
    explicit UAnimCurveCompressionSettings(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UAnimCurveCompressionCodec> Codec = nullptr;

    UAnimCurveCompressionCodec* GetCodec(const FString& Path) const { return Codec ? Codec->GetCodec(Path) : nullptr; }
};

}
