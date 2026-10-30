#include "ue5parse/runtime/Engine/Materials/Material.h"

#include "ue5parse/runtime/Engine/Engine/Texture.h"

namespace ue5parse::runtime {

// UE's MaterialShared.cpp: the native serialization of the expression and material inputs (UE 5.3).

bool FExpressionInput::Serialize(FArchive& Ar) {
    SerializeExpressionInput(Ar, *this);
    return true;
}

namespace {

// UseConstant is a UBOOL, then the constant.
template <class TInput, class TConstant>
bool SerializeMaterialInput(FArchive& Ar, TInput& Input, TConstant& Constant) {
    SerializeExpressionInput(Ar, Input);
    Ar << Input.UseConstant << Constant;
    return true;
}

}

bool FColorMaterialInput::Serialize(FArchive& Ar) { return SerializeMaterialInput(Ar, *this, Constant); }
bool FScalarMaterialInput::Serialize(FArchive& Ar) { return SerializeMaterialInput(Ar, *this, Constant); }
bool FVectorMaterialInput::Serialize(FArchive& Ar) { return SerializeMaterialInput(Ar, *this, Constant); }
bool FShadingModelMaterialInput::Serialize(FArchive& Ar) { return SerializeMaterialInput(Ar, *this, Constant); }
bool FStrataMaterialInput::Serialize(FArchive& Ar) { return SerializeMaterialInput(Ar, *this, Constant); }

bool FVector2MaterialInput::Serialize(FArchive& Ar) {
    SerializeExpressionInput(Ar, *this);
    Ar << UseConstant << ConstantX << ConstantY;
    return true;
}

void UMaterial::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    // The inline shader maps follow; the runtime does not render.
    Ar.Seek(Ar.TotalSize());
}

bool UMaterial::GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly) const {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    return !bOveriddenOnly && GetCachedParameterValue(EMaterialParameterType::Scalar, ParameterInfo, Property, Value) &&
           GetPropertyValue(Property, Value, OutValue);
}

bool UMaterial::GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly) const {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(EMaterialParameterType::Vector, ParameterInfo, Property, Value)) return false;
    OutValue = *static_cast<const FLinearColor*>(Value);
    return true;
}

bool UMaterial::GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly) const {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(EMaterialParameterType::DoubleVector, ParameterInfo, Property, Value)) return false;
    OutValue = *static_cast<const FVector4d*>(Value);
    return true;
}

bool UMaterial::GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly) const {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(EMaterialParameterType::Texture, ParameterInfo, Property, Value)) return false;
    OutValue = Cast<UTexture>(static_cast<const FSoftObjectPtr*>(Value)->LoadSynchronous());
    return true;
}

bool UMaterial::GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly) const {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    return !bOveriddenOnly && GetCachedParameterValue(EMaterialParameterType::StaticSwitch, ParameterInfo, Property, Value) &&
           GetPropertyValue(Property, Value, OutValue);
}

}
