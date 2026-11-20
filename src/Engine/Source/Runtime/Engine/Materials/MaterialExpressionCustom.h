#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionCustom.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ECustomMaterialOutputType : int32 { CMOT_Float1, CMOT_Float2, CMOT_Float3, CMOT_Float4, CMOT_MaterialAttributes, CMOT_MAX };

USTRUCT()
struct FCustomInput {
    GENERATED_BODY()
    UPROPERTY()
    FName InputName;
    UPROPERTY()
    FExpressionInput Input;
};

UCLASS()
class UMaterialExpressionCustom : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString Code = FString("1");
    UPROPERTY()
    TEnumAsByte<ECustomMaterialOutputType> OutputType = CMOT_Float3;
    UPROPERTY()
    FString Description = FString("Custom");
    UPROPERTY()
    TArray<FCustomInput> Inputs;
};

}
