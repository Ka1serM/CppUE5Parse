#pragma once

#include "Engine/Source/Runtime/Engine/ActorPartition/PartitionActor.h"

#include "ISMPartitionActor.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class AISMPartitionActor : public APartitionActor {
    GENERATED_BODY()
};

}
