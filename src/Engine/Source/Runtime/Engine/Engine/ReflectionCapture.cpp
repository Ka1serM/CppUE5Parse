#include "Engine/Source/Runtime/Engine/Engine/ReflectionCapture.h"

#include "Engine/Source/Runtime/Engine/Components/ReflectionCaptureComponent.h"

namespace ue5parse::runtime {

AReflectionCapture::AReflectionCapture(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    CaptureComponent = ObjectInitializer.CreateDefaultSubobject<UReflectionCaptureComponent>(FName("NewReflectionComponent"));
    bCanBeInCluster = true;
}

}
