#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Character.h"

#include "ArchVisCharacter.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AArchVisCharacter : public ACharacter {
    GENERATED_BODY()
public:
    explicit AArchVisCharacter(const FObjectInitializer& ObjectInitializer);
};

}
