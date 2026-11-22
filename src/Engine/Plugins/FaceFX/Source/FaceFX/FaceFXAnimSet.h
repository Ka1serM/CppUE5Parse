#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "FaceFXAnimSet.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UFaceFXAnimSet : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    TArray<TArray<uint8>> RawFaceFXAnimSetBytes;
    TArray<TArray<uint8>> RawFaceFXMiniSessionBytes;
};

}
