#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"

// UE's Components/SkeletalMeshComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

USkeletalMesh* USkeletalMeshComponent::GetSkeletalMeshAsset() const { return Cast<USkeletalMesh>(GetSkinnedAsset()); }

}
