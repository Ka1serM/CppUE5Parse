#pragma once

#include "Engine/Source/Runtime/Engine/Engine/BlendableInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialOverrideNanite.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialTranslatorProfile.h"

#include "Material.generated.h"

namespace ue5parse::runtime {

class UMaterialExpression;

UENUM()
enum EMaterialDomain : int32 { MD_Surface, MD_DeferredDecal, MD_LightFunction, MD_Volume, MD_PostProcess, MD_UI, MD_RuntimeVirtualTexture, MD_MAX };

UENUM()
enum EDecalBlendMode : int32 {
    DBM_Translucent,
    DBM_Stain,
    DBM_Normal,
    DBM_Emissive,
    DBM_DBuffer_ColorNormalRoughness,
    DBM_DBuffer_Color,
    DBM_DBuffer_ColorNormal,
    DBM_DBuffer_ColorRoughness,
    DBM_DBuffer_Normal,
    DBM_DBuffer_NormalRoughness,
    DBM_DBuffer_Roughness,
    DBM_DBuffer_Emissive,
    DBM_DBuffer_AlphaComposite,
    DBM_DBuffer_EmissiveAlphaComposite,
    DBM_Volumetric_DistanceFunction,
    DBM_AlphaComposite,
    DBM_AmbientOcclusion,
    DBM_MAX,
};

UENUM()
enum EMaterialDecalResponse : int32 {
    MDR_None,
    MDR_ColorNormalRoughness,
    MDR_Color,
    MDR_ColorNormal,
    MDR_ColorRoughness,
    MDR_Normal,
    MDR_NormalRoughness,
    MDR_Roughness,
    MDR_MAX,
};

UENUM()
enum EMaterialTranslucencyPass : int32 { MTP_BeforeDOF, MTP_AfterDOF, MTP_AfterMotionBlur, MTP_MAX };

USTRUCT()
struct FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> Expression = nullptr;
    UPROPERTY()
    int32 OutputIndex = 0;
    UPROPERTY()
    FName InputName;
    UPROPERTY()
    int32 Mask = 0;
    UPROPERTY()
    int32 MaskR = 0;
    UPROPERTY()
    int32 MaskG = 0;
    UPROPERTY()
    int32 MaskB = 0;
    UPROPERTY()
    int32 MaskA = 0;
    UPROPERTY()
    FName ExpressionName;
};

USTRUCT()
struct FColorMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    FLinearColor Constant;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FScalarMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    float Constant = 0;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FVectorMaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    FVector3f Constant = FVector3f::ZeroVector;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FVector2MaterialInput : public FMaterialInput {
    GENERATED_BODY()
    UPROPERTY()
    bool UseConstant = false;
    UPROPERTY()
    float ConstantX = 0;
    UPROPERTY()
    float ConstantY = 0;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FShadingModelMaterialInput : public FMaterialInput {
    GENERATED_BODY()
};

USTRUCT()
struct FStrataMaterialInput : public FMaterialInput {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FColorMaterialInput> : TStructOpsTypeTraitsBase2<FColorMaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FScalarMaterialInput> : TStructOpsTypeTraitsBase2<FScalarMaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FVectorMaterialInput> : TStructOpsTypeTraitsBase2<FVectorMaterialInput> {
    enum { WithSerializer = true };
};
template <>
struct TStructOpsTypeTraits<FVector2MaterialInput> : TStructOpsTypeTraitsBase2<FVector2MaterialInput> {
    enum { WithSerializer = true };
};

UCLASS()
class UMaterial : public UMaterialInterface {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysMaterial = nullptr;
    UPROPERTY()
    FColorMaterialInput DiffuseColor;
    UPROPERTY()
    FColorMaterialInput SpecularColor;
    UPROPERTY()
    FColorMaterialInput BaseColor;
    UPROPERTY()
    FScalarMaterialInput Metallic;
    UPROPERTY()
    FScalarMaterialInput Specular;
    UPROPERTY()
    FScalarMaterialInput SpecularPower_DEPRECATED;
    UPROPERTY()
    FScalarMaterialInput Roughness;
    UPROPERTY()
    FVectorMaterialInput Normal;
    UPROPERTY()
    FColorMaterialInput EmissiveColor;
    UPROPERTY()
    FScalarMaterialInput Opacity;
    UPROPERTY()
    FScalarMaterialInput OpacityMask;
    UPROPERTY()
    float FresnelBaseReflectFraction_DEPRECATED = 0;
    UPROPERTY()
    TEnumAsByte<EMaterialDomain> MaterialDomain = MD_Surface;
    UPROPERTY()
    TEnumAsByte<EBlendMode> BlendMode = BLEND_Opaque;
    UPROPERTY()
    TEnumAsByte<EDecalBlendMode> DecalBlendMode = DBM_Translucent;
    UPROPERTY()
    TEnumAsByte<EMaterialDecalResponse> MaterialDecalResponse = MDR_ColorNormalRoughness;
    UPROPERTY()
    TEnumAsByte<EMaterialShadingModel> ShadingModel = MSM_DefaultLit;
    UPROPERTY()
    float OpacityMaskClipValue = 0.3333f;
    UPROPERTY()
    FVectorMaterialInput WorldPositionOffset;
    UPROPERTY()
    FVectorMaterialInput WorldDisplacement;
    UPROPERTY()
    FScalarMaterialInput TessellationMultiplier;
    UPROPERTY()
    FColorMaterialInput SubsurfaceColor;
    UPROPERTY()
    FScalarMaterialInput AmbientOcclusion;
    UPROPERTY()
    FScalarMaterialInput Refraction;
    UPROPERTY()
    FVector2MaterialInput CustomizedUVs;
    UPROPERTY()
    FMaterialAttributesInput MaterialAttributes;
    UPROPERTY()
    bool bEnableSeparateTranslucency = true;
    UPROPERTY()
    bool bEnableResponsiveAA = false;
    UPROPERTY()
    bool TwoSided = false;
    UPROPERTY()
    int32 NumCustomizedUVs = 0;
    UPROPERTY()
    TEnumAsByte<ETranslucencyLightingMode> TranslucencyLightingMode = TLM_VolumetricNonDirectional;
    UPROPERTY()
    float TranslucencyDirectionalLightingIntensity = 1.0f;
    UPROPERTY()
    float TranslucentShadowDensityScale = 0.5f;
    UPROPERTY()
    float TranslucentSelfShadowDensityScale = 2.0f;
    UPROPERTY()
    float TranslucentSelfShadowSecondDensityScale = 10.0f;
    UPROPERTY()
    float TranslucentSelfShadowSecondOpacity = 0;
    UPROPERTY()
    float TranslucentBackscatteringExponent = 30.0f;
    UPROPERTY()
    FLinearColor TranslucentMultipleScatteringExtinction = FLinearColor(1.0f, 0.833f, 0.588f, 1.0f);
    UPROPERTY()
    float TranslucentShadowStartOffset = 100.0f;
    UPROPERTY()
    bool bDisableDepthTest = false;
    UPROPERTY()
    bool bGenerateSphericalParticleNormals = false;
    UPROPERTY()
    bool bTangentSpaceNormal = true;
    UPROPERTY()
    bool bUseEmissiveForDynamicAreaLighting = false;
    UPROPERTY()
    bool bPhysicallyBasedInputs_DEPRECATED = false;
    UPROPERTY()
    bool bUsedAsLightFunction_DEPRECATED = false;
    UPROPERTY()
    bool bUsedWithDeferredDecal_DEPRECATED = false;
    UPROPERTY()
    bool bUsedAsSpecialEngineMaterial = false;
    UPROPERTY()
    bool bUsedWithSkeletalMesh = false;
    UPROPERTY()
    bool bUsedWithEditorCompositing = false;
    UPROPERTY()
    bool bUsedWithLandscape = false;
    UPROPERTY()
    bool bUsedWithParticleSprites = false;
    UPROPERTY()
    bool bUsedWithBeamTrails = false;
    UPROPERTY()
    bool bUsedWithMeshParticles = false;
    UPROPERTY()
    bool bUsedWithStaticLighting = false;
    UPROPERTY()
    bool bUsedWithFluidSurfaces = false;
    UPROPERTY()
    bool bUsedWithMorphTargets = false;
    UPROPERTY()
    bool bUsedWithSplineMeshes = false;
    UPROPERTY()
    bool bUsedWithInstancedStaticMeshes = false;
    UPROPERTY()
    bool bUsesDistortion = false;
    UPROPERTY()
    bool bUsedWithClothing = false;
    UPROPERTY()
    bool bUsedWithUI = false;
    UPROPERTY()
    bool bFullyRough = false;
    UPROPERTY()
    bool bUseLightmapDirectionality = true;
    UPROPERTY()
    TEnumAsByte<EMaterialTessellationMode> D3D11TessellationMode = MTM_NoTessellation;
    UPROPERTY()
    bool bEnableCrackFreeDisplacement = false;
    UPROPERTY()
    bool bEnableAdaptiveTessellation = true;
    UPROPERTY()
    bool Wireframe = false;
    UPROPERTY()
    int32 EditorX = 0;
    UPROPERTY()
    int32 EditorY = 0;
    UPROPERTY()
    int32 EditorPitch = 0;
    UPROPERTY()
    int32 EditorYaw = 0;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialExpression>> Expressions;
    UPROPERTY()
    TArray<FMaterialFunctionInfo> MaterialFunctionInfos;
    UPROPERTY()
    TArray<FMaterialParameterCollectionInfo> MaterialParameterCollectionInfos;
    UPROPERTY()
    bool bIsMasked = false;
    UPROPERTY()
    bool bIsPreviewMaterial = false;
    UPROPERTY()
    bool bUseMaterialAttributes = false;
    UPROPERTY()
    bool bUseTranslucencyVertexFog = true;
    UPROPERTY()
    bool bAllowDevelopmentShaderCompile = true;
    UPROPERTY()
    bool bIsMaterialEditorStatsMaterial = false;
    UPROPERTY()
    uint32 UsageFlagWarnings = 0;
    UPROPERTY()
    TEnumAsByte<EBlendableLocation> BlendableLocation = BL_SceneColorAfterTonemapping;
    UPROPERTY()
    int32 BlendablePriority = 0;
    UPROPERTY()
    float RefractionDepthBias = 0;
    UPROPERTY()
    FGuid StateId;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> ExpressionTextureReferences;
    UPROPERTY()
    FScalarMaterialInput ClearCoat;
    UPROPERTY()
    FScalarMaterialInput ClearCoatRoughness;
    UPROPERTY()
    bool bAutomaticallySetUsageInEditor = true;
    UPROPERTY()
    bool bCanMaskedBeAssumedOpaque = false;
    UPROPERTY()
    FScalarMaterialInput PixelDepthOffset;
    UPROPERTY()
    bool bScreenSpaceReflections = false;
    UPROPERTY()
    float MaxDisplacement = 0;
    UPROPERTY()
    bool bOutputVelocityOnBasePass = false;
    UPROPERTY()
    bool DitheredLODTransition = false;
    UPROPERTY()
    bool bBlockGI = false;
    UPROPERTY()
    bool DitherOpacityMask = false;
    UPROPERTY()
    bool bUseHQForwardReflections = false;
    UPROPERTY()
    bool bEnableMobileSeparateTranslucency = false;
    UPROPERTY()
    bool bUsePlanarForwardReflections = false;
    UPROPERTY()
    bool bAllowNegativeEmissiveColor = false;
    UPROPERTY()
    bool bUseFullPrecision = false;
    UPROPERTY()
    TEnumAsByte<ERefractionMode> RefractionMode = RM_IndexOfRefraction;
    UPROPERTY()
    bool AllowTranslucentCustomDepthWrites = false;
    UPROPERTY()
    bool bNormalCurvatureToRoughness = false;
    UPROPERTY()
    bool BlendableOutputAlpha = false;
    UPROPERTY()
    bool bUsedWithNiagaraSprites = false;
    UPROPERTY()
    bool bUsedWithNiagaraRibbons = false;
    UPROPERTY()
    bool bUsedWithNiagaraMeshParticles = false;
    UPROPERTY()
    bool bComputeFogPerPixel = false;
    UPROPERTY()
    bool bCastDynamicShadowAsMasked = false;
    UPROPERTY()
    bool bIsFunctionPreviewMaterial = false;
    UPROPERTY()
    bool bContactShadows = false;
    UPROPERTY()
    bool bWriteOnlyAlpha = false;
    UPROPERTY()
    bool bUsedWithGeometryCache = false;
    UPROPERTY()
    TArray<bool> CachedQualityLevelsUsed;
    UPROPERTY()
    FMaterialShadingModelField ShadingModels{MSM_DefaultLit};
    UPROPERTY()
    FShadingModelMaterialInput ShadingModelFromMaterialExpression;
    UPROPERTY()
    bool bUsedWithGeometryCollections = false;
    UPROPERTY()
    bool bCastRayTracedShadows = true;
    UPROPERTY()
    bool bOutputTranslucentVelocity = false;
    UPROPERTY()
    bool bEnableStencilTest = false;
    UPROPERTY()
    TEnumAsByte<EMaterialStencilCompare> StencilCompare = MSC_Less;
    UPROPERTY()
    uint8 StencilRefValue = 0;
    UPROPERTY()
    bool bUsedWithWater = false;
    UPROPERTY()
    bool bUsedWithHairStrands = false;
    UPROPERTY()
    bool bIsSky = false;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterialMask> PhysMaterialMask = nullptr;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysicalMaterialMap = nullptr;
    UPROPERTY()
    FScalarMaterialInput Anisotropy;
    UPROPERTY()
    FVectorMaterialInput Tangent;
    UPROPERTY()
    bool bUsedWithLidarPointCloud = false;
    UPROPERTY()
    bool bForwardRenderUsePreintegratedGFForSimpleIBL = false;
    UPROPERTY()
    bool bIsBlendable = true;
    UPROPERTY()
    FMaterialCachedExpressionData CachedExpressionData;
    UPROPERTY()
    bool bUsedWithVirtualHeightfieldMesh = false;
    UPROPERTY()
    bool bUseAlphaToCoverage = false;
    UPROPERTY()
    TEnumAsByte<EMaterialShadingRate> ShadingRate = MSR_1x1;
    UPROPERTY()
    bool bApplyCloudFogging = false;
    UPROPERTY()
    bool bForwardBlendsSkyLightCubemaps = false;
    UPROPERTY()
    bool WriteDepthToTranslucentMaterial = false;
    UPROPERTY()
    FStrataMaterialInput FrontMaterial;
    UPROPERTY()
    TEnumAsByte<EMaterialTranslucencyPass> TranslucencyPass = MTP_AfterDOF;
    UPROPERTY()
    bool bUsedWithNanite = false;
    UPROPERTY()
    TEnumAsByte<EMaterialFloatPrecisionMode> FloatPrecisionMode = MFPM_Default;
    UPROPERTY()
    bool bMobileEnableHighQualityBRDF = false;
    UPROPERTY()
    bool bEnableExecWire = false;
    UPROPERTY()
    bool bEnableNewHLSLGenerator = false;
    UPROPERTY()
    TArray<TObjectPtr<UPhysicalMaterial>> RenderTracePhysicalMaterialOutputs;
    UPROPERTY()
    TEnumAsByte<EStrataBlendMode> StrataBlendMode = SBM_Opaque;
    UPROPERTY()
    FMaterialOverrideNanite NaniteOverrideMaterial;
    UPROPERTY()
    uint32 CachedConnectedInputs = 0;
    UPROPERTY()
    bool bIsThinSurface = false;
    UPROPERTY()
    bool bRootNodeOverridesDefaultDistortion = false;
    UPROPERTY()
    bool bUsedWithVolumetricCloud = false;
    UPROPERTY()
    bool bAllowFrontLayerTranslucency = true;
    UPROPERTY()
    TEnumAsByte<ERefractionMode> RefractionMethod = RM_None;
    UPROPERTY()
    float MaxWorldPositionOffsetDisplacement = 0;
    UPROPERTY()
    FDisplacementScaling DisplacementScaling;
    UPROPERTY()
    bool bUsedWithHeterogeneousVolumes = false;
    UPROPERTY()
    TEnumAsByte<ERefractionCoverageMode> RefractionCoverageMode = RCM_CoverageAccountedFor;
    UPROPERTY()
    bool bAlwaysEvaluateWorldPositionOffset = false;
    UPROPERTY()
    bool bHasPixelAnimation = false;
    UPROPERTY()
    bool bEnableTessellation = false;
    UPROPERTY()
    bool bAllowVariableRateShading = true;
    UPROPERTY()
    bool bUsedWithNeuralNetworks = false;
    UPROPERTY()
    int8 NeuralProfileId = 0;
    UPROPERTY()
    uint16 PreshaderGap = 0;
    UPROPERTY()
    FDisplacementFadeRange DisplacementFadeRange;
    UPROPERTY()
    bool bEnableDisplacementFade = false;
    UPROPERTY()
    bool bForceCompatibleWithLightFunctionAtlas = false;
    UPROPERTY()
    FName UserSceneTexture;
    UPROPERTY()
    FIntPoint UserTextureDivisor;
    UPROPERTY()
    FName ResolutionRelativeToInput;
    UPROPERTY()
    bool bDisablePreExposureScale = false;
    UPROPERTY()
    TEnumAsByte<EPixelDepthOffsetMode> PixelDepthOffsetMode = PDOM_AlongCameraVector;
    UPROPERTY()
    bool bSubstrateRoughnessTracking = true;
    UPROPERTY()
    bool bUsedWithMaterialCache = false;
    UPROPERTY()
    bool bUsedWithStaticMesh = true;
    UPROPERTY()
    bool bCompatibleWithLumenCardSharing = false;
    UPROPERTY()
    bool bAllowTranslucentLocalLightShadow = false;
    UPROPERTY()
    float TranslucentLocalLightShadowQuality = 0;
    UPROPERTY()
    float TranslucentDirectionalLightShadowQuality = 0;
    UPROPERTY()
    bool bIsTranslucencyVelocityFromDepth = false;
    UPROPERTY()
    bool bRelaxRuntimeVirtualTextureRestrictions = false;
    UPROPERTY()
    bool bUsedWithVoxels = false;
    UPROPERTY()
    bool bUsedWithMeshDeformer = false;
    UPROPERTY()
    bool bUsedWithCurves = false;
    UPROPERTY()
    bool bUsedWithInstancedSkinnedMesh = false;
    UPROPERTY()
    bool bUsedWithNaniteAssemblies = false;
    UPROPERTY()
    bool bEnableNewHLSLGeneratorOverridden = false;
    UPROPERTY()
    bool bMaterialTranslatorProfileOverridden = false;
    UPROPERTY()
    EMaterialTranslatorProfile MaterialTranslatorProfile = EMaterialTranslatorProfile::Compatibility;

    void Serialize(FArchive& Ar) override;
    UMaterial* GetMaterial() override { return this; }
    const UMaterial* GetMaterial() const override { return this; }
    const FMaterialCachedExpressionData* GetCachedExpressionData() const override;
    bool GetScalarParameterValue(const FMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const override;
    bool GetVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const override;
    bool GetDoubleVectorParameterValue(const FMaterialParameterInfo& ParameterInfo, FVector4d& OutValue, bool bOveriddenOnly = false) const override;
    bool GetTextureParameterValue(const FMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly = false) const override;
    bool GetStaticSwitchParameterValue(const FMaterialParameterInfo& ParameterInfo, bool& OutValue, bool bOveriddenOnly = false) const override;
    bool GetMaterialLayers(FMaterialLayersFunctions& OutLayers) const override;
    TEnumAsByte<EBlendMode> GetBlendMode() const override { return BlendMode; }
    bool IsTwoSided() const override { return TwoSided; }
    bool IsMasked() const override { return BlendMode == BLEND_Masked || bIsMasked; }
    float GetOpacityMaskClipValue() const override { return OpacityMaskClipValue; }
    EMaterialShadingModel GetShadingModel() const override { return ShadingModel.GetValue(); }
    UPhysicalMaterial* GetPhysicalMaterial() const override { return PhysMaterial; }
};

}
