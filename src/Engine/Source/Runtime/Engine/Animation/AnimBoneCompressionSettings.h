#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimBoneCompressionCodec.h"

#include "AnimBoneCompressionSettings.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimBoneCompressionSettings : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UAnimBoneCompressionCodec>> Codecs;

    UAnimBoneCompressionCodec* GetCodec(const FString& DDCHandle) const;
};

}
