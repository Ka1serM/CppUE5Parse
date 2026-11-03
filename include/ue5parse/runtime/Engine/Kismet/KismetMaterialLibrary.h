#pragma once

#include "ue5parse/runtime/Core/Math/Color.h"
#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "KismetMaterialLibrary.generated.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;

// EMIDCreationFlags (UE's Kismet/KismetMaterialLibrary.h).
UENUM()
enum class EMIDCreationFlags : uint8 {
    None = 0,
    // The instance goes into the transient package, not into the world context object.
    Transient = 1,
};

// UKismetMaterialLibrary: the material functions of Blueprints (UE's Kismet/KismetMaterialLibrary.h). Only the functions
// the cooked game calls are declared.
UCLASS()
class UKismetMaterialLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static UMaterialInstanceDynamic* CreateDynamicMaterialInstance(UObject* WorldContextObject, UMaterialInterface* Parent, FName OptionalName,
                                                                   EMIDCreationFlags CreationFlags);
    // The world's values of a parameter collection. A parameter the collection does not have reads as zero and is
    // reported once.
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
