#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "SkeletalMeshSocket.generated.h"

namespace ue5parse::runtime {

class USkeletalMeshComponent;

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
    bool GetSocketMatrixWithOffset(FMatrix& OutMatrix, USkeletalMeshComponent* SkelComp, const FVector& InOffset, const FRotator& InRotation) const;
};

}
