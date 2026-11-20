#pragma once

#include "Engine/Plugins/Runtime/XRBase/Source/XRBase/VRNotificationsComponent.h"

#include "MagicLeapHeadTrackingNotificationsComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMagicLeapHeadTrackingNotificationsComponent : public UVRNotificationsComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapHeadTrackingNotificationsComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
