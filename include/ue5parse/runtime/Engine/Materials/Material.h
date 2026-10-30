#pragma once

#include "ue5parse/runtime/Engine/Materials/MaterialExpression.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInterface.h"

#include "Material.generated.h"

namespace ue5parse::runtime {

enum EMaterialDomain : int32 { MD_Surface, MD_DeferredDecal, MD_LightFunction, MD_Volume, MD_PostProcess, MD_UI, MD_RuntimeVirtualTexture, MD_MAX };

// FMaterialInput: the reflected stand-in of UE's FMaterialInput<InputType>: an expression input with a constant used
// when nothing is connected. Each typed input adds its constant.
USTRUCT()
struct FMaterialInput {
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
};

USTRUCT()
struct FColorMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    FColor Constant;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FScalarMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    float Constant = 0;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FVectorMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    FVector3f Constant = FVector3f::ZeroVector;

    bool Serialize(FArchive& Ar);
};

// The constant is two reflected floats here, one FVector2f in UE's C++ type.
USTRUCT()
struct FVector2MaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    float ConstantX = 0;
    UPROPERTY()
    float ConstantY = 0;

    bool Serialize(FArchive& Ar);
};

// The shading model and Strata inputs: a uint32 constant that is not reflected.
USTRUCT()
struct FShadingModelMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    bool UseConstant = false;
    uint32 Constant = 0;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FStrataMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    bool UseConstant = false;
    uint32 Constant = 0;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FColorMaterialInput> : TStructOpsTypeTraitsBase2<FColorMaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FScalarMaterialInput> : TStructOpsTypeTraitsBase2<FScalarMaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FVectorMaterialInput> : TStructOpsTypeTraitsBase2<FVectorMaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FVector2MaterialInput> : TStructOpsTypeTraitsBase2<FVector2MaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FShadingModelMaterialInput> : TStructOpsTypeTraitsBase2<FShadingModelMaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FStrataMaterialInput> : TStructOpsTypeTraitsBase2<FStrataMaterialInput> {
    enum { WithSerializer = true };
};

// UMaterial: a base material (UE's Materials/Material.h). Its parameter defaults are in the cached expression data;
// its shader maps are not read.
UCLASS()
class UMaterial : public UMaterialInterface {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysMaterial = nullptr;
    UPROPERTY()
    TEnumAsByte<EMaterialDomain> MaterialDomain = MD_Surface;
    UPROPERTY()
    TEnumAsByte<EBlendMode> BlendMode = BLEND_Opaque;
    UPROPERTY()
    TEnumAsByte<EMaterialShadingModel> ShadingModel = MSM_DefaultLit;
    UPROPERTY()
    float OpacityMaskClipValue = 0.3333f;
    UPROPERTY()
    bool TwoSided = false;
    UPROPERTY()
    bool bIsMasked = false;
    UPROPERTY()
    bool bUsedWithSkeletalMesh = false;

    void Serialize(FArchive& Ar) override;
    UMaterial* GetMaterial() override { return this; }
    const UMaterial* GetMaterial() const override { return this; }
    bool GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const override;
    bool GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const override;
    bool GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly = false) const override;
    bool GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly = false) const override;
    bool GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly = false) const override;
    TEnumAsByte<EBlendMode> GetBlendMode() const override { return BlendMode; }
    bool IsTwoSided() const override { return TwoSided; }
    bool IsMasked() const override { return BlendMode == BLEND_Masked || bIsMasked; }
    float GetOpacityMaskClipValue() const override { return OpacityMaskClipValue; }
    EMaterialShadingModel GetShadingModel() const override { return ShadingModel.GetValue(); }
    UPhysicalMaterial* GetPhysicalMaterial() const override { return PhysMaterial; }
};

}
