#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"

#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"
#include "Engine/Source/Runtime/Engine/Materials/Material.h"

namespace ue5parse::runtime {

//! GTA: The Trilogy - Definitive Edition serializes the material texture info as raw fields instead of tagged properties.
bool FMaterialTextureInfo::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_GTATheTrilogyDefinitiveEdition)
        return false;
    Ar << SamplingScale << UVChannelIndex << TextureName;
    return true;
}

void UMaterialInterface::SkipInlineShaderMaps(FArchive& Ar) {
    OffsetToFirstResource = Ar.Tell();
    Ar.Seek(Ar.TotalSize());
}

Result<TArray<FMaterialResource>> UMaterialInterface::LoadMaterialResources() const {
    TArray<FMaterialResource> Resources;
    if (OffsetToFirstResource == INDEX_NONE)
        return Resources;
    FLinkerLoad* Linker = GetLinker();
    if (Linker == nullptr)
        return fail(ErrorCode::MissingExport, "UMaterialInterface.LoadMaterialResources");
    const ue5parse::Package& Package = Linker->GetPackage();
    Result<std::vector<std::byte>> Bytes = Package.readExportBytes(static_cast<std::size_t>(GetLinkerIndex()));
    if (!Bytes)
        return std::unexpected(Bytes.error());
    const ue5parse::PackageExport& Entry = Package.getExport(static_cast<std::size_t>(GetLinkerIndex()));
    FExportArchive Ar(*Linker, *Bytes, static_cast<int64>(Entry.cookedOffset), static_cast<int64>(Entry.dataOffset), nullptr);
    Ar.Seek(OffsetToFirstResource);
    SerializeInlineShaderMaps(Ar, Resources);
    if (Ar.IsError())
        return std::unexpected(Ar.GetError());
    return Resources;
}

UMaterialInterface::~UMaterialInterface() {
    if (CachedExpressionData != nullptr) {
        FMaterialCachedExpressionData::StaticStruct()->DestroyStruct(CachedExpressionData);
        RuntimeFree(CachedExpressionData);
    }
}

void UMaterialInterface::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetRemaining() < 4
        || !Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::MaterialInterfaceSavedCachedData, EGame::GAME_UE5_0))
        return;
    bool bSavedCachedExpressionData = false;
    Ar << bSavedCachedExpressionData;
    if (!bSavedCachedExpressionData)
        return;
    const UScriptStruct* Struct = FMaterialCachedExpressionData::StaticStruct();
    const auto Size = static_cast<std::size_t>(Struct->GetStructureSize());
    CachedExpressionData =
        static_cast<FMaterialCachedExpressionData*>(RuntimeMalloc(Size, static_cast<std::size_t>(FMath::Max(Struct->GetMinAlignment(), 16))));
    std::memset(CachedExpressionData, 0, Size);
    Struct->InitializeStruct(CachedExpressionData);
    Struct->SerializeItem(Ar, CachedExpressionData);
}

namespace {

FName CachedValuesName(EMaterialParameterType Type) {
    switch (Type) {
    case EMaterialParameterType::Scalar:
        return FName("ScalarValues");
    case EMaterialParameterType::Vector:
        return FName("VectorValues");
    case EMaterialParameterType::DoubleVector:
        return FName("DoubleVectorValues");
    case EMaterialParameterType::Texture:
        return FName("TextureValues");
    case EMaterialParameterType::TextureCollection:
        return FName("TextureCollectionValues");
    case EMaterialParameterType::Font:
        return FName("FontValues");
    case EMaterialParameterType::RuntimeVirtualTexture:
        return FName("RuntimeVirtualTextureValues");
    case EMaterialParameterType::SparseVolumeTexture:
        return FName("SparseVolumeTextureValues");
    case EMaterialParameterType::StaticSwitch:
        return FName("StaticSwitchValues");
    case EMaterialParameterType::ParameterCollection:
        return FName("ParameterCollectionValues");
    case EMaterialParameterType::ToonProfile:
        return FName("ToonProfileValues");
    case EMaterialParameterType::NumRuntime:
        break;
    }
    return FName();
}

}

bool UMaterialInterface::GetCachedParameterValue(EMaterialParameterType Type, const FMaterialParameterInfo& ParameterInfo, const FProperty*& OutProperty,
                                                 const void*& OutValue) const {
    const FMaterialCachedExpressionData* Data = GetCachedExpressionData();
    if (Data == nullptr)
        return false;
    const FMaterialCachedParameterEntry* Entry = Data->GetParameterTypeEntry(Type);
    if (Entry == nullptr)
        return false;
    const int32 Found = Entry->FindParameterIndex(ParameterInfo);
    if (Found == INDEX_NONE)
        return false;
    const FArrayProperty* Values = FindFProperty<FArrayProperty>(Data->GetParametersStruct(), CachedValuesName(Type));
    if (Values == nullptr)
        return false;
    FScriptArrayHelper ValueHelper(Values, Values->ContainerPtrToValuePtr<void>(Data->GetParameters()));
    if (!ValueHelper.IsValidIndex(Found))
        return false;
    OutProperty = Values->Inner;
    OutValue = ValueHelper.GetRawPtr(Found);
    return true;
}

//! Before UE 4.25 a cooked material has no cached expression data; it finds a parameter in its Expressions.
bool UMaterialInterface::HasCachedExpressionData() {
    return GetRuntimeProvider().getGame() >= EGame::GAME_UE4_25;
}

bool UMaterialInterface::GetCachedTextureValue(const FProperty* Property, const void* Value, UTexture*& OutValue) {
    if (!CastField<FSoftObjectProperty>(Property))
        return GetPropertyValue(Property, Value, OutValue);
    OutValue = Cast<UTexture>(static_cast<const FSoftObjectPtr*>(Value)->LoadSynchronous());
    return true;
}

bool UMaterialInterface::GetScalarParameterDefaultValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue) const {
    const FProperty* Property = nullptr;
    const void* Value = nullptr;
    return GetCachedParameterValue(EMaterialParameterType::Scalar, ParameterInfo, Property, Value) && GetPropertyValue(Property, Value, OutValue);
}

bool UMaterialInterface::GetScalarParameterValue(const FMaterialParameterInfo&, float&, bool) const {
    return false;
}
bool UMaterialInterface::GetVectorParameterValue(const FMaterialParameterInfo&, FLinearColor&, bool) const {
    return false;
}
bool UMaterialInterface::GetDoubleVectorParameterValue(const FMaterialParameterInfo&, FVector4d&, bool) const {
    return false;
}
bool UMaterialInterface::GetTextureParameterValue(const FMaterialParameterInfo&, UTexture*&, bool) const {
    return false;
}
bool UMaterialInterface::GetStaticSwitchParameterValue(const FMaterialParameterInfo&, bool&, bool) const {
    return false;
}

void UMaterialInterface::GetAllParameterInfoOfType(EMaterialParameterType Type, TArray<FMaterialParameterInfo>& OutParameterInfo) const {
    OutParameterInfo.Reset();
    const FMaterialCachedExpressionData* Data = GetCachedExpressionData();
    const FMaterialCachedParameterEntry* Entry = Data != nullptr ? Data->GetParameterTypeEntry(Type) : nullptr;
    if (Entry == nullptr)
        return;
    Entry->GetParameterInfos(OutParameterInfo);
}

TEnumAsByte<EBlendMode> UMaterialInterface::GetBlendMode() const {
    return BLEND_Opaque;
}
bool UMaterialInterface::IsTwoSided() const {
    return false;
}
bool UMaterialInterface::IsMasked() const {
    return false;
}
float UMaterialInterface::GetOpacityMaskClipValue() const {
    return 0.0f;
}
EMaterialShadingModel UMaterialInterface::GetShadingModel() const {
    return MSM_DefaultLit;
}

}
