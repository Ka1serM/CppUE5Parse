#pragma once

#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetCards.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetDataflow.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetInterpolation.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetMeshes.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetPhysics.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetRendering.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/HairCardsDatas.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/HairStrandsDatas.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/AssetUserData.h"

#include "GroomAsset.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;

USTRUCT()
struct FHairGroupLODInfo {
    GENERATED_BODY()
    UPROPERTY()
    int32 NumPoints = 0;
    UPROPERTY()
    int32 NumCurves = 0;
};

USTRUCT()
struct FHairGroupInfo {
    GENERATED_BODY()
    UPROPERTY()
    int32 GroupIndex = 0;
    UPROPERTY()
    int32 GroupID = 0;
    UPROPERTY()
    FName GroupName;
    UPROPERTY()
    int32 GroupCardsID = 0;
    UPROPERTY()
    int32 NumCurves = 0;
    UPROPERTY()
    int32 NumGuides = 0;
    UPROPERTY()
    int32 NumCurveVertices = 0;
    UPROPERTY()
    int32 NumGuideVertices = 0;
    UPROPERTY()
    float MaxCurveLength = 0.0f;
    UPROPERTY()
    float MaxImportedWidth = -1.0f;
    UPROPERTY()
    uint32 Flags = 0;
    UPROPERTY()
    TArray<FHairGroupLODInfo> LODInfos;
};

USTRUCT()
struct FHairGroupInfoWithVisibility : public FHairGroupInfo {
    GENERATED_BODY()
    UPROPERTY()
    bool bIsVisible = true;
};

USTRUCT()
struct FHairGroupsMaterial {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    FName SlotName;
};

//! FHairGroupData before 5.3.
struct FHairGroupPlatformData {
    struct FCardsLOD {
        FHairCardsBulkData BulkData;
        FHairCardsInterpolationBulkData InterpolationBulkData;
        FHairStrandsBulkData GuideBulkData;
        FHairStrandsInterpolationBulkData GuideInterpolationBulkData;
    };
    struct FMeshesLOD {
        FHairMeshesBulkData BulkData;
    };

    FHairStrandsBulkData GuidesBulkData;
    FHairStrandsBulkData StrandsBulkData;
    FHairStrandsInterpolationBulkData StrandsInterpolationBulkData;
    FHairStrandsClusterBulkData StrandsClusterBulkData;
    TArray<FCardsLOD> CardsLODs;
    TArray<FMeshesLOD> MeshesLODs;
};

UCLASS()
class UGroomAsset : public UObject {
    GENERATED_BODY()
public:
    explicit UGroomAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    EGroomLODMode LODMode = EGroomLODMode::Default;
    UPROPERTY()
    float AutoLODBias = 0.0f;
    UPROPERTY()
    FGroomDataflowSettings DataflowSettings;
    UPROPERTY()
    TArray<FHairGroupInfoWithVisibility> HairGroupsInfo;
    UPROPERTY()
    TArray<FHairGroupsRendering> HairGroupsRendering;
    UPROPERTY()
    TArray<FHairGroupsPhysics> HairGroupsPhysics;
    UPROPERTY()
    TArray<FHairGroupsInterpolation> HairGroupsInterpolation;
    UPROPERTY()
    TArray<FHairGroupsLOD> HairGroupsLOD;
    UPROPERTY()
    TArray<FHairGroupsCardsSourceDescription> HairGroupsCards;
    UPROPERTY()
    TArray<FHairGroupsMeshesSourceDescription> HairGroupsMeshes;
    UPROPERTY()
    TArray<FHairGroupsMaterial> HairGroupsMaterials;
    UPROPERTY()
    bool EnableGlobalInterpolation = false;
    UPROPERTY()
    bool EnableSimulationCache = false;
    UPROPERTY()
    EGroomInterpolationType HairInterpolationType = EGroomInterpolationType::SmoothTransform;
    UPROPERTY()
    TObjectPtr<USkeletalMesh> RiggedSkeletalMesh = nullptr;
    UPROPERTY()
    TArray<int32> DeformedGroupSections;
    UPROPERTY()
    FPerPlatformInt MinLOD;
    UPROPERTY()
    FPerPlatformBool DisableBelowMinLodStripping;
    UPROPERTY()
    TArray<float> EffectiveLODBias;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;

    TArray<FHairGroupPlatformData> HairGroupsPlatformData;
};

}
