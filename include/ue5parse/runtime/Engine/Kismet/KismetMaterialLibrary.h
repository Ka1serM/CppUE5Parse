#pragma once

#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "KismetMaterialLibrary.generated.h"

namespace ue5parse::runtime {

class UMaterialInstanceDynamic;
class UMaterialInterface;

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
};

}
