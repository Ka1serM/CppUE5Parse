#pragma once

#include "Engine/Source/Runtime/MediaAssets/MediaSource.h"

#include "BaseMediaSource.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UBaseMediaSource : public UMediaSource {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    FName PlayerName;
};

}
