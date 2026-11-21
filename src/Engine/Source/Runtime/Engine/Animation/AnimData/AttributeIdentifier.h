#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "AttributeIdentifier.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimationAttributeIdentifier {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName BoneName;
    UPROPERTY()
    int32 BoneIndex = INDEX_NONE;
    UPROPERTY()
    FSoftObjectPath ScriptStructPath;

    bool Serialize(FArchive& Ar) {
        Ar << Name << BoneName << BoneIndex << ScriptStructPath;
        return true;
    }
    bool operator==(const FAnimationAttributeIdentifier& Other) const = default;
};

inline uint32 GetTypeHash(const FAnimationAttributeIdentifier& Id) {
    return HashCombine(GetTypeHash(Id.BoneIndex), GetTypeHash(Id.Name));
}

template <>
struct TStructOpsTypeTraits<FAnimationAttributeIdentifier> : TStructOpsTypeTraitsBase2<FAnimationAttributeIdentifier> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
