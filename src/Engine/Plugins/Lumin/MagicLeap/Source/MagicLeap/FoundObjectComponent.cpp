#include "Engine/Plugins/Lumin/MagicLeap/Source/MagicLeap/FoundObjectComponent.h"

#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"

namespace ue5parse::runtime {

UFoundObjectComponent::UFoundObjectComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    SearchVolume = ObjectInitializer.CreateDefaultSubobject<UBoxComponent>(FName("SearchVolume"));
    SearchVolume->SetupAttachment(this);
}

}
