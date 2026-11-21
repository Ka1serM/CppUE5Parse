#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FRenderingObjectVersion {
    static constexpr FGuid GUID{0x12F88B9F, 0x88754AFC, 0xA67CD90C, 0x383ABD29};
    enum Type : int32 {
        IndirectLightingCache3BandSupport = 1,
        CustomReflectionCaptureResolutionSupport = 2,
        RemovedTextureStreamingLevelData = 3,
        MovedParticleCutoutsToRequiredModule = 8,
        MapBuildDataSeparatePackage = 9,
        TextureStreamingMeshUVChannelData = 10,
        TypeHandlingForMaterialSqrtNodes = 11,
        FixedBSPLightmaps = 12,
        ShaderResourceCodeSharing = 15,
        VolumetricLightmaps = 20,
        MaterialAttributeLayerParameters = 21,
        StoreReflectionCaptureBrightnessForCooking = 22,
        ShaderPermutationId = 25,
        IncreaseNormalPrecision = 26,
        VirtualTexturedLightmaps = 27,
        LightmapHasShadowmapData = 29,
        SkyAtmosphereStaticLightingVersioning = 32,
        VolumetricLightmapStreaming = 34,
        StaticMeshSectionForceOpaqueField = 37,
        PerInstanceCustomData = 40,
        VirtualTexturedLightmapsV3 = 45,
    };
};

struct FReleaseObjectVersion {
    static constexpr FGuid GUID{0x9C54D522, 0xA8264FBE, 0x94210746, 0x61B482D0};
    enum Type : int32 {
        LevelTransArrayConvertedToTArray = 3,
        EventSectionParameterStringAssetRef = 8,
        MaterialLayersParameterSerializationRefactor = 11,
        RemovedMaterialSharedInputCollection = 13,
        PropertiesSerializeRepCondition = 21,
        SerializeRigVMOffsetSegmentPaths = 35,
        AddedSubSequenceEntryWarpCounter = 42,
        CustomImplicitCollisionType = 29,
        FFieldPathOwnerSerialization = 30,
        MarginAddedToConvexAndBox = 37,
        StructureDataAddedToConvex = 38,
    };
};

struct FEditorObjectVersion {
    static constexpr FGuid GUID{0xE4B068ED, 0xF49442E9, 0xA231DA0B, 0x2E46BB41};
    enum Type : int32 {
        RefactorMeshEditorMaterials = 8,
        AddedFontFaceAssets = 9,
        AddedInlineFontFaceAssets = 14,
        AddedMaterialSharedInputs = 22,
        SerializeInstancedStaticMeshRenderData = 24,
    };
};

struct FMobileObjectVersion {
    static constexpr FGuid GUID{0xB02B49B5, 0xBB2044E9, 0xA30432B7, 0x52E40360};
    enum Type : int32 {
        InstancedStaticMeshLightmapSerialization = 1,
        LQVolumetricLightmapLayers = 2,
        StoreReflectionCaptureCompressedMobile = 3,
    };
};

struct FReflectionCaptureObjectVersion {
    static constexpr FGuid GUID{0x6B266CEC, 0x1EC74B8F, 0xA30BE4D9, 0x0942FC07};
    enum Type : int32 {
        MoveReflectionCaptureDataToMapBuildData = 1,
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
        MoveCompressedAnimDataToTheDDC = 5,
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
        RemoveUIDFromSmartNameSerialize = 5,
        SmartNameRefactorForDeterministicCooking = 10,
        AddLODToCurveMetaData = 12,
    };
};

struct FAnimObjectVersion {
    static constexpr FGuid GUID{0xAF43A65D, 0x7FD34947, 0x98733E8E, 0xD9C1BB05};
    enum Type : int32 {
        StoreMarkerNamesOnSkeleton = 2,
        SerializeRigVMRegisterArrayState = 3,
        IncreaseBoneIndexLimitPerChunk = 4,
        UnlimitedBoneInfluences = 5,
        SerializeRigVMRegisterDynamicState = 8,
        SerializeRigVMEntries = 10,
        SerializeHairClusterCullingData = 12,
        SerializeGroomCardsAndMeshes = 13,
        GroomLODStripping = 14,
    };
};

struct FPhysicsObjectVersion {
    static constexpr FGuid GUID{0x78F01B33, 0xEBEA4F98, 0xB9B484EA, 0xCCB95AA2};
    enum Type : int32 {
        GroomWithDescription = 3,
        ConvexUsesVerticesArray = 9,
        ChaosConvexUsesHalfEdges = 13,
        ChaosConvexHasUniqueEdgeSet = 14,
        GeometryCollectionConvexDefaults = 20,
    };
};

struct FDestructionObjectVersion {
    static constexpr FGuid GUID{0x174F1F0B, 0xB4C645A5, 0xB13F2EE8, 0xD0FB917D};
    enum Type : int32 {
        AddedTimestampedGeometryComponentCache = 1,
        GeometryCollectionInDDC = 3,
        GeometryCollectionInDDCAndAsset = 4,
        ChaosArchiveAdded = 5,
        DensityUnitsChanged = 7,
        BulkSerializeArrays = 8,
        GroupAndAttributeNameRemapping = 9,
        ImplicitObjectDoCollideAttribute = 10,
    };
};

struct FExternalPhysicsCustomObjectVersion {
    static constexpr FGuid GUID{0x35F94A83, 0xE258406C, 0xA31809F5, 0x9610247C};
    enum Type : int32 {
        ScaledGeometryIsConcrete = 12,
        AddConvexCenterOfMassAndVolume = 20,
        TBoxReplacedWithTAABB = 25,
        UnionObjectsCanAvoidHierarchy = 29,
        CapsulesNoUnionOrAABBs = 30,
        ConvexUsesTPlaneConcrete = 31,
    };
};

struct FFortniteSeasonBranchObjectVersion {
    static constexpr FGuid GUID{0x5B4C06B7, 0x24634AF8, 0x805BBF70, 0xCDF5D0DD};
    enum Type : int32 {
        ChaosImplicitObjectUnionLeafObjectsToInt32 = 12,
    };
};

struct FUE5ReleaseStreamObjectVersion {
    static constexpr FGuid GUID{0xD89B5E42, 0x24BD4D46, 0x8412ACA8, 0xDF641779};
    enum Type : int32 {
        RemovingTessellation = 3,
        MaterialInterfaceSavedCachedData = 14,
        AddClothMappingLODBias = 15,
        ExcludeBrightnessFromEncodedHDRCubemap = 23,
        StoreReflectionCaptureEncodedHDRDataInRG11B10Format = 38,
        RigVMSaveDebugMapInGraphFunctionData = 44,
        RemovedSmartNameContainerPayload = 64,
        GeometryCollectionConvexDefaults = 31,
        AddedInertiaTensorAndRotationOfMassAddedToConvex = 34,
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
        RigVMCopyOpStoreNumBytes = 43,
        RigVMMemoryStorageObject = 52,
        UseTriangleMeshBVH = 61,
        SoundWaveVirtualizationUpdate = 66,
        SkelMeshSectionVisibleInRayTracingFlagAdded = 54,
        MobileStationaryLocalLights = 79,
        IncreasedSkinWeightPrecision = 90,
        RigVMLazyEvaluation = 94,
        RigVMSerializeExecuteContextStruct = 97,
        RigVMSaveSerializedGraphInGraphFunctionData = 101,
        RigVMGeneratedClass = 104,
        GroomAssetWidthOverride = 107,
        SkeletalVertexAttributes = 113,
        RigVMExternalExecuteContextStruct = 114,
        DynamicMeshCompactedSerialization = 56,
        DynamicMeshAttributesWeightMapsAndNames = 62,
        SkinnedMeshInstanceDataSerializationV2 = 120,
        WorldPartitionSerializeStreamingPolicyOnCook = 74,
        GeometryCollectionNaniteData = 1,
        GeometryCollectionNaniteCooked = 11,
        GeometryCollectionNaniteTransient = 14,
        ConvexUsesVerticesArray = 26,
        GeometryCollectionPerChildDamageThreshold = 68,
        ManagedArrayCollectionAlwaysSerializeValue = 80,
        GeometryCollectionDamagePropagationData = 85,
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

struct FSequencerObjectVersion {
    static constexpr FGuid GUID{0x7B5AE74C, 0xD2704C10, 0xA9585798, 0x0B212A5A};
    enum Type : int32 {
        RenameMediaSourcePlatformPlayers = 1,
        EvaluationTree = 5,
        FloatToIntConversion = 7,
        SerializeFloatChannel = 10,
        SerializeFloatChannelCompletely = 12,
    };
};

struct FFortniteMainBranchObjectVersion {
    static constexpr FGuid GUID{0x601D1886, 0xAC644F84, 0xAA16D3DE, 0x0DEAC7D6};
    enum Type : int32 {
        MaterialInstanceSerializeOptimization_ShaderFName = 2,
        StaticParameterTerrainLayerWeightBlendType = 8,
        SerializeInstancedStaticMeshRenderData = 11,
        ShrinkCurveTableSize = 23,
        FortMappedCookedAnimation = 25,
        RigVMByteCodeDeterminism = 36,
        LevelsetSerializationSupportForBodySetup = 42,
        SerializeFloatChannelShowCurve = 53,
        AddedSubSequenceEntryWarpCounter = 55,
        LandscapeGrassSingleArray = 54,
        BPGCCookedEditorTags = 57,
        RigVMSaveDebugMapInGraphFunctionData = 113,
        ISMComponentEditableWhenInheritedSkipSerialization = 115,
        LandscapeSupportPerComponentGrassTypes = 116,
        SkeletalHalfEdgeData = 134,
        VolumetricLightMapGridDescSupport = 139,
        LevelInstanceStaticLightingSupport = 150,
        MeshPaintTextureUsesEditorOnly = 166,
        MaterialInputUsesLinearColor = 171,
        AddedPreprocessedFontGeometry = 180,
        DynamicMeshSerializeSculptLayers = 181,
        AnimSequenceRawDataOnlyFlagRemoval = 183,
        MeshMaterialSlotOverlayMaterialAdded = 196,
        DynamicMeshAttributesMorphTargets = 224,
        OverridableLandscapeGrassTypes = 253,
        DynamicMeshAttributesTriangleLabels = 257,
        InstancedSkinnedMeshBoneAttachments = 264,
        ChaosConvexVariableStructureDataAndVerticesArray = 46,
        ChaosImplicitObjectUnionBVHRefactor = 91,
        GeometryCollectionConvertVertexColorToSRGB = 107,
    };
};

struct FFortniteReleaseBranchCustomObjectVersion {
    static constexpr FGuid GUID{0xE7086368, 0x6B234C58, 0x84391B70, 0x16265E91};
    enum Type : int32 {
        DisableLevelset_v14_10 = 1,
        ActorComponentUCSModifiedPropertiesSparseStorage = 4,
        DynamicMeshAttributesSerializeBones = 15,
    };
};

struct FLandscapeCustomVersion {
    static constexpr FGuid GUID{0x23AFE18E, 0x4CE14E58, 0x8D61C252, 0xB953BEB7};
    enum Type : int32 {
        GrassMaterialWPO = 2,
    };
};

struct FNiagaraCustomVersion {
    static constexpr FGuid GUID{0xFCF57AFA, 0x50764283, 0xB9A9E658, 0xFFA02D32};
    enum Type : int32 {
        MovedToDerivedDataCache = 29,
        ChangeEmitterCompiledDataToSharedRefs = 51,
        AddGeneratedFunctionsToGPUParamInfo = 55,
        VariablesUseTypeDefRegistry = 64,
        AddVariadicParametersToGPUFunctionInfo = 77,
        SerializeUsageBitMaskToGPUFunctionInfo = 91,
    };
};

struct FNiagaraObjectVersion {
    static constexpr FGuid GUID{0xF2AED0AC, 0x9AFE416F, 0x8664AA7F, 0xFA26D6FC};
    enum Type : int32 {
        SkeletalMeshVertexSampling = 1,
    };
};

}
