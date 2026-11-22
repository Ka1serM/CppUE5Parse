#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCommon.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraGPUSortInfo.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraRendererProperties.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraSpriteRendererProperties.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UENUM()
enum class ENiagaraRendererPixelCoverageMode : uint8 { Automatic, Disabled, Enabled, Enabled_RGBA, Enabled_RGB, Enabled_A };

UENUM()
enum class ENiagaraSpriteAlignment : uint8 { Unaligned, VelocityAligned, CustomAlignment, Automatic };

UENUM()
enum class ENiagaraSpriteFacingMode : uint8 { FaceCamera, FaceCameraPlane, CustomFacingVector, FaceCameraPosition, FaceCameraDistanceBlend, Automatic };

UCLASS()
class UNiagaraSpriteRendererProperties : public UNiagaraRendererProperties {
    GENERATED_BODY()
public:
    explicit UNiagaraSpriteRendererProperties(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
    UPROPERTY()
    ENiagaraSpriteAlignment Alignment = ENiagaraSpriteAlignment::Automatic;
    UPROPERTY()
    ENiagaraSpriteFacingMode FacingMode = ENiagaraSpriteFacingMode::Automatic;
    UPROPERTY()
    FVector CustomFacingVectorMask;
    UPROPERTY()
    FVector2D PivotInUVSpace;
    UPROPERTY()
    ENiagaraSortMode SortMode = ENiagaraSortMode::None;
    UPROPERTY()
    FVector2D SubImageSize;
    UPROPERTY()
    uint8 bSubImageBlend : 1 = 0;
    UPROPERTY()
    uint8 bRemoveHMDRollInVR : 1 = 0;
    UPROPERTY()
    uint8 bSortOnlyWhenTranslucent : 1 = 0;
    UPROPERTY()
    float MinFacingCameraBlendDistance = 0.0f;
    UPROPERTY()
    float MaxFacingCameraBlendDistance = 0.0f;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PositionBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding ColorBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding VelocityBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding SpriteRotationBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding SpriteSizeBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding SpriteFacingBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding SpriteAlignmentBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding SubImageIndexBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding DynamicMaterialBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding DynamicMaterial1Binding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding DynamicMaterial2Binding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding DynamicMaterial3Binding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding CameraOffsetBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding UVScaleBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding MaterialRandomBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding CustomSortingBinding;
    UPROPERTY()
    int32 SyncId = 0;
    UPROPERTY()
    FNiagaraVariableAttributeBinding NormalizedAgeBinding;
    UPROPERTY()
    FNiagaraUserParameterBinding MaterialUserParamBinding;
    UPROPERTY()
    ENiagaraRendererSourceDataMode SourceMode = ENiagaraRendererSourceDataMode::Particles;
    UPROPERTY()
    uint32 bGpuLowLatencyTranslucency : 1 = 0;
    UPROPERTY()
    uint8 bEnableCameraDistanceCulling : 1 = 0;
    UPROPERTY()
    float MinCameraDistance = 0;
    UPROPERTY()
    float MaxCameraDistance = 1000.0f;
    UPROPERTY()
    uint32 RendererVisibility = 0;
    UPROPERTY()
    FNiagaraVariableAttributeBinding RendererVisibilityTagBinding;
    UPROPERTY()
    TArray<FNiagaraMaterialAttributeBinding> MaterialParameterBindings;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PivotOffsetBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevPositionBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevVelocityBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevSpriteRotationBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevSpriteSizeBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevSpriteFacingBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevSpriteAlignmentBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevCameraOffsetBinding;
    UPROPERTY()
    FNiagaraVariableAttributeBinding PrevPivotOffsetBinding;
    UPROPERTY()
    float MacroUVRadius = 0.0f;
    UPROPERTY()
    ENiagaraRendererPixelCoverageMode PixelCoverageMode = ENiagaraRendererPixelCoverageMode::Automatic;
    UPROPERTY()
    float PixelCoverageBlend = 1.0f;
    UPROPERTY()
    ENiagaraRendererSortPrecision SortPrecision = ENiagaraRendererSortPrecision::Default;
    UPROPERTY()
    ENiagaraRendererGpuTranslucentLatency GpuTranslucentLatency = ENiagaraRendererGpuTranslucentLatency::ProjectDefault;
    UPROPERTY()
    FNiagaraRendererMaterialParameters MaterialParameters;
    UPROPERTY()
    uint32 MaterialParamValidMask = 0;
    UPROPERTY()
    uint8 bCastShadows : 1 = 1;
};

}
