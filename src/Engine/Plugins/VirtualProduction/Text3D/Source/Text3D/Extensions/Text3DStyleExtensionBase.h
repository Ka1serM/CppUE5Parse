#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DExtensionBase.h"

#include "Text3DStyleExtensionBase.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UText3DStyleExtensionBase : public UText3DExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DStyleExtensionBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
