#include "Engine/Source/Runtime/Engine/WorldPartition/DataLayer/WorldDataLayers.h"

namespace ue5parse::runtime {

AWorldDataLayers::AWorldDataLayers(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.DoNotCreateDefaultSubobject(FName("Sprite"))) {
    bAlwaysRelevant = true;
    bReplicates = true;
}

}
