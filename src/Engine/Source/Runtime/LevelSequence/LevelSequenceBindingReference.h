#pragma once

#include "Engine/Source/Runtime/MovieScene/MovieSceneBindingReferences.h"

#include "LevelSequenceBindingReference.generated.h"

namespace ue5parse::runtime {

struct FPropertyTag;

USTRUCT()
struct FLevelSequenceBindingReference {
    GENERATED_BODY()

    UPROPERTY()
    FString PackageName;
    UPROPERTY()
    FSoftObjectPath ExternalObjectPath;
    UPROPERTY()
    FString ObjectPath;
};

USTRUCT()
struct FLevelSequenceBindingReferenceArray {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FLevelSequenceBindingReference> References;
};

USTRUCT()
struct FLevelSequenceBindingReferences {
    GENERATED_BODY()

    UPROPERTY()
    TMap<FGuid, FLevelSequenceBindingReferenceArray> BindingIdToReferences;
    UPROPERTY()
    TSet<FGuid> AnimSequenceInstances;
    UPROPERTY()
    TSet<FGuid> PostProcessInstances;
};

USTRUCT()
struct FUpgradedLevelSequenceBindingReferences : public FMovieSceneBindingReferences {
    GENERATED_BODY()

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FUpgradedLevelSequenceBindingReferences> : TStructOpsTypeTraitsBase2<FUpgradedLevelSequenceBindingReferences> {
    enum { WithSerializeFromMismatchedTag = true };
};

}
