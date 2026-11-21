#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DRenderingExtensionBase.h"

#include "Text3DDefaultRenderingExtension.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UText3DDefaultRenderingExtension : public UText3DRenderingExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DDefaultRenderingExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
