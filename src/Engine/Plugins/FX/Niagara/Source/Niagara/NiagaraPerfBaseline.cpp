#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPerfBaseline.h"

#include "Engine/Source/Runtime/Engine/Components/TextRenderComponent.h"

namespace ue5parse::runtime {

ANiagaraPerfBaselineActor::ANiagaraPerfBaselineActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("RootComponent"));
    Label = ObjectInitializer.CreateDefaultSubobject<UTextRenderComponent>(FName("LabelText"));
    Label->SetRelativeRotation_Direct(FRotator(0.0f, 180.0f, 0.0f));
    Label->SetRelativeLocation_Direct(FVector(-250.0f, 0.0f, -100.0f));
}

}
