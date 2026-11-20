#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "PhysicsSerializer.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPhysicsSerializer : public UObject {
    GENERATED_BODY()
public:
    FFormatContainer BinaryFormatData;

    void Serialize(FArchive& Ar) override;
};

}
