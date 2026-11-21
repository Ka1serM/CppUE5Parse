#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MagicLeapImageTrackerComponent.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

UENUM()
enum class EMagicLeapImageTargetOrientation : uint8 { ForwardAxisAsNormal, UpAxisAsNormal };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMagicLeapImageTargetUnreliableTrackingMulti, const FVector&, LastTrackedLocation, const FRotator&,
                                              LastTrackedRotation, const FVector&, NewUnreliableLocation, const FRotator&, NewUnreliableRotation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMagicLeapSetImageTargetSucceededMulti);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMagicLeapSetImageTargetFailedMulti);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMagicLeapImageTargetFoundMulti);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMagicLeapImageTargetLostMulti);

UCLASS()
class UMagicLeapImageTrackerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UMagicLeapImageTrackerComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UTexture2D> TargetImageTexture = nullptr;
    UPROPERTY()
    FString Name;
    UPROPERTY()
    float LongerDimension = 0.0f;
    UPROPERTY()
    bool bIsStationary = false;
    UPROPERTY()
    bool bUseUnreliablePose = false;
    UPROPERTY()
    EMagicLeapImageTargetOrientation AxisOrientation = EMagicLeapImageTargetOrientation::ForwardAxisAsNormal;
    UPROPERTY()
    FMagicLeapSetImageTargetSucceededMulti OnSetImageTargetSucceeded;
    UPROPERTY()
    FMagicLeapSetImageTargetFailedMulti OnSetImageTargetFailed;
    UPROPERTY()
    FMagicLeapImageTargetFoundMulti OnImageTargetFound;
    UPROPERTY()
    FMagicLeapImageTargetLostMulti OnImageTargetLost;
    UPROPERTY()
    FMagicLeapImageTargetUnreliableTrackingMulti OnImageTargetUnreliableTracking;
};

}
