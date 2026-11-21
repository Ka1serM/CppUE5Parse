#pragma once

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PCGLandscapeCache.generated.h"

namespace ue5parse::runtime {

struct FPCGLandscapeCacheEntry {
    TObjectPtr<UObject> Component = nullptr;
    TArray<FName> LayerDataNames;
    FVector PointHalfSize = FVector::One();
    int32 Stride = 0;
    FByteBulkData BulkData;

    void Serialize(FArchive& Ar);
};

struct FPCGLandscapeCacheKey {
    FGuid Guid;
    FIntPoint Coordinates;
};

UCLASS()
class UPCGLandscapeCache : public UObject {
    GENERATED_BODY()
public:
    explicit UPCGLandscapeCache(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    TArray<FPCGLandscapeCacheKey> CachedKeys;
    TArray<FPCGLandscapeCacheEntry> CachedData;
};

}
