#pragma once

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/VectorField/VectorField.h"

#include "VectorFieldStatic.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UVectorFieldStatic : public UVectorField {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 SizeX = 0;
    UPROPERTY()
    int32 SizeY = 0;
    UPROPERTY()
    int32 SizeZ = 0;
    UPROPERTY()
    bool bAllowCPUAccess = false;

    FByteBulkData SourceData;

    void Serialize(FArchive& Ar) override;
};

}
