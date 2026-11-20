#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "SparseVolumeTextureViewerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USparseVolumeTextureViewerComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit USparseVolumeTextureViewerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
