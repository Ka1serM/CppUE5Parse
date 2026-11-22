#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperCharacter.h"

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperFlipbookComponent.h"
#include "Engine/Source/Runtime/Engine/Components/CapsuleComponent.h"

namespace ue5parse::runtime {

APaperCharacter::APaperCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.DoNotCreateDefaultSubobject(ACharacter::MeshComponentName)) {
    Sprite = ObjectInitializer.CreateOptionalDefaultSubobject<UPaperFlipbookComponent>(FName("Sprite0"));
    if (Sprite == nullptr)
        return;
    Sprite->SetupAttachment(GetCapsuleComponent());
}

}
