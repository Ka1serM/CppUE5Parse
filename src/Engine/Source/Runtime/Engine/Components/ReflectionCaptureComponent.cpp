#include "Engine/Source/Runtime/Engine/Components/ReflectionCaptureComponent.h"

namespace ue5parse::runtime {

UReflectionCaptureComponent::UReflectionCaptureComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Mobility = EComponentMobility::Static;
}

}
