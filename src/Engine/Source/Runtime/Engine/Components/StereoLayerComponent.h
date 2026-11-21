#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "StereoLayerComponent.generated.h"

namespace ue5parse::runtime {

class UTexture;

UENUM()
enum EStereoLayerType : int32 { SLT_WorldLocked, SLT_TrackerLocked, SLT_FaceLocked, SLT_MAX };

UENUM()
enum EStereoLayerShape : int32 { SLSH_QuadLayer, SLSH_CylinderLayer, SLSH_CubemapLayer, SLSH_EquirectLayer, SLSH_MAX };

USTRUCT()
struct FEquirectProps {
    GENERATED_BODY()

    UPROPERTY()
    FBox2D LeftUVRect{FVector2D(0.0, 0.0), FVector2D(1.0, 1.0)};
    UPROPERTY()
    FBox2D RightUVRect{FVector2D(0.0, 0.0), FVector2D(1.0, 1.0)};
    UPROPERTY()
    FVector2D LeftScale{1.0, 1.0};
    UPROPERTY()
    FVector2D RightScale{1.0, 1.0};
    UPROPERTY()
    FVector2D LeftBias{0.0, 0.0};
    UPROPERTY()
    FVector2D RightBias{0.0, 0.0};
    UPROPERTY()
    float Radius = 0.0f;
};

UCLASS()
class UStereoLayerShape : public UObject {
    GENERATED_BODY()
public:
    explicit UStereoLayerShape(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UStereoLayerShapeQuad : public UStereoLayerShape {
    GENERATED_BODY()
public:
    explicit UStereoLayerShapeQuad(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UStereoLayerShapeCylinder : public UStereoLayerShape {
    GENERATED_BODY()
public:
    explicit UStereoLayerShapeCylinder(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    float Radius = 100.0f;
    UPROPERTY()
    float OverlayArc = 100.0f;
    UPROPERTY()
    int32 Height = 50;
};

UCLASS()
class UStereoLayerShapeCubemap : public UStereoLayerShape {
    GENERATED_BODY()
public:
    explicit UStereoLayerShapeCubemap(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UStereoLayerShapeEquirect : public UStereoLayerShape {
    GENERATED_BODY()
public:
    explicit UStereoLayerShapeEquirect(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FBox2D LeftUVRect{FVector2D(0.0, 0.0), FVector2D(1.0, 1.0)};
    UPROPERTY()
    FBox2D RightUVRect{FVector2D(0.0, 0.0), FVector2D(1.0, 1.0)};
    UPROPERTY()
    FVector2D LeftScale{1.0, 1.0};
    UPROPERTY()
    FVector2D RightScale{1.0, 1.0};
    UPROPERTY()
    FVector2D LeftBias{0.0, 0.0};
    UPROPERTY()
    FVector2D RightBias{0.0, 0.0};
    UPROPERTY()
    float Radius = 0.0f;
};

UCLASS()
class UStereoLayerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UStereoLayerComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    uint32 bLiveTexture : 1 = false;
    UPROPERTY()
    uint32 bSupportsDepth : 1 = false;
    UPROPERTY()
    uint32 bNoAlphaChannel : 1 = false;
    UPROPERTY()
    TObjectPtr<UTexture> Texture = nullptr;
    UPROPERTY()
    TObjectPtr<UTexture> LeftTexture = nullptr;
    UPROPERTY()
    uint32 bQuadPreserveTextureRatio : 1 = false;
    UPROPERTY()
    FVector2D QuadSize{100.0, 100.0};
    UPROPERTY()
    FBox2D UVRect{FVector2D(0.0, 0.0), FVector2D(1.0, 1.0)};
    UPROPERTY()
    float CylinderRadius_DEPRECATED = 100.0f;
    UPROPERTY()
    float CylinderOverlayArc_DEPRECATED = 100.0f;
    UPROPERTY()
    int32 CylinderHeight_DEPRECATED = 50;
    UPROPERTY()
    FEquirectProps EquirectProps_DEPRECATED;
    UPROPERTY()
    TEnumAsByte<EStereoLayerType> StereoLayerType = SLT_FaceLocked;
    UPROPERTY()
    TEnumAsByte<EStereoLayerShape> StereoLayerShape_DEPRECATED = SLSH_QuadLayer;
    UPROPERTY()
    TObjectPtr<UStereoLayerShape> Shape = nullptr;
    UPROPERTY()
    int32 Priority = 0;
    UPROPERTY()
    TArray<FName> AdditionalFlags;
};

}
