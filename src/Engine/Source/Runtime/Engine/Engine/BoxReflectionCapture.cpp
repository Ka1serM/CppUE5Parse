#include "Engine/Source/Runtime/Engine/Engine/BoxReflectionCapture.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"
#include "Engine/Source/Runtime/Engine/Components/BoxReflectionCaptureComponent.h"

namespace ue5parse::runtime {

ABoxReflectionCapture::ABoxReflectionCapture(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UBoxReflectionCaptureComponent>(FName("NewReflectionComponent"))) {
    auto* BoxComponent = CastChecked<UBoxReflectionCaptureComponent>(GetCaptureComponent());
    BoxComponent->SetRelativeScale3D_Direct(FVector(1000, 1000, 400));
    RootComponent = BoxComponent;
    auto* DrawInfluenceBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("DrawBox0"));
    DrawInfluenceBox->SetupAttachment(BoxComponent);
    DrawInfluenceBox->bDrawOnlyIfSelected = true;
    DrawInfluenceBox->bUseEditorCompositing = true;
    DrawInfluenceBox->BodyInstance.CollisionProfileName = FName("NoCollision");
    DrawInfluenceBox->BoxExtent = FVector(1, 1, 1);
    BoxComponent->PreviewInfluenceBox = DrawInfluenceBox;
    auto* DrawCaptureBox = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("DrawBox1"));
    DrawCaptureBox->SetupAttachment(BoxComponent);
    DrawCaptureBox->bDrawOnlyIfSelected = true;
    DrawCaptureBox->bUseEditorCompositing = true;
    DrawCaptureBox->BodyInstance.CollisionProfileName = FName("NoCollision");
    DrawCaptureBox->ShapeColor = FColor(100, 90, 40);
    DrawCaptureBox->BoxExtent = FVector(1, 1, 1);
    BoxComponent->PreviewCaptureBox = DrawCaptureBox;
}

}
