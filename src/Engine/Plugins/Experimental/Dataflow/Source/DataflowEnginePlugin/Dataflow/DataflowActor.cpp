#include "Engine/Plugins/Experimental/Dataflow/Source/DataflowEnginePlugin/Dataflow/DataflowActor.h"

#include "Engine/Plugins/Experimental/Dataflow/Source/DataflowEnginePlugin/Dataflow/DataflowComponent.h"

namespace ue5parse::runtime {

ADataflowActor::ADataflowActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DataflowComponent = ObjectInitializer.CreateDefaultSubobject<UDataflowComponent>(FName("DataflowComponent0"));
    RootComponent = DataflowComponent;
    PrimaryActorTick.bCanEverTick = true;
}

}
