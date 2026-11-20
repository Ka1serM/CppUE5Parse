#pragma once

#include "Engine/Source/Runtime/MediaAssets/BaseMediaSource.h"

#include "StreamMediaSource.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UStreamMediaSource : public UBaseMediaSource {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString StreamUrl;
};

}
