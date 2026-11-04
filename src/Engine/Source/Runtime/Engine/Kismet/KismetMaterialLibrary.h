#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "KismetMaterialLibrary.generated.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;

UENUM()
enum class EMIDCreationFlags : uint8 {
    None = 0,
    Transient = 1,
};

UCLASS()
class UKismetMaterialLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static UMaterialInstanceDynamic* CreateDynamicMaterialInstance(UObject* WorldContextObject, UMaterialInterface* Parent, FName OptionalName,
                                                                   EMIDCreationFlags CreationFlags);
    UFUNCTION(BlueprintCallable)
    static void SetScalarParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, FName ParameterName, float ParameterValue);
    UFUNCTION(BlueprintCallable)
    static void SetVectorParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, FName ParameterName,
                                        const FLinearColor& ParameterValue);
    UFUNCTION(BlueprintCallable)
    static float GetScalarParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, FName ParameterName);
    UFUNCTION(BlueprintCallable)
    static FLinearColor GetVectorParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, FName ParameterName);
};

}
