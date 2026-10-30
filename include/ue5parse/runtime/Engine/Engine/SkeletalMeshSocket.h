#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"

#include "SkeletalMeshSocket.generated.h"

namespace ue5parse::runtime {

// USkeletalMeshSocket: a named transform relative to a bone (UE's Engine/SkeletalMeshSocket.h).
UCLASS()
class USkeletalMeshSocket : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName SocketName;
    UPROPERTY()
    FName BoneName;
    UPROPERTY()
    FVector RelativeLocation = FVector::ZeroVector;
    UPROPERTY()
    FRotator RelativeRotation = FRotator::ZeroRotator;
    UPROPERTY()
    FVector RelativeScale = FVector(1, 1, 1);
    UPROPERTY()
    bool bForceAlwaysAnimated = true;

    FTransform GetSocketLocalTransform() const { return FTransform(RelativeRotation, RelativeLocation, RelativeScale); }
};

}
