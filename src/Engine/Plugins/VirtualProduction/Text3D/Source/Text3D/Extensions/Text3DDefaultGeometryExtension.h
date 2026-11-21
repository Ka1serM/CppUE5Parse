#pragma once

#include "Engine/Plugins/VirtualProduction/Text3D/Source/Text3D/Extensions/Text3DGeometryExtensionBase.h"

#include "Text3DDefaultGeometryExtension.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UText3DDefaultGeometryExtension : public UText3DGeometryExtensionBase {
    GENERATED_BODY()
public:
    explicit UText3DDefaultGeometryExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
