#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpression.h"

#include "MaterialExpressionLandscapeLayerCoords.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ETerrainCoordMappingType : int32 { TCMT_Auto, TCMT_XY, TCMT_XZ, TCMT_YZ, TCMT_MAX };

UENUM()
enum ELandscapeCustomizedCoordType : int32 { LCCT_None, LCCT_CustomUV0, LCCT_CustomUV1, LCCT_CustomUV2, LCCT_WeightMapUV, LCCT_MAX };

UCLASS()
class UMaterialExpressionLandscapeLayerCoords : public UMaterialExpression {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<ETerrainCoordMappingType> MappingType = TCMT_Auto;
    UPROPERTY()
    TEnumAsByte<ELandscapeCustomizedCoordType> CustomUVType = LCCT_None;
    UPROPERTY()
    float MappingScale = 0.0f;
    UPROPERTY()
    float MappingRotation = 0.0f;
    UPROPERTY()
    float MappingPanU = 0.0f;
    UPROPERTY()
    float MappingPanV = 0.0f;
};

}
