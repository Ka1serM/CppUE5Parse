#pragma once

#include "Engine/Source/Runtime/Engine/Engine/StreamableRenderAsset.h"
#include "Engine/Source/Runtime/Engine/ReferenceSkeleton.h"

#include "SkinnedAsset.generated.h"

namespace ue5parse::runtime {

class USkeleton;
class UPhysicsAsset;
class FSkeletalMeshRenderData;
class USkeletalMeshSocket;
struct FSkeletalMaterial;

UCLASS(Abstract)
class USkinnedAsset : public UStreamableRenderAsset {
    GENERATED_BODY()
public:
    virtual USkeleton* GetSkeleton() const { return nullptr; }
    virtual const FReferenceSkeleton& GetRefSkeleton() const;
    virtual UPhysicsAsset* GetPhysicsAsset() const { return nullptr; }
    virtual FSkeletalMeshRenderData* GetResourceForRendering() const { return nullptr; }
    virtual const TArray<FSkeletalMaterial>& GetMaterials() const;
    virtual USkeletalMeshSocket* FindSocket(FName InSocketName) const { return nullptr; }
    virtual FBoxSphereBounds GetBounds() const { return FBoxSphereBounds(); }
};

}
