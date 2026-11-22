#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

#include "MaterialExpressionFunctionInput.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EFunctionInputType : int32 {
    FunctionInput_Scalar,
    FunctionInput_Vector2,
    FunctionInput_Vector3,
    FunctionInput_Vector4,
    FunctionInput_Texture2D,
    FunctionInput_TextureCube,
    FunctionInput_Texture2DArray,
    FunctionInput_VolumeTexture,
    FunctionInput_StaticBool,
    FunctionInput_MaterialAttributes,
    FunctionInput_TextureExternal,
    FunctionInput_Bool,
    FunctionInput_Substrate,
    FunctionInput_MAX,
};

UCLASS()
class UMaterialExpressionFunctionInput : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    FExpressionInput Preview;
    UPROPERTY()
    FName InputName = FName("In");
    UPROPERTY()
    FString Description;
    UPROPERTY()
    FGuid Id;
    UPROPERTY()
    TEnumAsByte<EFunctionInputType> InputType = FunctionInput_Vector3;
    UPROPERTY()
    FVector4f PreviewValue = FVector4f(0.0f, 0.0f, 0.0f, 0.0f);
    UPROPERTY()
    bool bUsePreviewValueAsDefault = false;
    UPROPERTY()
    int32 SortPriority = 0;
};

}
