#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DCharacterExtensionBase.h"

#include "Text3DDefaultCharacterExtension.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UText3DDefaultCharacterExtension : public UText3DCharacterExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DDefaultCharacterExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
