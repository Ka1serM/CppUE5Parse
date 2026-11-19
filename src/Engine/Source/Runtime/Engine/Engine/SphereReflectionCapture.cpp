#include "Engine/Source/Runtime/Engine/Engine/SphereReflectionCapture.h"

#include "Engine/Source/Runtime/Engine/Components/DrawSphereComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SphereReflectionCaptureComponent.h"

namespace ue5parse::runtime {

ASphereReflectionCapture::ASphereReflectionCapture(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<USphereReflectionCaptureComponent>(FName("NewReflectionComponent"))) {
    auto* SphereComponent = CastChecked<USphereReflectionCaptureComponent>(GetCaptureComponent());
    RootComponent = SphereComponent;
    auto* DrawInfluenceRadius = ObjectInitializer.CreateDefaultSubobject<UDrawSphereComponent>(FName("DrawRadius0"));
    DrawInfluenceRadius->SetupAttachment(SphereComponent);
    DrawInfluenceRadius->bDrawOnlyIfSelected = true;
    DrawInfluenceRadius->bUseEditorCompositing = true;
    DrawInfluenceRadius->BodyInstance.CollisionProfileName = FName("NoCollision");
    SphereComponent->PreviewInfluenceRadius = DrawInfluenceRadius;
    DrawCaptureRadius = ObjectInitializer.CreateDefaultSubobject<UDrawSphereComponent>(FName("DrawRadius1"));
    DrawCaptureRadius->SetupAttachment(SphereComponent);
    DrawCaptureRadius->bDrawOnlyIfSelected = true;
    DrawCaptureRadius->bUseEditorCompositing = true;
    DrawCaptureRadius->BodyInstance.CollisionProfileName = FName("NoCollision");
    DrawCaptureRadius->ShapeColor = FColor(100, 90, 40);
}

}
