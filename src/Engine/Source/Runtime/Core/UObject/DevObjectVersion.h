#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FRenderingObjectVersion {
    static constexpr FGuid GUID{0x12F88B9F, 0x88754AFC, 0xA67CD90C, 0x383ABD29};
    enum Type : int32 {
        IndirectLightingCache3BandSupport = 1,
        RemovedTextureStreamingLevelData = 3,
        MapBuildDataSeparatePackage = 9,
        TextureStreamingMeshUVChannelData = 10,
        FixedBSPLightmaps = 12,
        IncreaseNormalPrecision = 26,
        StaticMeshSectionForceOpaqueField = 37,
    };
};

struct FReleaseObjectVersion {
    static constexpr FGuid GUID{0x9C54D522, 0xA8264FBE, 0x94210746, 0x61B482D0};
    enum Type : int32 {
        LevelTransArrayConvertedToTArray = 3,
        PropertiesSerializeRepCondition = 21,
    };
};

struct FEditorObjectVersion {
    static constexpr FGuid GUID{0xE4B068ED, 0xF49442E9, 0xA231DA0B, 0x2E46BB41};
    enum Type : int32 {
        RefactorMeshEditorMaterials = 8,
        AddedFontFaceAssets = 9,
        AddedInlineFontFaceAssets = 14,
    };
};

struct FCoreObjectVersion {
    static constexpr FGuid GUID{0x375EC13C, 0x06E448FB, 0xB50084F0, 0x262A717E};
    enum Type : int32 {
        MaterialInputNativeSerialize = 1,
        EnumProperties = 2,
        SkeletalMaterialEditorDataStripping = 3,
        FProperties = 4,
    };
};

struct FFrameworkObjectVersion {
    static constexpr FGuid GUID{0xCFFC743F, 0x43B04480, 0x939114DF, 0x171D2073};
    enum Type : int32 {
        SmartNameRefactor = 7,
        RemoveSoundWaveCompressionName = 12,
        MoveCurveTypesToSkeleton = 15,
        RemoveUField_Next = 29,
        PinsStoreFName = 31,
        UserDefinedStructsStoreDefaultInstance = 32,
    };
};

struct FAnimPhysObjectVersion {
    static constexpr FGuid GUID{0x29E575DD, 0xE0A34627, 0x9D10D276, 0x232CDCEA};
    enum Type : int32 {
        SmartNameRefactorForDeterministicCooking = 10,
        AddLODToCurveMetaData = 12,
    };
};

struct FAnimObjectVersion {
    static constexpr FGuid GUID{0xAF43A65D, 0x7FD34947, 0x98733E8E, 0xD9C1BB05};
    enum Type : int32 {
        StoreMarkerNamesOnSkeleton = 2,
        IncreaseBoneIndexLimitPerChunk = 4,
        UnlimitedBoneInfluences = 5,
    };
};

struct FUE5ReleaseStreamObjectVersion {
    static constexpr FGuid GUID{0xD89B5E42, 0x24BD4D46, 0x8412ACA8, 0xDF641779};
    enum Type : int32 {
        RemovingTessellation = 3,
        MaterialInterfaceSavedCachedData = 14,
        AddClothMappingLODBias = 15,
        RemovedSmartNameContainerPayload = 64,
    };
};

struct FRecomputeTangentCustomVersion {
    static constexpr FGuid GUID{0x5579F886, 0x933A4C1F, 0x83BA087B, 0x6361B92F};
    enum Type : int32 {
        RuntimeRecomputeTangent = 1,
        RecomputeTangentVertexColorMask = 2,
    };
};

struct FSkeletalMeshCustomVersion {
    static constexpr FGuid GUID{0xD78A4A00, 0xE8584697, 0xBAA819B5, 0x487D46B4};
    enum Type : int32 {
        CombineSectionWithChunk = 1,
        CombineSoftAndRigidVerts = 2,
        SaveNumVertices = 4,
        UseSharedColorBufferFormat = 6,
        UseSeparateSkinWeightBuffer = 7,
        NewClothingSystemAdded = 8,
        CompactClothVertexBuffer = 10,
        SplitModelAndRenderData = 12,
    };
};

struct FUE5MainStreamObjectVersion {
    static constexpr FGuid GUID{0x697DD581, 0xE64F41AB, 0xAA4A51EC, 0xBEB7B628};
    enum Type : int32 {
        AddedCookedBoolFontFaceAssets = 12,
        MaterialSavedCachedData = 36,
        SoundWaveVirtualizationUpdate = 66,
        SkelMeshSectionVisibleInRayTracingFlagAdded = 54,
        IncreasedSkinWeightPrecision = 90,
        SkeletalVertexAttributes = 113,
    };
};

struct FUE5SpecialProjectStreamObjectVersion {
    static constexpr FGuid GUID{0x59DA5D52, 0x12324948, 0xB8785978, 0x70B8E98B};
    enum Type : int32 {
        SerializeSceneComponentStaticBounds = 2,
        SerializeActorLabelInCookedBuilds = 4,
        SerializeSkeletalMeshMorphTargetRenderData = 7,
    };
};

struct FFortniteMainBranchObjectVersion {
    static constexpr FGuid GUID{0x601D1886, 0xAC644F84, 0xAA16D3DE, 0x0DEAC7D6};
    enum Type : int32 {
        ShrinkCurveTableSize = 23,
        LevelsetSerializationSupportForBodySetup = 42,
        BPGCCookedEditorTags = 57,
        ISMComponentEditableWhenInheritedSkipSerialization = 115,
        SkeletalHalfEdgeData = 134,
        LevelInstanceStaticLightingSupport = 150,
        MeshPaintTextureUsesEditorOnly = 166,
        MaterialInputUsesLinearColor = 171,
        AddedPreprocessedFontGeometry = 180,
        MeshMaterialSlotOverlayMaterialAdded = 196,
    };
};

struct FFortniteReleaseBranchCustomObjectVersion {
    static constexpr FGuid GUID{0xE7086368, 0x6B234C58, 0x84391B70, 0x16265E91};
    enum Type : int32 {
        DisableLevelset_v14_10 = 1,
        ActorComponentUCSModifiedPropertiesSparseStorage = 4,
    };
};

struct FNiagaraCustomVersion {
    static constexpr FGuid GUID{0xFCF57AFA, 0x50764283, 0xB9A9E658, 0xFFA02D32};
    enum Type : int32 {
        VariablesUseTypeDefRegistry = 64,
    };
};

struct FNiagaraObjectVersion {
    static constexpr FGuid GUID{0xF2AED0AC, 0x9AFE416F, 0x8664AA7F, 0xFA26D6FC};
    enum Type : int32 {
        SkeletalMeshVertexSampling = 1,
    };
};

}
