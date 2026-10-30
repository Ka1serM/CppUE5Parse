#include "ue5parse/runtime/Engine/Materials/MaterialInterface.h"

#include "ue5parse/runtime/Engine/Engine/Texture.h"
#include "ue5parse/runtime/Engine/Materials/Material.h"

// UE's MaterialInterface.cpp and the parameter lookups of MaterialCachedData.cpp (UE 5.3).

namespace ue5parse::runtime {

const UScriptStruct* UMaterialInterface::GetCachedExpressionDataStruct() {
    return FindNativeStruct(FName("MaterialCachedExpressionData"));
}

UMaterialInterface::~UMaterialInterface() {
    if (CachedExpressionData != nullptr) {
        GetCachedExpressionDataStruct()->DestroyStruct(CachedExpressionData);
        RuntimeFree(CachedExpressionData);
    }
}

void UMaterialInterface::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetRemaining() < 4) return;
    bool bSavedCachedExpressionData = false;
    Ar << bSavedCachedExpressionData;
    if (!bSavedCachedExpressionData) return;
    const UScriptStruct* Struct = GetCachedExpressionDataStruct();
    if (Struct == nullptr) {
        Ar.SetError(ErrorCode::MissingSchema, "UMaterialInterface.CachedExpressionData");
        return;
    }
    const auto Size = static_cast<std::size_t>(FMath::Max(Struct->GetStructureSize(), 1));
    CachedExpressionData = static_cast<uint8*>(RuntimeMalloc(Size, static_cast<std::size_t>(FMath::Max(Struct->GetMinAlignment(), 16))));
    std::memset(CachedExpressionData, 0, Size);
    Struct->InitializeStruct(CachedExpressionData);
    Struct->SerializeItem(Ar, CachedExpressionData);
}

namespace {

// The ParameterInfoSet of one EMaterialParameterType of the cached data's RuntimeEntries.
const void* FindParameterInfoSet(const void* Data, EMaterialParameterType Type, const FSetProperty*& OutSet) {
    const UScriptStruct* Struct = UMaterialInterface::GetCachedExpressionDataStruct();
    const FStructProperty* Entries = FindFProperty<FStructProperty>(Struct, FName("RuntimeEntries"));
    if (Entries == nullptr || static_cast<int32>(Type) >= Entries->ArrayDim) return nullptr;
    const void* Entry = Entries->ContainerPtrToValuePtr<void>(Data, static_cast<int32>(Type));
    OutSet = FindFProperty<FSetProperty>(Entries->Struct, FName("ParameterInfoSet"));
    return OutSet ? OutSet->ContainerPtrToValuePtr<void>(Entry) : nullptr;
}

// The value array of each EMaterialParameterType in FMaterialCachedExpressionData (UE 5.3).
FName CachedValuesName(EMaterialParameterType Type) {
    switch (Type) {
    case EMaterialParameterType::Scalar: return FName("ScalarValues");
    case EMaterialParameterType::Vector: return FName("VectorValues");
    case EMaterialParameterType::DoubleVector: return FName("DoubleVectorValues");
    case EMaterialParameterType::Texture: return FName("TextureValues");
    case EMaterialParameterType::Font: return FName("FontValues");
    case EMaterialParameterType::RuntimeVirtualTexture: return FName("RuntimeVirtualTextureValues");
    case EMaterialParameterType::SparseVolumeTexture: return FName("SparseVolumeTextureValues");
    case EMaterialParameterType::StaticSwitch: return FName("StaticSwitchValues");
    }
    return FName();
}

}

bool UMaterialInterface::GetCachedParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo,
                                                 const FProperty*& OutProperty, const void*& OutValue) const {
    const void* Data = GetCachedExpressionData();
    if (Data == nullptr) return false;
    const FSetProperty* InfoSet = nullptr;
    const void* SetData = FindParameterInfoSet(Data, Type, InfoSet);
    if (SetData == nullptr) return false;
    // The parameter's element id in the set (UE's FindId) indexes the value array.
    FScriptSetHelper Helper(InfoSet, SetData);
    const UScriptStruct* InfoStruct = TBaseStructure<FMaterialParameterInfo>::Get();
    int32 Found = INDEX_NONE;
    for (int32 Index = 0; Index < Helper.GetMaxIndex() && Found == INDEX_NONE; Index++) {
        if (!Helper.IsValidIndex(Index)) continue;
        FMaterialParameterInfo Element;
        InfoStruct->CopyScriptStruct(&Element, Helper.GetElementPtr(Index));
        if (Element == ParameterInfo) Found = Index;
    }
    if (Found == INDEX_NONE) return false;
    const FArrayProperty* Values = FindFProperty<FArrayProperty>(GetCachedExpressionDataStruct(), CachedValuesName(Type));
    if (Values == nullptr) return false;
    FScriptArrayHelper ValueHelper(Values, Values->ContainerPtrToValuePtr<void>(Data));
    if (!ValueHelper.IsValidIndex(Found)) return false;
    OutProperty = Values->Inner;
    OutValue = ValueHelper.GetRawPtr(Found);
    return true;
}

bool UMaterialInterface::GetScalarParameterValue(const FMaterialParameterInfo&, float&, bool) const { return false; }
bool UMaterialInterface::GetVectorParameterValue(const FMaterialParameterInfo&, FLinearColor&, bool) const { return false; }
bool UMaterialInterface::GetDoubleVectorParameterValue(const FMaterialParameterInfo&, FVector4d&, bool) const { return false; }
bool UMaterialInterface::GetTextureParameterValue(const FMaterialParameterInfo&, UTexture*&, bool) const { return false; }
bool UMaterialInterface::GetStaticSwitchParameterValue(const FMaterialParameterInfo&, bool&, bool) const { return false; }

void UMaterialInterface::GetAllParameterInfoOfType(EMaterialParameterType Type, TArray<FMaterialParameterInfo>& OutParameterInfo) const {
    OutParameterInfo.Reset();
    const void* Data = GetCachedExpressionData();
    if (Data == nullptr) return;
    const FSetProperty* InfoSet = nullptr;
    const void* SetData = FindParameterInfoSet(Data, Type, InfoSet);
    if (SetData == nullptr) return;
    FScriptSetHelper Helper(InfoSet, SetData);
    const UScriptStruct* InfoStruct = TBaseStructure<FMaterialParameterInfo>::Get();
    for (int32 Index = 0; Index < Helper.GetMaxIndex(); Index++) {
        if (!Helper.IsValidIndex(Index)) continue;
        FMaterialParameterInfo Element;
        InfoStruct->CopyScriptStruct(&Element, Helper.GetElementPtr(Index));
        OutParameterInfo.Add(Element);
    }
}

TEnumAsByte<EBlendMode> UMaterialInterface::GetBlendMode() const { return BLEND_Opaque; }
bool UMaterialInterface::IsTwoSided() const { return false; }
bool UMaterialInterface::IsMasked() const { return false; }
float UMaterialInterface::GetOpacityMaskClipValue() const { return 0.0f; }
EMaterialShadingModel UMaterialInterface::GetShadingModel() const { return MSM_DefaultLit; }

void UMaterialInterface::GetUsedTextures(TArray<UTexture*>& OutTextures) const {
    OutTextures.Reset();
    const void* Data = GetCachedExpressionData();
    TArray<UObject*> Referenced;
    if (Data == nullptr || !GetPropertyValue_InContainer(GetCachedExpressionDataStruct(), Data, FName("ReferencedTextures"), Referenced)) return;
    for (UObject* Object : Referenced)
        if (UTexture* Texture = Cast<UTexture>(Object)) OutTextures.AddUnique(Texture);
}

}
