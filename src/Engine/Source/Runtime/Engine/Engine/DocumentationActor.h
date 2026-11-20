#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DocumentationActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ADocumentationActor : public AActor {
    GENERATED_BODY()
public:
    explicit ADocumentationActor(const FObjectInitializer& ObjectInitializer);
};

}
