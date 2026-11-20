#pragma once

#include "Engine/Source/Runtime/Landscape/Landscape.h"

#include "LandscapeStreamingProxy.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ALandscapeStreamingProxy : public ALandscapeProxy {
    GENERATED_BODY()
public:
    explicit ALandscapeStreamingProxy(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TLazyObjectPtr<ALandscape> LandscapeActor;
    UPROPERTY()
    TSoftObjectPtr<ALandscape> LandscapeActorRef;
    UPROPERTY()
    TSet<FName> OverriddenSharedProperties;
};

}
