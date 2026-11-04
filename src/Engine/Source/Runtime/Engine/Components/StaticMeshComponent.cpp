#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FStaticMeshComponentLODInfo& Info) {
    constexpr uint8 OverrideColorsStripFlag = 1;
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsAudioVisualDataStripped()) Ar << Info.MapBuildDataId;
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

void UStaticMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd()) return;
    Ar << LODData;
}

int32 UStaticMeshComponent::GetNumMaterials() const {
    return StaticMesh ? StaticMesh->GetStaticMaterials().Num() : 0;
}

int32 UStaticMeshComponent::GetMaterialIndex(FName MaterialSlotName) const {
    return StaticMesh ? StaticMesh->GetMaterialIndex(MaterialSlotName) : INDEX_NONE;
}

UMaterialInterface* UStaticMeshComponent::GetMaterial(int32 MaterialIndex) const {
    if (UMaterialInterface* Override = Super::GetMaterial(MaterialIndex)) return Override;
    return StaticMesh ? StaticMesh->GetMaterial(MaterialIndex) : nullptr;
}

bool UStaticMeshComponent::SetStaticMesh(UStaticMesh* NewMesh) {
    if (NewMesh == GetStaticMesh()) return false;
    StaticMesh = NewMesh;
    return true;
}

void UStaticMeshComponent::OnRep_StaticMesh(UStaticMesh* OldStaticMesh) {
    if (OldStaticMesh == StaticMesh) return;
    UStaticMesh* NewStaticMesh = StaticMesh;
    StaticMesh = OldStaticMesh;
    SetStaticMesh(NewStaticMesh);
}

}
