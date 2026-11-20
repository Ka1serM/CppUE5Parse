#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

#include "VolumetricCloud.generated.h"

namespace ue5parse::runtime {

class UVolumetricCloudComponent;

UCLASS()
class AVolumetricCloud : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UVolumetricCloudComponent> VolumetricCloudComponent = nullptr;

    explicit AVolumetricCloud(const FObjectInitializer& ObjectInitializer);
};

}
