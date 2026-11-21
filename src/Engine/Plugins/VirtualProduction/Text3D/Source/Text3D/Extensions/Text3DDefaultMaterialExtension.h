#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DMaterialExtensionBase.h"

#include "Text3DDefaultMaterialExtension.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UCLASS()
class UText3DDefaultMaterialExtension : public UText3DMaterialExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DDefaultMaterialExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UMaterialInterface> FrontMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> BevelMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> ExtrudeMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> BackMaterial = nullptr;
};

}
