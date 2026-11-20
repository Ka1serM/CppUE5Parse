#pragma once

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "LandscapeNaniteComponent.generated.h"

namespace ue5parse::runtime {

class ULandscapeComponent;

UCLASS()
class ULandscapeNaniteComponent : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid ProxyContentId;
    UPROPERTY()
    TArray<TObjectPtr<ULandscapeComponent>> SourceLandscapeComponents;
};

}
