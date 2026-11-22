#include "Engine/Source/Runtime/Engine/Materials/MaterialInstance.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"
#include "Engine/Source/Runtime/Engine/Materials/Material.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceConstant.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceDynamic.h"

namespace ue5parse::runtime {

namespace {

//! GTA: The Trilogy - Definitive Edition serializes the material parameter values as raw fields instead of tagged properties.
bool SerializesRawParameterValues(const FArchive& Ar) {
    return Ar.GetGame() == EGame::GAME_GTATheTrilogyDefinitiveEdition;
}

void SerializeRawParameterInfo(FArchive& Ar, FMaterialParameterInfo& Info) {
    uint8 Association = 0;
    Ar << Info.Name << Association << Info.Index;
    Info.Association = TEnumAsByte<EMaterialParameterAssociation>(Association);
}

constexpr int32 MaxParentDepth = 64;

bool GetMaterialParameterValue(const UMaterial& Material, const FMaterialParameterInfo& ParameterInfo, float& OutValue) {
    return Material.GetScalarParameterValue(ParameterInfo, OutValue);
}
bool GetMaterialParameterValue(const UMaterial& Material, const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue) {
    return Material.GetVectorParameterValue(ParameterInfo, OutValue);
}
bool GetMaterialParameterValue(const UMaterial& Material, const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue) {
    return Material.GetDoubleVectorParameterValue(ParameterInfo, OutValue);
}
bool GetMaterialParameterValue(const UMaterial& Material, const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue) {
    return Material.GetTextureParameterValue(ParameterInfo, OutValue);
}
bool GetMaterialParameterValue(const UMaterial& Material, const FMaterialParameterInfo& ParameterInfo, bool& OutValue) {
    return Material.GetStaticSwitchParameterValue(ParameterInfo, OutValue);
}

template <class TParameterValue>
const TParameterValue* FindOverride(const TArray<TParameterValue>& Values, const FMaterialParameterInfo& ParameterInfo) {
    for (const TParameterValue& Value : Values)
        if (Value.ParameterInfo == ParameterInfo)
            return &Value;
    return nullptr;
}

template <class TParameterValue>
void copyLegacyNames(TArray<TParameterValue>& Values) {
    for (TParameterValue& Value : Values)
        Value.ParameterInfo.Name = Value.ParameterName;
}

template <class TParameterValue>
TParameterValue* FindOverride(TArray<TParameterValue>& Values, const FMaterialParameterInfo& ParameterInfo) {
    for (TParameterValue& Value : Values)
        if (Value.ParameterInfo == ParameterInfo)
            return &Value;
    return nullptr;
}

}

void UMaterialInstance::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    //! UE 4.18 and older identify overrides by ParameterName; UE 4.19 copies it to ParameterInfo.Name on load.
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MaterialAttributeLayerParameters, EGame::GAME_UE4_19)) {
        copyLegacyNames(ScalarParameterValues);
        copyLegacyNames(VectorParameterValues);
        copyLegacyNames(TextureParameterValues);
        copyLegacyNames(FontParameterValues);
    }
    bool bSavedCachedData = false;
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::MaterialSavedCachedData, EGame::GAME_UE5_0))
        Ar << bSavedCachedData;
    if (bSavedCachedData)
        FMaterialInstanceCachedData::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&CachedData));
    if (Ar.IsError())
        return;
    if (bHasStaticPermutationResource) {
        //! UE 4.18 and older write the static parameters of the instance before its shader maps.
        if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MaterialAttributeLayerParameters, EGame::GAME_UE4_19))
            SkipLegacyStaticParameterSet(Ar);
        SkipInlineShaderMaps(Ar);
        return;
    }
    if (Ar.GetGame() == EGame::GAME_Valorant)
        Ar.SkipBytes(8);
}

UMaterial* UMaterialInstance::GetMaterial() {
    UMaterialInterface* Current = Parent;
    for (int32 Depth = 0; Current != nullptr && Depth < MaxParentDepth; Depth++) {
        if (UMaterial* Material = Cast<UMaterial>(Current))
            return Material;
        UMaterialInstance* Instance = Cast<UMaterialInstance>(Current);
        Current = Instance ? Instance->Parent : nullptr;
    }
    return nullptr;
}

const UMaterial* UMaterialInstance::GetMaterial() const {
    return const_cast<UMaterialInstance*>(this)->GetMaterial();
}

const FMaterialCachedExpressionData* UMaterialInstance::GetCachedExpressionData() const {
    const UMaterialInstance* Instance = this;
    for (int32 Depth = 0; Depth < MaxParentDepth; Depth++) {
        if (Instance->CachedExpressionData != nullptr)
            return Instance->CachedExpressionData;
        if (Instance->Parent == nullptr)
            return nullptr;
        const UMaterialInstance* Next = Cast<UMaterialInstance>(Instance->Parent);
        if (Next == nullptr)
            return Instance->Parent->GetCachedExpressionData();
        Instance = Next;
    }
    return nullptr;
}

template <class TValue, class FindOverride>
bool UMaterialInstance::GetInstanceParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, TValue& OutValue,
                                                  bool bOveriddenOnly, FindOverride&& Find) const {
    const UMaterialInstance* Instance = this;
    for (int32 Depth = 0; Instance != nullptr && Depth < MaxParentDepth; Depth++) {
        if (Find(*Instance, OutValue))
            return true;
        Instance = Cast<UMaterialInstance>(Instance->Parent);
    }
    if (!HasCachedExpressionData()) {
        const UMaterial* Material = GetMaterial();
        return !bOveriddenOnly && Material != nullptr && GetMaterialParameterValue(*Material, ParameterInfo, OutValue);
    }
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(Type, ParameterInfo, Property, Value))
        return false;
    if constexpr (std::is_same_v<TValue, UTexture*>) {
        return GetCachedTextureValue(Property, Value, OutValue);
    } else if constexpr (std::is_same_v<TValue, FLinearColor> || std::is_same_v<TValue, FVector4d>) {
        OutValue = *static_cast<const TValue*>(Value);
        return true;
    } else {
        return GetPropertyValue(Property, Value, OutValue);
    }
}

bool UMaterialInstance::GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::Scalar, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, float& Out) {
                                         const FScalarParameterValue* Value = FindOverride(Instance.ScalarParameterValues, ParameterInfo);
                                         if (Value)
                                             Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

bool UMaterialInstance::GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::Vector, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, FLinearColor& Out) {
                                         const FVectorParameterValue* Value = FindOverride(Instance.VectorParameterValues, ParameterInfo);
                                         if (Value)
                                             Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

bool UMaterialInstance::GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::DoubleVector, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, FVector4d& Out) {
                                         const FDoubleVectorParameterValue* Value = FindOverride(Instance.DoubleVectorParameterValues, ParameterInfo);
                                         if (Value)
                                             Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

bool UMaterialInstance::GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::Texture, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, UTexture*& Out) {
                                         const FTextureParameterValue* Value = FindOverride(Instance.TextureParameterValues, ParameterInfo);
                                         if (Value)
                                             Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

const TArray<FStaticSwitchParameter>& UMaterialInstance::GetStaticSwitchParameters() const {
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE5_1)
        return StaticParameters.StaticSwitchParameters;
    return StaticParametersRuntime.StaticSwitchParameters;
}

bool UMaterialInstance::GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::StaticSwitch, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, bool& Out) {
                                         for (const FStaticSwitchParameter& Switch : Instance.GetStaticSwitchParameters()) {
                                             if (!(Switch.ParameterInfo == ParameterInfo) || !Switch.bOverride)
                                                 continue;
                                             Out = Switch.Value;
                                             return true;
                                         }
                                         return false;
                                     });
}

//! UE 4.19 to 4.27 store overridden layers as a static parameter, UE 5.0 in StaticParameters and UE 5.1 in StaticParametersRuntime.
bool UMaterialInstance::GetMaterialLayers(FMaterialLayersFunctions& OutLayers) const {
    const UMaterialInstance* Instance = this;
    for (int32 Depth = 0; Instance != nullptr && Depth < MaxParentDepth; Depth++) {
        if (Instance->StaticParametersRuntime.bHasMaterialLayers) {
            static_cast<FMaterialLayersFunctionsRuntimeData&>(OutLayers) = Instance->StaticParametersRuntime.MaterialLayers;
            return true;
        }
        if (Instance->StaticParameters.bHasMaterialLayers) {
            OutLayers = Instance->StaticParameters.MaterialLayers;
            return true;
        }
        for (const FStaticMaterialLayersParameter& Parameter : Instance->StaticParameters.MaterialLayersParameters) {
            if (!Parameter.bOverride)
                continue;
            OutLayers = Parameter.Value;
            return true;
        }
        Instance = Cast<UMaterialInstance>(Instance->Parent);
    }
    const UMaterial* Material = GetMaterial();
    return Material != nullptr && Material->GetMaterialLayers(OutLayers);
}

TEnumAsByte<EBlendMode> UMaterialInstance::GetBlendMode() const {
    if (BasePropertyOverrides.bOverride_BlendMode)
        return BasePropertyOverrides.BlendMode;
    return Parent && Parent != this ? Parent->GetBlendMode() : TEnumAsByte<EBlendMode>(BLEND_Opaque);
}

bool UMaterialInstance::IsTwoSided() const {
    if (BasePropertyOverrides.bOverride_TwoSided)
        return BasePropertyOverrides.TwoSided;
    return Parent && Parent != this && Parent->IsTwoSided();
}

bool UMaterialInstance::IsMasked() const {
    const EBlendMode BlendMode = GetBlendMode();
    if (BlendMode == BLEND_Masked)
        return true;
    return Parent && Parent != this && BlendMode == Parent->GetBlendMode() && Parent->IsMasked();
}

float UMaterialInstance::GetOpacityMaskClipValue() const {
    if (BasePropertyOverrides.bOverride_OpacityMaskClipValue)
        return BasePropertyOverrides.OpacityMaskClipValue;
    return Parent && Parent != this ? Parent->GetOpacityMaskClipValue() : 0.0f;
}

EMaterialShadingModel UMaterialInstance::GetShadingModel() const {
    if (BasePropertyOverrides.bOverride_ShadingModel)
        return BasePropertyOverrides.ShadingModel.GetValue();
    return Parent && Parent != this ? Parent->GetShadingModel() : MSM_DefaultLit;
}

UPhysicalMaterial* UMaterialInstance::GetPhysicalMaterial() const {
    if (PhysMaterial != nullptr)
        return PhysMaterial;
    return Parent && Parent != this ? Parent->GetPhysicalMaterial() : nullptr;
}

float UMaterialInstanceConstant::K2_GetScalarParameterValue(FName ParameterName) {
    float Result = 0.f;
    if (!ParameterName.isNone())
        GetScalarParameterValue(FMaterialParameterInfo(ParameterName), Result);
    return Result;
}

UTexture* UMaterialInstanceConstant::K2_GetTextureParameterValue(FName ParameterName) {
    UTexture* Result = nullptr;
    if (!ParameterName.isNone())
        GetTextureParameterValue(FMaterialParameterInfo(ParameterName), Result);
    return Result;
}

FLinearColor UMaterialInstanceConstant::K2_GetVectorParameterValue(FName ParameterName) {
    FLinearColor Result(0, 0, 0, 0);
    if (!ParameterName.isNone())
        GetVectorParameterValue(FMaterialParameterInfo(ParameterName), Result);
    return Result;
}

UMaterialInstanceDynamic* UMaterialInstanceDynamic::Create(UMaterialInterface* ParentMaterial, UObject* InOuter, FName Name) {
    UObject* Outer = InOuter ? InOuter : GetTransientPackage();
    UMaterialInstanceDynamic* Instance = NewObject<UMaterialInstanceDynamic>(Outer, UMaterialInstanceDynamic::StaticClass(), Name);
    Instance->Parent = ParentMaterial;
    return Instance;
}

void UMaterialInstanceDynamic::SetScalarParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, float Value) {
    if (FScalarParameterValue* Existing = FindOverride(ScalarParameterValues, ParameterInfo)) {
        Existing->ParameterValue = Value;
        return;
    }
    FScalarParameterValue& Added = ScalarParameterValues[ScalarParameterValues.AddDefaulted()];
    Added.ParameterInfo = ParameterInfo;
    Added.ParameterValue = Value;
}

void UMaterialInstanceDynamic::SetScalarParameterValue(FName ParameterName, float Value) {
    SetScalarParameterValueByInfo(FMaterialParameterInfo(ParameterName), Value);
}

void UMaterialInstanceDynamic::SetVectorParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, FLinearColor Value) {
    if (FVectorParameterValue* Existing = FindOverride(VectorParameterValues, ParameterInfo)) {
        Existing->ParameterValue = Value;
        return;
    }
    FVectorParameterValue& Added = VectorParameterValues[VectorParameterValues.AddDefaulted()];
    Added.ParameterInfo = ParameterInfo;
    Added.ParameterValue = Value;
}

void UMaterialInstanceDynamic::SetVectorParameterValue(FName ParameterName, FLinearColor Value) {
    SetVectorParameterValueByInfo(FMaterialParameterInfo(ParameterName), Value);
}

void UMaterialInstanceDynamic::SetDoubleVectorParameterValue(FName ParameterName, FVector4 Value) {
    const FMaterialParameterInfo ParameterInfo(ParameterName);
    if (FDoubleVectorParameterValue* Existing = FindOverride(DoubleVectorParameterValues, ParameterInfo)) {
        Existing->ParameterValue = Value;
        return;
    }
    FDoubleVectorParameterValue& Added = DoubleVectorParameterValues[DoubleVectorParameterValues.AddDefaulted()];
    Added.ParameterInfo = ParameterInfo;
    Added.ParameterValue = Value;
}

void UMaterialInstanceDynamic::SetTextureParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, UTexture* Value) {
    if (FTextureParameterValue* Existing = FindOverride(TextureParameterValues, ParameterInfo)) {
        Existing->ParameterValue = Value;
        return;
    }
    FTextureParameterValue& Added = TextureParameterValues[TextureParameterValues.AddDefaulted()];
    Added.ParameterInfo = ParameterInfo;
    Added.ParameterValue = Value;
}

void UMaterialInstanceDynamic::SetTextureParameterValue(FName ParameterName, UTexture* Value) {
    SetTextureParameterValueByInfo(FMaterialParameterInfo(ParameterName), Value);
}

float UMaterialInstanceDynamic::K2_GetScalarParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo) {
    float Result = 0.f;
    GetScalarParameterValue(ParameterInfo, Result);
    return Result;
}

float UMaterialInstanceDynamic::K2_GetScalarParameterValue(FName ParameterName) {
    return K2_GetScalarParameterValueByInfo(FMaterialParameterInfo(ParameterName));
}

UTexture* UMaterialInstanceDynamic::K2_GetTextureParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo) {
    UTexture* Result = nullptr;
    GetTextureParameterValue(ParameterInfo, Result);
    return Result;
}

UTexture* UMaterialInstanceDynamic::K2_GetTextureParameterValue(FName ParameterName) {
    return K2_GetTextureParameterValueByInfo(FMaterialParameterInfo(ParameterName));
}

FLinearColor UMaterialInstanceDynamic::K2_GetVectorParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo) {
    FLinearColor Result(0, 0, 0, 0);
    GetVectorParameterValue(ParameterInfo, Result);
    return Result;
}

FLinearColor UMaterialInstanceDynamic::K2_GetVectorParameterValue(FName ParameterName) {
    return K2_GetVectorParameterValueByInfo(FMaterialParameterInfo(ParameterName));
}

void UMaterialInstanceDynamic::CopyParameterOverrides(UMaterialInstance* MaterialInstance) {
    ClearParameterValues();
    if (MaterialInstance == nullptr)
        return;
    ScalarParameterValues = MaterialInstance->ScalarParameterValues;
    VectorParameterValues = MaterialInstance->VectorParameterValues;
    DoubleVectorParameterValues = MaterialInstance->DoubleVectorParameterValues;
    TextureParameterValues = MaterialInstance->TextureParameterValues;
}

void UMaterialInstanceDynamic::K2_CopyMaterialInstanceParameters(UMaterialInterface* Source, bool bQuickParametersOnly) {
    ClearParameterValues();
    if (Source == nullptr || Source == this)
        return;
    TArray<FMaterialParameterInfo> Infos;
    Source->GetAllParameterInfoOfType(EMaterialParameterType::Scalar, Infos);
    for (const FMaterialParameterInfo& Info : Infos) {
        float Value = 0;
        if (Source->GetScalarParameterValue(Info, Value))
            SetScalarParameterValueByInfo(Info, Value);
    }
    Infos.Reset();
    Source->GetAllParameterInfoOfType(EMaterialParameterType::Vector, Infos);
    for (const FMaterialParameterInfo& Info : Infos) {
        FLinearColor Value;
        if (Source->GetVectorParameterValue(Info, Value))
            SetVectorParameterValueByInfo(Info, Value);
    }
    if (bQuickParametersOnly)
        return;
    Infos.Reset();
    Source->GetAllParameterInfoOfType(EMaterialParameterType::Texture, Infos);
    for (const FMaterialParameterInfo& Info : Infos) {
        UTexture* Value = nullptr;
        if (Source->GetTextureParameterValue(Info, Value))
            SetTextureParameterValueByInfo(Info, Value);
    }
}

void UMaterialInstanceDynamic::ClearParameterValues() {
    ScalarParameterValues.Empty();
    VectorParameterValues.Empty();
    DoubleVectorParameterValues.Empty();
    TextureParameterValues.Empty();
}

void UMaterialInstanceDynamic::K2_InterpolateMaterialInstanceParams(UMaterialInstance* SourceA, UMaterialInstance* SourceB, float Alpha) {
    if (SourceA == nullptr || SourceB == nullptr)
        return;
    for (const FScalarParameterValue& A : SourceA->ScalarParameterValues) {
        float B = 0;
        if (SourceB->GetScalarParameterValue(A.ParameterInfo, B))
            SetScalarParameterValueByInfo(A.ParameterInfo, FMath::Lerp(A.ParameterValue, B, Alpha));
    }
    for (const FVectorParameterValue& A : SourceA->VectorParameterValues) {
        FLinearColor B;
        if (SourceB->GetVectorParameterValue(A.ParameterInfo, B))
            SetVectorParameterValueByInfo(A.ParameterInfo, A.ParameterValue + (B - A.ParameterValue) * Alpha);
    }
}

bool FScalarParameterValue::Serialize(FArchive& Ar) {
    if (!SerializesRawParameterValues(Ar))
        return false;
    SerializeRawParameterInfo(Ar, ParameterInfo);
    Ar << ParameterValue << ExpressionGUID;
    return true;
}

bool FVectorParameterValue::Serialize(FArchive& Ar) {
    if (!SerializesRawParameterValues(Ar))
        return false;
    SerializeRawParameterInfo(Ar, ParameterInfo);
    Ar << ParameterValue << ExpressionGUID;
    return true;
}

bool FTextureParameterValue::Serialize(FArchive& Ar) {
    if (!SerializesRawParameterValues(Ar))
        return false;
    SerializeRawParameterInfo(Ar, ParameterInfo);
    Ar << ParameterValue << ExpressionGUID;
    return true;
}

}
