#include "ue5parse/runtime/Engine/Materials/MaterialInstance.h"

#include "ue5parse/runtime/Engine/Engine/Texture.h"
#include "ue5parse/runtime/Engine/Materials/Material.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInstanceConstant.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInstanceDynamic.h"
#include "ue5parse/provider/FileProvider.h"

// UE's MaterialInstance.cpp, MaterialInstanceConstant.cpp and MaterialInstanceDynamic.cpp (UE 5.3): overrides first,
// then the parent.

namespace ue5parse::runtime {

namespace {

// The parent chain is bounded: a cycle in broken data must not hang.
constexpr int32 MaxParentDepth = 64;

template <class TParameterValue>
const TParameterValue* FindOverride(const TArray<TParameterValue>& Values, const FMaterialParameterInfo& ParameterInfo) {
    for (const TParameterValue& Value : Values)
        if (Value.ParameterInfo == ParameterInfo) return &Value;
    return nullptr;
}

template <class TParameterValue>
TParameterValue* FindOverride(TArray<TParameterValue>& Values, const FMaterialParameterInfo& ParameterInfo) {
    for (TParameterValue& Value : Values)
        if (Value.ParameterInfo == ParameterInfo) return &Value;
    return nullptr;
}

}

void UMaterialInstance::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetRemaining() < 4) return;
    // FMaterialInstanceCachedData, saved with cooked instances.
    bool bSavedCachedData = false;
    Ar << bSavedCachedData;
    if (bSavedCachedData) {
        if (const UScriptStruct* Struct = FindNativeStruct(FName("MaterialInstanceCachedData"))) {
            FInstancedStruct CachedData;
            CachedData.InitializeAs(Struct);
            Struct->SerializeItem(Ar, CachedData.GetMutableMemory());
        }
    }
    if (Ar.IsError()) return;
    if (bHasStaticPermutationResource) {
        // The inline shader maps follow; the runtime does not render.
        Ar.Seek(Ar.TotalSize());
        return;
    }
    if (Ar.HasGameQuirk(QUIRK_MaterialInstancePadding)) Ar.SkipBytes(8);
}

UMaterial* UMaterialInstance::GetMaterial() {
    UMaterialInterface* Current = Parent;
    for (int32 Depth = 0; Current != nullptr && Depth < MaxParentDepth; Depth++) {
        if (UMaterial* Material = Cast<UMaterial>(Current)) return Material;
        UMaterialInstance* Instance = Cast<UMaterialInstance>(Current);
        Current = Instance ? Instance->Parent : nullptr;
    }
    return nullptr;
}

const UMaterial* UMaterialInstance::GetMaterial() const { return const_cast<UMaterialInstance*>(this)->GetMaterial(); }

const void* UMaterialInstance::GetCachedExpressionData() const {
    const UMaterialInstance* Instance = this;
    for (int32 Depth = 0; Depth < MaxParentDepth; Depth++) {
        if (Instance->CachedExpressionData != nullptr) return Instance->CachedExpressionData;
        if (Instance->Parent == nullptr) return nullptr;
        const UMaterialInstance* Next = Cast<UMaterialInstance>(Instance->Parent);
        if (Next == nullptr) return Instance->Parent->GetCachedExpressionData();
        Instance = Next;
    }
    return nullptr;
}

// UMaterialInstance::GetMaterialParameterValue: the nearest instance's override, else the default of the chain's cached
// expression data.
template <class TValue, class FindOverride>
bool UMaterialInstance::GetInstanceParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, TValue& OutValue,
                                                  bool bOveriddenOnly, FindOverride&& Find) const {
    const UMaterialInstance* Instance = this;
    for (int32 Depth = 0; Instance != nullptr && Depth < MaxParentDepth; Depth++) {
        if (Find(*Instance, OutValue)) return true;
        Instance = Cast<UMaterialInstance>(Instance->Parent);
    }
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    if (bOveriddenOnly || !GetCachedParameterValue(Type, ParameterInfo, Property, Value)) return false;
    if constexpr (std::is_same_v<TValue, UTexture*>) {
        OutValue = Cast<UTexture>(static_cast<const FSoftObjectPtr*>(Value)->LoadSynchronous());
        return true;
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
                                         if (Value) Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

bool UMaterialInstance::GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::Vector, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, FLinearColor& Out) {
                                         const FVectorParameterValue* Value = FindOverride(Instance.VectorParameterValues, ParameterInfo);
                                         if (Value) Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

bool UMaterialInstance::GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::DoubleVector, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, FVector4d& Out) {
                                         const FDoubleVectorParameterValue* Value = FindOverride(Instance.DoubleVectorParameterValues, ParameterInfo);
                                         if (Value) Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

bool UMaterialInstance::GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::Texture, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, UTexture*& Out) {
                                         const FTextureParameterValue* Value = FindOverride(Instance.TextureParameterValues, ParameterInfo);
                                         if (Value) Out = Value->ParameterValue;
                                         return Value != nullptr;
                                     });
}

TArray<FStaticSwitchParameter> UMaterialInstance::GetStaticSwitchParameters() const {
    TArray<FStaticSwitchParameter> Result;
    const FStructProperty* StaticParameters = FindFProperty<FStructProperty>(GetClass(), FName("StaticParametersRuntime"));
    if (StaticParameters == nullptr) return Result;
    GetPropertyValue_InContainer(StaticParameters->Struct, StaticParameters->ContainerPtrToValuePtr<void>(this), FName("StaticSwitchParameters"), Result);
    return Result;
}

bool UMaterialInstance::GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly) const {
    return GetInstanceParameterValue(EMaterialParameterType::StaticSwitch, ParameterInfo, OutValue, bOveriddenOnly,
                                     [&](const UMaterialInstance& Instance, bool& Out) {
                                         for (const FStaticSwitchParameter& Switch : Instance.GetStaticSwitchParameters()) {
                                             if (!(Switch.ParameterInfo == ParameterInfo) || !Switch.bOverride) continue;
                                             Out = Switch.Value;
                                             return true;
                                         }
                                         return false;
                                     });
}

template <class T>
bool UMaterialInstance::GetBasePropertyOverride(FName OverrideFlag, FName Member, T& OutValue) const {
    const FStructProperty* Overrides = FindFProperty<FStructProperty>(GetClass(), FName("BasePropertyOverrides"));
    if (Overrides == nullptr) return false;
    const void* Data = Overrides->ContainerPtrToValuePtr<void>(this);
    bool bOverride = false;
    if (!GetPropertyValue_InContainer(Overrides->Struct, Data, OverrideFlag, bOverride) || !bOverride) return false;
    return GetPropertyValue_InContainer(Overrides->Struct, Data, Member, OutValue);
}

TEnumAsByte<EBlendMode> UMaterialInstance::GetBlendMode() const {
    uint8 BlendMode = 0;
    if (GetBasePropertyOverride(FName("bOverride_BlendMode"), FName("BlendMode"), BlendMode)) return static_cast<EBlendMode>(BlendMode);
    return Parent && Parent != this ? Parent->GetBlendMode() : TEnumAsByte<EBlendMode>(BLEND_Opaque);
}

bool UMaterialInstance::IsTwoSided() const {
    bool bTwoSided = false;
    if (GetBasePropertyOverride(FName("bOverride_TwoSided"), FName("TwoSided"), bTwoSided)) return bTwoSided;
    return Parent && Parent != this && Parent->IsTwoSided();
}

bool UMaterialInstance::IsMasked() const {
    const EBlendMode BlendMode = GetBlendMode();
    if (BlendMode == BLEND_Masked) return true;
    return Parent && Parent != this && BlendMode == Parent->GetBlendMode() && Parent->IsMasked();
}

float UMaterialInstance::GetOpacityMaskClipValue() const {
    float Value = 0;
    if (GetBasePropertyOverride(FName("bOverride_OpacityMaskClipValue"), FName("OpacityMaskClipValue"), Value)) return Value;
    return Parent && Parent != this ? Parent->GetOpacityMaskClipValue() : 0.0f;
}

EMaterialShadingModel UMaterialInstance::GetShadingModel() const {
    uint8 ShadingModel = 0;
    if (GetBasePropertyOverride(FName("bOverride_ShadingModel"), FName("ShadingModel"), ShadingModel)) return static_cast<EMaterialShadingModel>(ShadingModel);
    return Parent && Parent != this ? Parent->GetShadingModel() : MSM_DefaultLit;
}

UPhysicalMaterial* UMaterialInstance::GetPhysicalMaterial() const {
    if (PhysMaterial != nullptr) return PhysMaterial;
    return Parent && Parent != this ? Parent->GetPhysicalMaterial() : nullptr;
}

float UMaterialInstanceConstant::K2_GetScalarParameterValue(FName ParameterName) {
    float Result = 0.f;
    if (!ParameterName.isNone()) GetScalarParameterValue(FMaterialParameterInfo(ParameterName), Result);
    return Result;
}

UTexture* UMaterialInstanceConstant::K2_GetTextureParameterValue(FName ParameterName) {
    UTexture* Result = nullptr;
    if (!ParameterName.isNone()) GetTextureParameterValue(FMaterialParameterInfo(ParameterName), Result);
    return Result;
}

FLinearColor UMaterialInstanceConstant::K2_GetVectorParameterValue(FName ParameterName) {
    FLinearColor Result(0, 0, 0, 0);
    if (!ParameterName.isNone()) GetVectorParameterValue(FMaterialParameterInfo(ParameterName), Result);
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
    if (MaterialInstance == nullptr) return;
    ScalarParameterValues = MaterialInstance->ScalarParameterValues;
    VectorParameterValues = MaterialInstance->VectorParameterValues;
    DoubleVectorParameterValues = MaterialInstance->DoubleVectorParameterValues;
    TextureParameterValues = MaterialInstance->TextureParameterValues;
}

void UMaterialInstanceDynamic::K2_CopyMaterialInstanceParameters(UMaterialInterface* Source, bool bQuickParametersOnly) {
    // UE copies every parameter value of the source's chain into this instance.
    ClearParameterValues();
    if (Source == nullptr || Source == this) return;
    TArray<FMaterialParameterInfo> Infos;
    Source->GetAllParameterInfoOfType(EMaterialParameterType::Scalar, Infos);
    for (const FMaterialParameterInfo& Info : Infos) {
        float Value = 0;
        if (Source->GetScalarParameterValue(Info, Value)) SetScalarParameterValueByInfo(Info, Value);
    }
    Infos.Reset();
    Source->GetAllParameterInfoOfType(EMaterialParameterType::Vector, Infos);
    for (const FMaterialParameterInfo& Info : Infos) {
        FLinearColor Value;
        if (Source->GetVectorParameterValue(Info, Value)) SetVectorParameterValueByInfo(Info, Value);
    }
    if (bQuickParametersOnly) return;
    Infos.Reset();
    Source->GetAllParameterInfoOfType(EMaterialParameterType::Texture, Infos);
    for (const FMaterialParameterInfo& Info : Infos) {
        UTexture* Value = nullptr;
        if (Source->GetTextureParameterValue(Info, Value)) SetTextureParameterValueByInfo(Info, Value);
    }
}

void UMaterialInstanceDynamic::ClearParameterValues() {
    ScalarParameterValues.Empty();
    VectorParameterValues.Empty();
    DoubleVectorParameterValues.Empty();
    TextureParameterValues.Empty();
}

void UMaterialInstanceDynamic::K2_InterpolateMaterialInstanceParams(UMaterialInstance* SourceA, UMaterialInstance* SourceB, float Alpha) {
    if (SourceA == nullptr || SourceB == nullptr) return;
    // Scalars and vectors that both sources override (UE's K2_InterpolateMaterialInstanceParams).
    for (const FScalarParameterValue& A : SourceA->ScalarParameterValues) {
        float B = 0;
        if (SourceB->GetScalarParameterValue(A.ParameterInfo, B)) SetScalarParameterValueByInfo(A.ParameterInfo, FMath::Lerp(A.ParameterValue, B, Alpha));
    }
    for (const FVectorParameterValue& A : SourceA->VectorParameterValues) {
        FLinearColor B;
        if (SourceB->GetVectorParameterValue(A.ParameterInfo, B))
            SetVectorParameterValueByInfo(A.ParameterInfo, A.ParameterValue + (B - A.ParameterValue) * Alpha);
    }
}

}
