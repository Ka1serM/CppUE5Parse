#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"
#include "Engine/Source/Runtime/Engine/LightMap.h"
#include "Engine/Source/Runtime/Engine/ShadowMap.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FStaticMeshComponentLODInfo& Info) {
    constexpr uint8 OverrideColorsStripFlag = 1;
    FStripDataFlags StripFlags(Ar);
    const bool bLegacyMapBuildData =
        !Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MapBuildDataSeparatePackage, EGame::GAME_UE4_14);
    if (!StripFlags.IsAudioVisualDataStripped() && bLegacyMapBuildData) {
        FLightMapRef LightMap;
        FShadowMapRef ShadowMap;
        Ar << LightMap << ShadowMap;
    } else if (!StripFlags.IsAudioVisualDataStripped()) {
        Ar << Info.MapBuildDataId;
        if (Ar.GetGame() >= EGame::GAME_UE5_5)
            Ar << Info.OriginalMapBuildDataId;
    }
    if (!StripFlags.IsClassDataStripped(OverrideColorsStripFlag)) {
        uint8 bLoadVertexColorData = 0;
        Ar << bLoadVertexColorData;
        if (bLoadVertexColorData) {
            Info.OverrideVertexColors = std::make_shared<FColorVertexBuffer>();
            Info.OverrideVertexColors->Serialize(Ar);
        }
    }
    return Ar;
}

UStaticMeshComponent::UStaticMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = false;
    BodyInstance.CollisionProfileName = FName("BlockAllDynamic");
    bBoundsChangeTriggersStreamingDataRebuild = true;
    bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;
}

void UStaticMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    Ar << LODData;
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MeshPaintTextureUsesEditorOnly, EGame::GAME_UE5_5))
        return;
    bool bSerializeAsCookedData = false;
    Ar << bSerializeAsCookedData;
    if (!bSerializeAsCookedData)
        return;
    UObject* Texture = nullptr;
    Ar << Texture;
    MeshPaintTextureCooked = Cast<UTexture>(Texture);
}

int32 UStaticMeshComponent::GetNumMaterials() const {
    return StaticMesh ? StaticMesh->GetStaticMaterials().Num() : 0;
}

int32 UStaticMeshComponent::GetMaterialIndex(FName MaterialSlotName) const {
    return StaticMesh ? StaticMesh->GetMaterialIndex(MaterialSlotName) : INDEX_NONE;
}

UMaterialInterface* UStaticMeshComponent::GetMaterial(int32 MaterialIndex) const {
    if (UMaterialInterface* Override = Super::GetMaterial(MaterialIndex))
        return Override;
    return StaticMesh ? StaticMesh->GetMaterial(MaterialIndex) : nullptr;
}

bool UStaticMeshComponent::SetStaticMesh(UStaticMesh* NewMesh) {
    if (NewMesh == GetStaticMesh())
        return false;
    StaticMesh = NewMesh;
    return true;
}

void UStaticMeshComponent::OnRep_StaticMesh(UStaticMesh* OldStaticMesh) {
    if (OldStaticMesh == StaticMesh)
        return;
    UStaticMesh* NewStaticMesh = StaticMesh;
    StaticMesh = OldStaticMesh;
    SetStaticMesh(NewStaticMesh);
}

}
