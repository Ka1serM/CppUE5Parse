#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NiagaraPreviewGrid.generated.h"

namespace ue5parse::runtime {

class UNiagaraSystem;

UCLASS()
class ANiagaraPreviewGrid : public AActor {
    GENERATED_BODY()
public:
    explicit ANiagaraPreviewGrid(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UNiagaraSystem> System = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> PreviewAxisX = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> PreviewAxisY = nullptr;
    UPROPERTY()
    float SpacingX = 250.0f;
    UPROPERTY()
    float SpacingY = 250.0f;
};

}
