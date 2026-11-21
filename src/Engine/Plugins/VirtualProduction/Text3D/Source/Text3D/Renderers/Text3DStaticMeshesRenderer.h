#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Renderers/Text3DRendererBase.h"

#include "Text3DStaticMeshesRenderer.generated.h"

namespace ue5parse::runtime {

class USceneComponent;

UCLASS()
class UText3DStaticMeshesRenderer : public UText3DRendererBase {
    GENERATED_BODY()
public:
    explicit UText3DStaticMeshesRenderer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<USceneComponent> TextRoot = nullptr;
};

}
