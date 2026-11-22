#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "BitMapData.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBitMapData : public UObject {
    GENERATED_BODY()
public:
    explicit UBitMapData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    int32 m_SizeX = 0;
    UPROPERTY()
    int32 m_SizeY = 0;
    UPROPERTY()
    int32 m_BitsPerWord = 0;
    //! The words of m_BitsPerWord bits each, row by row, with the low bit first.
    UPROPERTY()
    TArray<uint8> Data;
};

}
