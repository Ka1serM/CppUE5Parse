#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "WavesArenaBGMesh.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UWavesArenaBGMeshBase : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UWavesArenaBGMeshBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        Super::Serialize(Ar);
        if (!Ar.IsError())
            Ar << CustomGameData;
    }

    UPROPERTY()
    FName CustomGameData;
};

UCLASS()
class UWavesArenaBGMeshComponentUnlit : public UWavesArenaBGMeshBase {
    GENERATED_BODY()
public:
    explicit UWavesArenaBGMeshComponentUnlit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UWavesArenaBGMeshComponent : public UWavesArenaBGMeshBase {
    GENERATED_BODY()
public:
    explicit UWavesArenaBGMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
