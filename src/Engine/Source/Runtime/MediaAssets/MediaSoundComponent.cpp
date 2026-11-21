#include "Engine/Source/Runtime/MediaAssets/MediaSoundComponent.h"

namespace ue5parse::runtime {

UMediaSoundComponent::UMediaSoundComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    EnvelopeFollowerAttackTime = 10;
    EnvelopeFollowerReleaseTime = 100;
}

}
