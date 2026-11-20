#include "Engine/Source/Runtime/Engine/WorldPartition/HLOD/HLODActor.h"

namespace ue5parse::runtime {

AWorldPartitionHLOD::AWorldPartitionHLOD(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bCanBeDamaged = false;
    bReplicates = true;
}

}
