#pragma once

#include "ue5parse/runtime/Core/Containers/Map.h"
#include "ue5parse/runtime/Core/Math/Color.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "MaterialParameterCollection.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FCollectionParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    FName ParameterName;
    UPROPERTY()
    FGuid Id;
};

USTRUCT()
struct FCollectionScalarParameter : public FCollectionParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    float DefaultValue = 0;
};

USTRUCT()
struct FCollectionVectorParameter : public FCollectionParameterBase {
    GENERATED_BODY()
    UPROPERTY()
    FLinearColor DefaultValue;
};

// UMaterialParameterCollection: global parameters that materials read; each world holds their values (UE's
// Materials/MaterialParameterCollection.h).
UCLASS()
class UMaterialParameterCollection : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid StateId;
    UPROPERTY()
    TArray<FCollectionScalarParameter> ScalarParameters;
    UPROPERTY()
    TArray<FCollectionVectorParameter> VectorParameters;

    const FCollectionScalarParameter* GetScalarParameterByName(FName ParameterName) const;
    const FCollectionVectorParameter* GetVectorParameterByName(FName ParameterName) const;
};

// UMaterialParameterCollectionInstance: a world's values of a collection; a parameter without a value has its default
// (UE's Materials/MaterialParameterCollectionInstance.h).
UCLASS()
class UMaterialParameterCollectionInstance : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> Collection = nullptr;
    UPROPERTY(transient)
    TMap<FName, float> ScalarParameterValues;
    UPROPERTY(transient)
    TMap<FName, FLinearColor> VectorParameterValues;
    // A script set a parameter that the collection does not have; the message goes out once.
    bool bLoggedMissingParameterWarning = false;

    // False when the collection has no such parameter.
    bool SetScalarParameterValue(FName ParameterName, float ParameterValue);
    bool SetVectorParameterValue(FName ParameterName, const FLinearColor& ParameterValue);
    bool GetScalarParameterValue(FName ParameterName, float& OutParameterValue) const;
    bool GetVectorParameterValue(FName ParameterName, FLinearColor& OutParameterValue) const;
};

}
