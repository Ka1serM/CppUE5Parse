#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "PaperGroupedSpriteComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPaperGroupedSpriteComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UPaperGroupedSpriteComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
