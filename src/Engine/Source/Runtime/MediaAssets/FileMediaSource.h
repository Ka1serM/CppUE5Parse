#pragma once

#include "Engine/Source/Runtime/MediaAssets/BaseMediaSource.h"

#include "FileMediaSource.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UFileMediaSource : public UBaseMediaSource {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString FilePath;
    UPROPERTY()
    bool PrecacheFile = false;
};

}
