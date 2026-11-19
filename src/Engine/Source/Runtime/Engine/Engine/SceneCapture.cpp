#include "Engine/Source/Runtime/Engine/Engine/SceneCapture.h"

namespace ue5parse::runtime {

ASceneCapture::ASceneCapture(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComponent"));
    RootComponent = SceneComponent;
}

}
