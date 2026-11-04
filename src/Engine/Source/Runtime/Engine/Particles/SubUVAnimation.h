#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SubUVAnimation.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

UENUM()
enum ESubUVBoundingVertexCount : int32 { BVC_FourVertices, BVC_EightVertices };

UENUM()
enum EOpacitySourceMode : int32 { OSM_Alpha, OSM_ColorBrightness, OSM_RedChannel, OSM_GreenChannel, OSM_BlueChannel };

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
    TArray<FVector2f> BoundingGeometry;

    void Serialize(FArchive& Ar) override;

    int32 GetNumBoundingVertices() const { return BoundingMode == BVC_FourVertices ? 4 : 8; }
    int32 GetNumFrames() const { return SubImages_Vertical * SubImages_Horizontal; }
    bool IsBoundingGeometryValid() const { return BoundingGeometry.Num() != 0; }
};

}
