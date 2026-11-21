#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DTokenExtensionBase.h"

#include "Text3DDefaultTokenExtension.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UText3DDefaultTokenExtension : public UText3DTokenExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DDefaultTokenExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
