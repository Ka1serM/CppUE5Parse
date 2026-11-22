#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"
#include "Engine/Source/Runtime/InputCore/InputCoreTypes.h"

#include "MotionControllerComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UStaticMesh;

UENUM()
enum class ETrackingStatus : uint8 { NotTracked, InertialOnly, Tracked };

UCLASS()
class UMotionControllerComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UMotionControllerComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    int32 PlayerIndex = 0;
    UPROPERTY()
    EControllerHand Hand_DEPRECATED = EControllerHand::Left;
    UPROPERTY()
    FName MotionSource = FName("Left");
    UPROPERTY()
    uint32 bDisableLowLatencyUpdate : 1 = false;
    UPROPERTY()
    ETrackingStatus CurrentTrackingStatus = ETrackingStatus::NotTracked;
    UPROPERTY()
    bool bDisplayDeviceModel = false;
    UPROPERTY()
    FName DisplayModelSource;
    UPROPERTY()
    TObjectPtr<UStaticMesh> CustomDisplayMesh = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> DisplayMeshMaterialOverrides;
    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> DisplayComponent = nullptr;
};

}
