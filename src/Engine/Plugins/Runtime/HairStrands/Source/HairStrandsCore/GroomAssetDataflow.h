#pragma once

#include <memory>

#include "Engine/Source/Runtime/Experimental/Chaos/GeometryCollection/ManagedArrayCollection.h"
#include "Engine/Source/Runtime/Experimental/Dataflow/Engine/Dataflow/DataflowInstance.h"

#include "GroomAssetDataflow.generated.h"

namespace ue5parse::runtime {

class USkeletalMesh;

//! Before 5.7 the struct owns DataflowAsset and an FString DataflowTerminal, which load into the FDataflowInstance members.
USTRUCT()
struct FGroomDataflowSettings : public FDataflowInstance {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FGroomDataflowSettings& Settings) {
        Settings.Serialize(Ar);
        return Ar;
    }

    const FManagedArrayCollection& GetRestCollection() const { return *RestCollection; }

    UPROPERTY()
    TArray<TObjectPtr<USkeletalMesh>> SkeletalMeshes;
    UPROPERTY()
    TArray<int32> MeshLODs;

private:
    std::shared_ptr<FManagedArrayCollection> RestCollection = std::make_shared<FManagedArrayCollection>();
};

template <>
struct TStructOpsTypeTraits<FGroomDataflowSettings> : TStructOpsTypeTraitsBase2<FGroomDataflowSettings> {
    enum { WithSerializer = true };
};

}
