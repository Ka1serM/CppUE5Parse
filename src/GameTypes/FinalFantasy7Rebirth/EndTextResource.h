#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "EndTextResource.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FEndTextResourceStrings {
    GENERATED_BODY()
    UPROPERTY()
    FString Text;
    UPROPERTY()
    TMap<FName, FString> MetaData;

    friend FArchive& operator<<(FArchive& Ar, FEndTextResourceStrings& Strings) { return Ar << Strings.Text << Strings.MetaData; }
};

UCLASS()
class UEndTextResource : public UObject {
    GENERATED_BODY()
public:
    explicit UEndTextResource(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TMap<FString, FEndTextResourceStrings> Strings;
};

}
