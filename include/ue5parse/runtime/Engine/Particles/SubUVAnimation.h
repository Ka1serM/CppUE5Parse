#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

#include "SubUVAnimation.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

UENUM()
enum ESubUVBoundingVertexCount : int32 { BVC_FourVertices, BVC_EightVertices };

UENUM()
enum EOpacitySourceMode : int32 { OSM_Alpha, OSM_ColorBrightness, OSM_RedChannel, OSM_GreenChannel, OSM_BlueChannel };

// USubUVAnimation: the frame grid of a flipbook texture and the cooked bounding geometry of each frame, which the
// sprites use to cut out the transparent part of the frame (UE's Particles/SubUVAnimation.h).
UCLASS()
class USubUVAnimation : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UTexture2D> SubUVTexture = nullptr;
    UPROPERTY()
    int32 SubImages_Horizontal = 8;
    UPROPERTY()
    int32 SubImages_Vertical = 8;
    UPROPERTY()
    TEnumAsByte<ESubUVBoundingVertexCount> BoundingMode = BVC_EightVertices;
    UPROPERTY()
    TEnumAsByte<EOpacitySourceMode> OpacitySourceMode = OSM_Alpha;
    UPROPERTY()
    float AlphaThreshold = 0.0f;
    // FSubUVDerivedData: the GetNumBoundingVertices() vertices of each frame, in the frame's UV space.
    TArray<FVector2f> BoundingGeometry;

    void Serialize(FArchive& Ar) override;

    int32 GetNumBoundingVertices() const { return BoundingMode == BVC_FourVertices ? 4 : 8; }
    int32 GetNumBoundingTriangles() const { return BoundingMode == BVC_FourVertices ? 2 : 6; }
    int32 GetNumFrames() const { return SubImages_Vertical * SubImages_Horizontal; }
    bool IsBoundingGeometryValid() const { return BoundingGeometry.Num() != 0; }
    const FVector2f* GetFrameData(int32 FrameIndex) const { return &BoundingGeometry[FrameIndex * GetNumBoundingVertices()]; }
};

}
