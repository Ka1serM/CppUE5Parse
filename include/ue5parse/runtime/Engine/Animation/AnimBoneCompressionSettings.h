#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionCodec.h"

#include "AnimBoneCompressionSettings.generated.h"

namespace ue5parse::runtime {

// UAnimBoneCompressionSettings (UE's Animation/AnimBoneCompressionSettings.h): the codecs an animation may use.
UCLASS()
class UAnimBoneCompressionSettings : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UAnimBoneCompressionCodec>> Codecs;

    // The codec of a DDC handle (UE's GetCodec).
    UAnimBoneCompressionCodec* GetCodec(const FString& DDCHandle) const;
};

}
