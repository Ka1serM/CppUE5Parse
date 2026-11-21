#include "Engine/Source/Runtime/Experimental/FieldSystem/Source/FieldSystemEngine/Field/FieldSystemActor.h"

#include "Engine/Source/Runtime/Experimental/FieldSystem/Source/FieldSystemEngine/Field/FieldSystemComponent.h"

namespace ue5parse::runtime {

AFieldSystemActor::AFieldSystemActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    FieldSystemComponent = ObjectInitializer.CreateDefaultSubobject<UFieldSystemComponent>(FName("FieldSystemComponent"));
    RootComponent = FieldSystemComponent;
}

}
