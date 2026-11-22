#pragma once

#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"

#include "VAL_PremiumItemAsset.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EVAL_PremiumItemType : uint8 {
    Normal = 0,
    Premium = 1,
};

USTRUCT()
struct FPerBodyTypeConfig {
    GENERATED_BODY()
    UPROPERTY()
    EVAL_PremiumItemType ItemType = EVAL_PremiumItemType::Normal;
    UPROPERTY()
    FInstancedStruct Config;
};

//! Palia saves the configuration of each body type after the properties, as a struct named after the class.
UCLASS()
class UVAL_PremiumItemAsset : public UPrimaryDataAsset {
    GENERATED_BODY()
public:
    explicit UVAL_PremiumItemAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TArray<FPerBodyTypeConfig> PerBodyTypeConfig;
};

//! Palia saves the struct object of the options and then the options as values of that struct.
USTRUCT()
struct FVAL_CharacterCustomizationVariantOptionsArray {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> OptionStruct = nullptr;
    UPROPERTY()
    TArray<FInstancedStruct> Options;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FVAL_CharacterCustomizationVariantOptionsArray> : TStructOpsTypeTraitsBase2<FVAL_CharacterCustomizationVariantOptionsArray> {
    enum { WithSerializer = true };
};

}
