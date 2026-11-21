#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DExtensionBase.h"

#include "Text3DMaterialExtensionBase.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UText3DMaterialExtensionBase : public UText3DExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DMaterialExtensionBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
