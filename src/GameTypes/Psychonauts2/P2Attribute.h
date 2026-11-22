#pragma once

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "P2Attribute.generated.h"

namespace ue5parse::runtime {

//! Psychonauts 2 saves a line code as 13 UTF-8 bytes.
USTRUCT()
struct FLinecode {
    GENERATED_BODY()
    UPROPERTY()
    FString Value;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FLinecode> : TStructOpsTypeTraitsBase2<FLinecode> {
    enum { WithSerializer = true };
};

UENUM()
enum class EAttribute : uint8 {
    Invalid = 0,
    Enum = 1,
    Integer = 2,
    Float = 3,
    Boolean = 4,
    String = 5,
    Text = 6,
    Name = 7,
    Object = 8,
    Class = 9,
    Vector = 10,
    Quat = 11,
    Color = 12,
    NUM = 13,
};

//! Psychonauts 2 saves the type of the attribute and then only the value of that type.
USTRUCT()
struct FP2Attribute {
    GENERATED_BODY()
    UPROPERTY()
    EAttribute Type = EAttribute::Invalid;
    UPROPERTY()
    FName NameValue;
    UPROPERTY()
    int32 IntValue = 0;
    UPROPERTY()
    float FloatValue = 0;
    UPROPERTY()
    bool BoolValue = false;
    UPROPERTY()
    FString StringValue;
    UPROPERTY()
    FText TextValue;
    UPROPERTY()
    TObjectPtr<UObject> ObjectValue = nullptr;
    UPROPERTY()
    FVector3f VectorValue;
    UPROPERTY()
    FQuat4f QuatValue;
    UPROPERTY()
    FLinearColor ColorValue;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FP2Attribute> : TStructOpsTypeTraitsBase2<FP2Attribute> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FP2StoryDataStruct {
    GENERATED_BODY()
    UPROPERTY()
    int32 TextsIndex = 0;
    UPROPERTY()
    int32 DialogueIndex = 0;
    UPROPERTY()
    int16 CharactersIndex = 0;
    UPROPERTY()
    int16 CharacterVariantsIndex = 0;
    UPROPERTY()
    int16 AudioTemplatesIndex = 0;
    UPROPERTY()
    int16 LipSyncAnimsIndex = 0;
    UPROPERTY()
    uint8 Type = 0;

    friend FArchive& operator<<(FArchive& Ar, FP2StoryDataStruct& Data) {
        return Ar << Data.TextsIndex << Data.DialogueIndex << Data.CharactersIndex << Data.CharacterVariantsIndex << Data.AudioTemplatesIndex
                  << Data.LipSyncAnimsIndex << Data.Type;
    }
};

//! CUE4Parse reads the packed story data after the properties without a game check.
UCLASS()
class UStoryData : public UObject {
    GENERATED_BODY()
public:
    explicit UStoryData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        Super::Serialize(Ar);
        if (!Ar.IsError())
            Ar << Data;
    }

    UPROPERTY()
    TArray<FP2StoryDataStruct> Data;
};

}
