#pragma once

#include "Engine/Source/Runtime/Engine/Engine/NetDriver.h"

#include "DemoNetDriver.generated.h"

namespace ue5parse::runtime {

UCLASS(Transient)
class UDemoNetDriver : public UNetDriver {
    GENERATED_BODY()
public:
    bool IsServer() const override;
    bool IsRecording() const;
    bool IsPlaying() const;
};

}
