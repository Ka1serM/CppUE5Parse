#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "RuntimeVirtualTextureVolume.generated.h"

namespace ue5parse::runtime {

class URuntimeVirtualTextureComponent;

UCLASS()
class ARuntimeVirtualTextureVolume : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<URuntimeVirtualTextureComponent> VirtualTextureComponent = nullptr;

    explicit ARuntimeVirtualTextureVolume(const FObjectInitializer& ObjectInitializer);
};

}
