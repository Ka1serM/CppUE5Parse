#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "StaticMeshSocket.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS()
class UStaticMeshSocket : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FName SocketName;
    UPROPERTY()
    FVector RelativeLocation = FVector::ZeroVector;
    UPROPERTY()
    FRotator RelativeRotation = FRotator::ZeroRotator;
    UPROPERTY()
    FVector RelativeScale = FVector(1, 1, 1);
    UPROPERTY()
    FString Tag;

    FTransform GetSocketLocalTransform() const { return FTransform(RelativeRotation, RelativeLocation, RelativeScale); }
    bool GetSocketTransform(FTransform& OutTransform, const UStaticMeshComponent* MeshComp) const;
};

}
