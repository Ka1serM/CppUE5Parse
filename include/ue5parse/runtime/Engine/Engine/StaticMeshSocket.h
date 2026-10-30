#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"

#include "StaticMeshSocket.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

// UStaticMeshSocket: a named transform relative to a static mesh (UE's Engine/StaticMeshSocket.h).
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
    // The socket's world transform on a component (UE's GetSocketTransform).
    bool GetSocketTransform(FTransform& OutTransform, const UStaticMeshComponent* MeshComp) const;
};

}
