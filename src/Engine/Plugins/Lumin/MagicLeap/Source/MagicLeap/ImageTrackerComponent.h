#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ImageTrackerComponent.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FImageTargetFound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FImageTargetLost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FImageTargetUnreliableTracking, const FVector&, LastTrackedLocation, const FRotator&, LastTrackedRotation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetImageTargetSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSetImageTargetFailed);

UCLASS()
class UImageTrackerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UImageTrackerComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FSetImageTargetSucceeded OnSetImageTargetSucceeded;
    UPROPERTY()
    FSetImageTargetFailed OnSetImageTargetFailed;
    UPROPERTY()
    FImageTargetFound OnImageTargetFound;
    UPROPERTY()
    FImageTargetLost OnImageTargetLost;
    UPROPERTY()
    FImageTargetUnreliableTracking OnImageTargetUnreliableTracking;
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
};

}
