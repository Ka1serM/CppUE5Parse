#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DLayoutExtensionBase.h"

#include "Text3DDefaultLayoutExtension.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UText3DDefaultLayoutExtension : public UText3DLayoutExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DDefaultLayoutExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
