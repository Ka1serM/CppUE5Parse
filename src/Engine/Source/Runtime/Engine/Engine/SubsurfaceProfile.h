#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SubsurfaceProfile.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ESubsurfaceImplementationTechniqueHint : uint8 { SIH_AFIS, SIH_Separable };

USTRUCT()
struct FSubsurfaceProfileStruct {
    GENERATED_BODY()
    UPROPERTY()
    FLinearColor SurfaceAlbedo = FLinearColor(0.91058f, 0.338275f, 0.2718f);
    UPROPERTY()
    FLinearColor MeanFreePathColor = FLinearColor(1.0f, 0.1983f / 2.229f, 0.1607f / 2.229f);
    UPROPERTY()
    float MeanFreePathDistance = 1.2f * 2.229f;
    UPROPERTY()
    float WorldUnitScale = 0.1f;
    UPROPERTY()
    bool bEnableBurley = false;
    UPROPERTY()
    bool bEnableMeanFreePath = false;
    UPROPERTY()
    FLinearColor Tint = FLinearColor(1.0f, 1.0f, 1.0f);
    UPROPERTY()
    float ScatterRadius = 1.2f;
    UPROPERTY()
    FLinearColor SubsurfaceColor = FLinearColor(0.48f, 0.41f, 0.28f);
    UPROPERTY()
    FLinearColor FalloffColor = FLinearColor(1.0f, 0.37f, 0.3f);
    UPROPERTY()
    FLinearColor BoundaryColorBleed = FLinearColor::White;
    UPROPERTY()
    ESubsurfaceImplementationTechniqueHint Implementation = ESubsurfaceImplementationTechniqueHint::SIH_AFIS;
    UPROPERTY()
    float ExtinctionScale = 1.0f;
    UPROPERTY()
    float NormalScale = 0.08f;
    UPROPERTY()
    float DistanceScale = 1.0f;
    UPROPERTY()
    float ScatteringDistribution = 0.93f;
    UPROPERTY()
    float IOR = 1.55f;
    UPROPERTY()
    float Roughness0 = 0.75f;
    UPROPERTY()
    float Roughness1 = 1.30f;
    UPROPERTY()
    float LobeMix = 0.85f;
    UPROPERTY()
    FLinearColor TransmissionTintColor = FLinearColor(1.0f, 1.0f, 1.0f);
};

UCLASS()
class USubsurfaceProfile : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSubsurfaceProfileStruct Settings;
    UPROPERTY()
    FGuid Guid;
};

}
