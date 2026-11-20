#include "Engine/Source/Runtime/Engine/Camera/CameraShakeSourceActor.h"

#include "Engine/Source/Runtime/Engine/Camera/CameraShakeSourceComponent.h"

namespace ue5parse::runtime {

ACameraShakeSourceActor::ACameraShakeSourceActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    CameraShakeSourceComponent = ObjectInitializer.CreateDefaultSubobject<UCameraShakeSourceComponent>(FName("CameraShakeSourceComponent"));
    RootComponent = CameraShakeSourceComponent;
}

}
