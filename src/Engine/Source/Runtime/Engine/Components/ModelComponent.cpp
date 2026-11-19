#include "Engine/Source/Runtime/Engine/Components/ModelComponent.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/LightMap.h"
#include "Engine/Source/Runtime/Engine/Model.h"
#include "Engine/Source/Runtime/Engine/ShadowMap.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FModelElement& Element) {
    const bool bLegacyMapBuildData =
        !Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MapBuildDataSeparatePackage, EGame::GAME_UE4_14);
    if (bLegacyMapBuildData) {
        FLightMapRef LightMap;
        FShadowMapRef ShadowMap;
        Ar << LightMap << ShadowMap;
    }
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::FixedBSPLightmaps, EGame::GAME_UE4_14))
        Ar << Element.MapBuildDataId;
    Ar << Element.Component << Element.Material << Element.Nodes;
    if (!bLegacyMapBuildData)
        return Ar;
    TArray<FGuid> IrrelevantLights;
    return Ar << IrrelevantLights;
}

UModelComponent::UModelComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    CastShadow = true;
    bUseAsOccluder = true;
    Mobility = EComponentMobility::Static;
    bGenerateOverlapEvents = false;
}

void UModelComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    Ar << Model;
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::REMOVE_ZONES_FROM_MODEL) {
        int32 DummyZoneIndex = 0;
        Ar << DummyZoneIndex;
    }
    Ar << Elements << ComponentIndex << Nodes;
}

}
