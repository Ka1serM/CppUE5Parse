#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "REDBinaryObject.generated.h"

namespace ue5parse::runtime {

//! The binary objects of Arc System Works keep their data twice, as big endian and as little endian bulk data.
UCLASS()
class UREDBinaryObject : public UObject {
    GENERATED_BODY()
public:
    explicit UREDBinaryObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        Super::Serialize(Ar);
        if (Ar.IsError())
            return;
        DataBE.Serialize(Ar);
        DataLE.Serialize(Ar);
    }

    FByteBulkData DataBE;
    FByteBulkData DataLE;
};

}
