#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "PaperFlipbookComponent.generated.h"

namespace ue5parse::runtime {

class UPaperFlipbook;

UCLASS()
class UPaperFlipbookComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperFlipbook> SourceFlipbook = nullptr;

    explicit UPaperFlipbookComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
