#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "RuntimeVirtualTextureComponent.generated.h"

namespace ue5parse::runtime {

class URuntimeVirtualTexture;

UCLASS()
class URuntimeVirtualTextureComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<URuntimeVirtualTexture> VirtualTexture = nullptr;

    explicit URuntimeVirtualTextureComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
