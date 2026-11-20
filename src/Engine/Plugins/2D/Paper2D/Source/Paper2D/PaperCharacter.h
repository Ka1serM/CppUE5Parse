#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Character.h"

#include "PaperCharacter.generated.h"

namespace ue5parse::runtime {

class UPaperFlipbookComponent;

UCLASS()
class APaperCharacter : public ACharacter {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperFlipbookComponent> Sprite = nullptr;

    explicit APaperCharacter(const FObjectInitializer& ObjectInitializer);
};

}
