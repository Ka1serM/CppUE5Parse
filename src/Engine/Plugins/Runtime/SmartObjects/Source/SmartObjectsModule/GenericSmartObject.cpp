#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/GenericSmartObject.h"

#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectComponent.h"

namespace ue5parse::runtime {

AGenericSmartObject::AGenericSmartObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
    RootComponent->Mobility = EComponentMobility::Static;
    SOComponent = ObjectInitializer.CreateDefaultSubobject<USmartObjectComponent>(FName("SmartObjectComp"));
    SOComponent->SetupAttachment(RootComponent);
    bHidden = true;
    bCanBeDamaged = false;
}

}
