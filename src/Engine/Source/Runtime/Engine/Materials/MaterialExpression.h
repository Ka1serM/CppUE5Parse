#pragma once

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MaterialExpression.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FExpressionInput {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> Expression = nullptr;
    UPROPERTY()
    int32 OutputIndex = 0;
    UPROPERTY()
    FName InputName;
    UPROPERTY()
    int32 Mask = 0;
    UPROPERTY()
    int32 MaskR = 0;
    UPROPERTY()
    int32 MaskG = 0;
    UPROPERTY()
    int32 MaskB = 0;
    UPROPERTY()
    int32 MaskA = 0;
    UPROPERTY()
    FName ExpressionName;
    UPROPERTY()
    int32 GCC64_Padding = 0;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FExpressionInput> : TStructOpsTypeTraitsBase2<FExpressionInput> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMaterialAttributesInput : public FExpressionInput {
    GENERATED_BODY()
    UPROPERTY()
    uint64 PropertyConnectedMask = 0;

    bool Serialize(FArchive& Ar) { return FExpressionInput::Serialize(Ar); }
};

template <>
struct TStructOpsTypeTraits<FMaterialAttributesInput> : TStructOpsTypeTraitsBase2<FMaterialAttributesInput> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FExpressionOutput {
    GENERATED_BODY()
    UPROPERTY()
    FName OutputName;
    UPROPERTY()
    int32 Mask = 0;
    UPROPERTY()
    int32 MaskR = 0;
    UPROPERTY()
    int32 MaskG = 0;
    UPROPERTY()
    int32 MaskB = 0;
    UPROPERTY()
    int32 MaskA = 0;
};

class UMaterial;
class UMaterialFunction;

UCLASS(Abstract)
class UMaterialExpression : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UMaterial> Material = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialFunction> Function = nullptr;
    UPROPERTY()
    bool bIsParameterExpression = false;
    UPROPERTY()
    TArray<FExpressionOutput> Outputs;
};

template <class TInput>
bool SerializeExpressionInput(FArchive& Ar, TInput& Input) {
    if (!Ar.CustomVerAtLeast(FCoreObjectVersion::GUID, FCoreObjectVersion::MaterialInputNativeSerialize, EGame::GAME_UE4_12))
        return false;
    //! Cooked packages before 5.1 store the expression name instead of the expression.
    const bool bHasExpressionName = Ar.GetGame() < EGame::GAME_UE5_1;
    if (!bHasExpressionName)
        Ar << Input.Expression;
    Ar << Input.OutputIndex;
    if (Ar.CustomVerAtLeast(FFrameworkObjectVersion::GUID, FFrameworkObjectVersion::PinsStoreFName, EGame::GAME_UE4_19)) {
        Ar << Input.InputName;
    } else {
        FString InputNameStr;
        Ar << InputNameStr;
        Input.InputName = MakeName(InputNameStr);
    }
    Ar << Input.Mask << Input.MaskR << Input.MaskG << Input.MaskB << Input.MaskA;
    if (bHasExpressionName)
        Ar << Input.ExpressionName;
    return true;
}

}
