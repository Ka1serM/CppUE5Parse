#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

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
    bool bLoggedMissingParameterWarning = false;

    bool SetScalarParameterValue(FName ParameterName, float ParameterValue);
    bool SetVectorParameterValue(FName ParameterName, const FLinearColor& ParameterValue);
    bool GetScalarParameterValue(FName ParameterName, float& OutParameterValue) const;
    bool GetVectorParameterValue(FName ParameterName, FLinearColor& OutParameterValue) const;
};

}
