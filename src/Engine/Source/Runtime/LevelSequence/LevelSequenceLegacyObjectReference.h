#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LazyObjectPtr.h"

#include "LevelSequenceLegacyObjectReference.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FLevelSequenceLegacyObjectReference {
    GENERATED_BODY()

    FUniqueObjectGuid ObjectId;
    FString ObjectPath;

    friend FArchive& operator<<(FArchive& Ar, FLevelSequenceLegacyObjectReference& Reference) { return Ar << Reference.ObjectId << Reference.ObjectPath; }
    bool operator==(const FLevelSequenceLegacyObjectReference& Other) const { return ObjectId == Other.ObjectId && ObjectPath.Equals(Other.ObjectPath); }
};

USTRUCT()
struct FLevelSequenceObjectReferenceMap {
    GENERATED_BODY()

    TArray<std::pair<FGuid, FLevelSequenceLegacyObjectReference>> Map;

    bool Serialize(FArchive& Ar);
    bool operator==(const FLevelSequenceObjectReferenceMap& Other) const;
};

template <>
struct TStructOpsTypeTraits<FLevelSequenceObjectReferenceMap> : TStructOpsTypeTraitsBase2<FLevelSequenceObjectReferenceMap> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
