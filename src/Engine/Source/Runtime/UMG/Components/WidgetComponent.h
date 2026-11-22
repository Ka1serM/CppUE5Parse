#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureRenderTarget2D.h"

#include "WidgetComponent.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class ULocalPlayer;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UUserWidget;

UENUM()
enum class EWidgetSpace : uint8 { World, Screen };

UENUM()
enum class EWidgetTimingPolicy : uint8 { RealTime, GameTime };

UENUM()
enum class EWidgetBlendMode : uint8 { Opaque, Masked, Transparent };

UENUM()
enum class EWidgetGeometryMode : uint8 { Plane, Cylinder };

UENUM()
enum class EWindowVisibility : uint8 { Visible, SelfHitTestInvisible };

UENUM()
enum class ETickMode : uint8 { Disabled, Enabled, Automatic };

UCLASS()
class UWidgetComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UWidgetComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    EWidgetSpace Space = EWidgetSpace::World;
    UPROPERTY()
    EWidgetTimingPolicy TimingPolicy = EWidgetTimingPolicy::RealTime;
    UPROPERTY()
    TSubclassOf<UUserWidget> WidgetClass;
    UPROPERTY()
    FIntPoint DrawSize{500, 500};
    UPROPERTY()
    bool bManuallyRedraw = false;
    UPROPERTY()
    bool bRedrawRequested = true;
    UPROPERTY()
    float RedrawTime = 0.0f;
    UPROPERTY()
    FIntPoint CurrentDrawSize{0, 0};
    UPROPERTY()
    bool bUseInvalidationInWorldSpace = false;
    UPROPERTY()
    bool bDrawAtDesiredSize = false;
    UPROPERTY()
    FVector2D Pivot{0.5, 0.5};
    UPROPERTY()
    float MaxInteractionDistance = 1000.0f;
    UPROPERTY()
    bool bReceiveHardwareInput = false;
    UPROPERTY()
    bool bWindowFocusable = true;
    UPROPERTY()
    EWindowVisibility WindowVisibility = EWindowVisibility::SelfHitTestInvisible;
    UPROPERTY()
    bool bApplyGammaCorrection = false;
    UPROPERTY()
    TObjectPtr<ULocalPlayer> OwnerPlayer = nullptr;
    UPROPERTY()
    FLinearColor BackgroundColor{0.0f, 0.0f, 0.0f, 0.0f};
    UPROPERTY()
    FLinearColor TintColorAndOpacity{1.0f, 1.0f, 1.0f, 1.0f};
    UPROPERTY()
    float OpacityFromTexture = 1.0f;
    UPROPERTY()
    EWidgetBlendMode BlendMode = EWidgetBlendMode::Masked;
    UPROPERTY()
    bool bIsOpaque_DEPRECATED = false;
    UPROPERTY()
    bool bIsTwoSided = false;
    UPROPERTY()
    float ParabolaDistortion = 0.0f;
    UPROPERTY()
    bool TickWhenOffscreen = false;
    UPROPERTY()
    float RenderTargetPixelsPerCentimeter = 0.0f;
    UPROPERTY()
    float SlateLayoutScale = 1.0f;
    UPROPERTY()
    bool bOverrideRenderTargetFormat = false;
    UPROPERTY()
    TEnumAsByte<ETextureRenderTargetFormat> RenderTargetFormatOverride = RTF_RGBA8;
    UPROPERTY()
    bool bOutputDepthAndVelocity = false;
    UPROPERTY()
    TObjectPtr<UUserWidget> Widget = nullptr;
    UPROPERTY()
    TObjectPtr<UBodySetup> BodySetup = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TranslucentMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TranslucentMaterial_OneSided = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TranslucentMaterial_OutputDepthAndVelocity = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TranslucentMaterial_OutputDepthAndVelocity_OneSided = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OpaqueMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> OpaqueMaterial_OneSided = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaskedMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaskedMaterial_OneSided = nullptr;
    UPROPERTY()
    TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MaterialInstance = nullptr;
    UPROPERTY()
    bool bUseLegacyRotation = false;
    UPROPERTY()
    bool bAddedToScreen = false;
    UPROPERTY()
    bool bEditTimeUsable = false;
    UPROPERTY()
    FName SharedLayerName = FName("WidgetComponentScreenLayer");
    UPROPERTY()
    int32 LayerZOrder = -100;
    UPROPERTY()
    EWidgetGeometryMode GeometryMode = EWidgetGeometryMode::Plane;
    UPROPERTY()
    double CylinderArcAngle = 180.0;
    UPROPERTY()
    ETickMode TickMode = ETickMode::Enabled;
};

}
