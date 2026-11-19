#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "FontBulkData.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UFontBulkData : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

private:
    FByteBulkData BulkData;
};

}
