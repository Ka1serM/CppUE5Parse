#pragma once

#include <optional>
#include <span>
#include <string>

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "BinaryAsset.generated.h"

namespace ue5parse::runtime {

//! Octopath Traveler 0 and Champions of the Continent save the data of the asset as MessagePack in BinaryData, which Data holds as JSON text.
UCLASS()
class UBinaryAsset : public UObject {
    GENERATED_BODY()
public:
    explicit UBinaryAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    static std::optional<std::string> DecodeMessagePack(std::span<const uint8> Bytes);

    UPROPERTY()
    TArray<uint8> BinaryData;
    UPROPERTY()
    FString Data;
};

}
