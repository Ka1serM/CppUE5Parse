#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomActor.h"

#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomComponent.h"

namespace ue5parse::runtime {

AGroomActor::AGroomActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GroomComponent = ObjectInitializer.CreateDefaultSubobject<UGroomComponent>(FName("GroomComponent0"));
    RootComponent = GroomComponent;
}

}
