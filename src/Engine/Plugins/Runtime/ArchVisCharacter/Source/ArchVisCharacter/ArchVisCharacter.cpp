#include "Engine/Plugins/Runtime/ArchVisCharacter/Source/ArchVisCharacter/ArchVisCharacter.h"

#include "Engine/Plugins/Runtime/ArchVisCharacter/Source/ArchVisCharacter/ArchVisCharMovementComponent.h"

namespace ue5parse::runtime {

AArchVisCharacter::AArchVisCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UArchVisCharMovementComponent>(ACharacter::CharacterMovementComponentName)) {}

}
