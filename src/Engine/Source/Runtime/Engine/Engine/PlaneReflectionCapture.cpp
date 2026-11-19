#include "Engine/Source/Runtime/Engine/Engine/PlaneReflectionCapture.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"
#include "Engine/Source/Runtime/Engine/Components/DrawSphereComponent.h"
#include "Engine/Source/Runtime/Engine/Components/PlaneReflectionCaptureComponent.h"

namespace ue5parse::runtime {

APlaneReflectionCapture::APlaneReflectionCapture(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UPlaneReflectionCaptureComponent>(FName("NewReflectionComponent"))) {
    auto* PlaneComponent = CastChecked<UPlaneReflectionCaptureComponent>(GetCaptureComponent());
    PlaneComponent->SetRelativeScale3D_Direct(FVector(1, 1000, 1000));
    RootComponent = PlaneComponent;
    auto* DrawInfluenceRadius = ObjectInitializer.CreateDefaultSubobject<UDrawSphereComponent>(FName("DrawRadius0"));
    DrawInfluenceRadius->SetupAttachment(PlaneComponent);
    DrawInfluenceRadius->bDrawOnlyIfSelected = true;
    DrawInfluenceRadius->SetUsingAbsoluteScale(true);
    DrawInfluenceRadius->bUseEditorCompositing = true;
    DrawInfluenceRadius->BodyInstance.CollisionProfileName = FName("NoCollision");
    PlaneComponent->PreviewInfluenceRadius = DrawInfluenceRadius;
    auto* DrawCaptureBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("DrawBox1"));
    DrawCaptureBox->SetupAttachment(PlaneComponent);
    DrawCaptureBox->bDrawOnlyIfSelected = true;
    DrawCaptureBox->bUseEditorCompositing = true;
    DrawCaptureBox->BodyInstance.CollisionProfileName = FName("NoCollision");
    DrawCaptureBox->ShapeColor = FColor(100, 90, 40);
    DrawCaptureBox->BoxExtent = FVector(1, 1, 1);
    PlaneComponent->PreviewCaptureBox = DrawCaptureBox;
}

}
