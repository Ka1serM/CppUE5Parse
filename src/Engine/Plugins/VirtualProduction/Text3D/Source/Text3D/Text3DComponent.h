#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "Text3DComponent.generated.h"

namespace ue5parse::runtime {

class UText3DRendererBase;
class UText3DCharacterExtensionBase;
class UText3DGeometryExtensionBase;
class UText3DLayoutExtensionBase;
class UText3DMaterialExtensionBase;
class UText3DRenderingExtensionBase;
class UText3DStyleExtensionBase;
class UText3DTokenExtensionBase;

UCLASS()
class UText3DComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UText3DComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USceneComponent> TextRoot = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DRendererBase> TextRenderer = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DCharacterExtensionBase> CharacterExtension = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DGeometryExtensionBase> GeometryExtension = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DLayoutExtensionBase> LayoutExtension = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DMaterialExtensionBase> MaterialExtension = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DRenderingExtensionBase> RenderingExtension = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DStyleExtensionBase> StyleExtension = nullptr;
    UPROPERTY()
    TObjectPtr<UText3DTokenExtensionBase> TokenExtension = nullptr;
};

}
