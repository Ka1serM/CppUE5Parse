#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DStyleExtensionBase.h"

#include "Text3DDefaultStyleExtension.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UText3DDefaultStyleExtension : public UText3DStyleExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DDefaultStyleExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
