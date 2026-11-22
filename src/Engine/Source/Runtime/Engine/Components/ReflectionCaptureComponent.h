#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ReflectionCaptureComponent.generated.h"

namespace ue5parse::runtime {

class UBillboardComponent;
class UTextureCube;
class UMaterialInterface;

UENUM()
enum class EReflectionSourceType : uint8 { CapturedScene, SpecifiedCubemap };

UENUM()
enum class EMobileReflectionCompression : uint8 { Default, On, Off };

UCLASS(Abstract)
class UReflectionCaptureComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBillboardComponent> CaptureOffsetComponent = nullptr;
    UPROPERTY()
    EReflectionSourceType ReflectionSourceType = EReflectionSourceType::CapturedScene;
    UPROPERTY()
    TObjectPtr<UTextureCube> Cubemap = nullptr;
    UPROPERTY()
    float SourceCubemapAngle = 0.0f;
    UPROPERTY()
    float Brightness = 1.0f;
    UPROPERTY()
    FVector CaptureOffset = FVector::ZeroVector;
    UPROPERTY()
    FLinearColor RuntimeSkylightScale = FLinearColor(0.1f, 0.1f, 0.1f);
    UPROPERTY()
    bool bRuntimeCapture = false;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> PostProcessMaterial = nullptr;
    UPROPERTY()
    float MaxViewDistance = 0.0f;
    UPROPERTY()
    bool bFiniteFarPlane = false;
    UPROPERTY()
    FGuid MapBuildDataId;
    UPROPERTY()
    FGuid StateId;
    UPROPERTY()
    EMobileReflectionCompression MobileReflectionCompression = EMobileReflectionCompression::Default;
    UPROPERTY()
    bool bModifyMaxValueRGBM = false;
    UPROPERTY()
    float MaxValueRGBM = 0.0f;

    explicit UReflectionCaptureComponent(const FObjectInitializer& ObjectInitializer);

    void Serialize(FArchive& Ar) override;

private:
    float AverageBrightness = 1.0f;
    TArray<uint8> FullHDRCapturedData;
    int32 FullHDRCubemapSize = 128;
    TArray<uint8> EncodedHDRCapturedData;
};

}
