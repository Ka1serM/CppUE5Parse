#include "Engine/Plugins/PCG/Source/PCG/PCGComponent.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/PCG/Source/PCG/PCGGraph.h"

namespace ue5parse::runtime {

UPCGComponent::UPCGComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    if (GetRuntimeProvider().getGame() >= EGame::GAME_UE5_2)
        GraphInstance = ObjectInitializer.CreateDefaultSubobject<UPCGGraphInstance>(FName("PCGGraphInstance"));
}

}
