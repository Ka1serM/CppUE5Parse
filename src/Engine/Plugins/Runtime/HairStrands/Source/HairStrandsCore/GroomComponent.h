#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"
#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomDesc.h"

#include "GroomComponent.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;
class UPhysicsAsset;

UCLASS()
class UGroomComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UGroomComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UObject> GroomAsset = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> GroomCache = nullptr;
    UPROPERTY()
    TObjectPtr<USkeletalMesh> SourceSkeletalMesh = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> BindingAsset = nullptr;
    UPROPERTY()
    TObjectPtr<UPhysicsAsset> PhysicsAsset = nullptr;
    UPROPERTY()
    TArray<FHairGroupDesc> GroomGroupsDesc;
    UPROPERTY()
    bool bUseCards = false;
};

}
