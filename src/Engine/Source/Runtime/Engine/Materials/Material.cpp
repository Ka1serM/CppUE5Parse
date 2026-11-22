#include "Engine/Source/Runtime/Engine/Materials/Material.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionScalarParameter.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionStaticBoolParameter.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionTextureSampleParameter.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionVectorParameter.h"

namespace ue5parse::runtime {

bool FExpressionInput::Serialize(FArchive& Ar) {
    return SerializeExpressionInput(Ar, *this);
}

namespace {

template <class TExpression>
const TExpression* FindParameterExpression(const TArray<TObjectPtr<UMaterialExpression>>& Expressions, const FMaterialParameterInfo& ParameterInfo) {
    if (ParameterInfo.Association != GlobalParameter)
        return nullptr;
    for (const TObjectPtr<UMaterialExpression>& Expression : Expressions) {
        const TExpression* Parameter = Cast<TExpression>(Expression.Get());
        if (Parameter != nullptr && Parameter->ParameterName == ParameterInfo.Name)
            return Parameter;
    }
    return nullptr;
}

template <class TInput, class TConstant>
bool SerializeMaterialInput(FArchive& Ar, TInput& Input, TConstant& Constant) {
    if (!SerializeExpressionInput(Ar, Input))
        return false;
    Ar << Input.UseConstant << Constant;
    return true;
}

}

bool FColorMaterialInput::Serialize(FArchive& Ar) {
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MaterialInputUsesLinearColor, EGame::GAME_UE5_6))
        return SerializeMaterialInput(Ar, *this, Constant);
    FColor OldConstant;
    if (!SerializeMaterialInput(Ar, *this, OldConstant))
        return false;
    Constant = OldConstant.ReinterpretAsLinear();
    return true;
}
bool FScalarMaterialInput::Serialize(FArchive& Ar) {
    return SerializeMaterialInput(Ar, *this, Constant);
}
bool FVectorMaterialInput::Serialize(FArchive& Ar) {
    return SerializeMaterialInput(Ar, *this, Constant);
}

bool FVector2MaterialInput::Serialize(FArchive& Ar) {
    if (!SerializeExpressionInput(Ar, *this))
        return false;
    Ar << UseConstant << ConstantX << ConstantY;
    return true;
}

void UMaterial::Serialize(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(16);
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    SkipInlineShaderMaps(Ar);
}

//! Before UE 4.25 the layers are on the MaterialAttributeLayers expression, UE 4.25 to 4.27 cache only their functions.
bool UMaterial::GetMaterialLayers(FMaterialLayersFunctions& OutLayers) const {
    if (!HasCachedExpressionData()) {
        for (const TObjectPtr<UMaterialExpression>& Expression : Expressions) {
            const auto* Layers = Cast<UMaterialExpressionMaterialAttributeLayers>(Expression.Get());
            if (Layers == nullptr)
                continue;
            OutLayers = Layers->DefaultLayers;
            return true;
        }
        return false;
    }
    const FMaterialCachedExpressionData* Data = GetCachedExpressionData();
    if (Data == nullptr)
        return false;
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE5_0) {
        if (Data->DefaultLayers.Num() == 0)
            return false;
        OutLayers.Layers = Data->DefaultLayers;
        OutLayers.Blends = Data->DefaultLayerBlends;
        return true;
    }
    if (!Data->bHasMaterialLayers)
        return false;
    static_cast<FMaterialLayersFunctionsRuntimeData&>(OutLayers) = Data->MaterialLayers;
    return true;
}

const FMaterialCachedExpressionData* UMaterial::GetCachedExpressionData() const {
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE5_0)
        return &CachedExpressionData;
    return Super::GetCachedExpressionData();
}

bool UMaterial::GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly) const {
    if (!HasCachedExpressionData()) {
        const auto* Parameter = FindParameterExpression<UMaterialExpressionScalarParameter>(Expressions, ParameterInfo);
        if (bOveriddenOnly || Parameter == nullptr)
            return false;
        OutValue = Parameter->DefaultValue;
        return true;
    }
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    return !bOveriddenOnly && GetCachedParameterValue(EMaterialParameterType::Scalar, ParameterInfo, Property, Value)
           && GetPropertyValue(Property, Value, OutValue);
}

bool UMaterial::GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly) const {
    if (!HasCachedExpressionData()) {
        const auto* Parameter = FindParameterExpression<UMaterialExpressionVectorParameter>(Expressions, ParameterInfo);
        if (bOveriddenOnly || Parameter == nullptr)
            return false;
        OutValue = Parameter->DefaultValue;
        return true;
    }
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(EMaterialParameterType::Vector, ParameterInfo, Property, Value))
        return false;
    OutValue = *static_cast<const FLinearColor*>(Value);
    return true;
}

bool UMaterial::GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly) const {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(EMaterialParameterType::DoubleVector, ParameterInfo, Property, Value))
        return false;
    OutValue = *static_cast<const FVector4d*>(Value);
    return true;
}

bool UMaterial::GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly) const {
    if (!HasCachedExpressionData()) {
        const auto* Parameter = FindParameterExpression<UMaterialExpressionTextureSampleParameter>(Expressions, ParameterInfo);
        if (bOveriddenOnly || Parameter == nullptr)
            return false;
        OutValue = Parameter->Texture;
        return true;
    }
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(EMaterialParameterType::Texture, ParameterInfo, Property, Value))
        return false;
    return GetCachedTextureValue(Property, Value, OutValue);
}

bool UMaterial::GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly) const {
    if (!HasCachedExpressionData()) {
        const auto* Parameter = FindParameterExpression<UMaterialExpressionStaticBoolParameter>(Expressions, ParameterInfo);
        if (bOveriddenOnly || Parameter == nullptr)
            return false;
        OutValue = Parameter->DefaultValue;
        return true;
    }
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    return !bOveriddenOnly && GetCachedParameterValue(EMaterialParameterType::StaticSwitch, ParameterInfo, Property, Value)
           && GetPropertyValue(Property, Value, OutValue);
}

}
