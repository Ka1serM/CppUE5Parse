#include "Engine/Plugins/Runtime/AR/AppleAR/AppleARKitFaceSupport/Source/AppleARKitFaceSupport/AppleARKitFaceMeshComponent.h"

namespace ue5parse::runtime {

UAppleARKitFaceMeshComponent::UAppleARKitFaceMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
}

}
