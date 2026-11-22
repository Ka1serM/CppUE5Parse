#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "ImgMediaPlaybackComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UImgMediaPlaybackComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UImgMediaPlaybackComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
