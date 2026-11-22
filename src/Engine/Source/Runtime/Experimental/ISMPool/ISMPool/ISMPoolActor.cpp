#include "Engine/Source/Runtime/Experimental/ISMPool/ISMPool/ISMPoolActor.h"

#include "Engine/Source/Runtime/Experimental/ISMPool/ISMPool/ISMPoolComponent.h"
#include "Engine/Source/Runtime/Experimental/ISMPool/ISMPool/ISMPoolDebugDrawComponent.h"

namespace ue5parse::runtime {

AISMPoolActor::AISMPoolActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ISMPoolComp = ObjectInitializer.CreateDefaultSubobject<UISMPoolComponent>(FName("ISMPoolComp"));
    RootComponent = ISMPoolComp;
    ISMPoolDebugDrawComp = ObjectInitializer.CreateDefaultSubobject<UISMPoolDebugDrawComponent>(FName("ISMPoolDebug"));
    ISMPoolDebugDrawComp->SetupAttachment(ISMPoolComp);
}

}
